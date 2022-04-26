#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "./comm.h"
#include "./db.h"

// Global variable to keep track of whether the server is still accepting clients.
// Server should stop receiving clients in case of EOF, so this variable is set to
// 1 in main after the while loop.
int accepting;
/* 
 * Use the variables in this struct to synchronize your main thread with client
 * threads. Note that all client threads must have terminated before you clean
 * up the database. 
 */
typedef struct server_control {
    pthread_mutex_t server_mutex;
    pthread_cond_t server_cond;
    int num_client_threads;
} server_control_t;

/*
 * Controls when the clients in the client thread list should be stopped and
 * let go.
 */
typedef struct client_control {
    pthread_mutex_t go_mutex;
    pthread_cond_t go;
    int stopped;
} client_control_t;

/*
 * The encapsulation of a client thread, i.e., the thread that handles
 * commands from clients.
 */
typedef struct client {
    pthread_t thread;
    FILE *cxstr;  // File stream for input and output
    // For client list
    struct client *prev;
    struct client *next;
} client_t;

/*
 * The encapsulation of a thread that handles signals sent to the server.
 * When SIGINT is sent to the server all client threads should be destroyed.
 */
typedef struct sig_handler {
    sigset_t set;
    pthread_t thread;
} sig_handler_t;


client_t *thread_list_head;
pthread_mutex_t thread_list_mutex = PTHREAD_MUTEX_INITIALIZER;
client_control_t client_control = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 1};
server_control_t server_control = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0};

void *run_client(void *arg);
void *monitor_signal(void *arg);
void thread_cleanup(void *arg);

/*
 * Called by client threads to wait until progress is permitted.
 */
void client_control_wait() {
    // TODO: Block the calling thread until the main thread calls
    // client_control_release(). See the client_control_t struct.
}
/*
 * Called by main thread to stop client threads
 */
void client_control_stop() {
    // TODO: Ensure that the next time client threads call client_control_wait()
    // at the top of the event loop in run_client, they will block.
}
/*
 * Called by main thread to resume client threads
 */
void client_control_release() {
    // TODO: Allow clients that are blocked within client_control_wait()
    // to continue. See the client_control_t struct.
}
/*
 * Called by listener (in comm.c) to create a new client thread
 */
void client_constructor(FILE *cxstr) {
    // Mallocing memory for client
    client_t* client;
    if ((client = (client_t*) malloc(sizeof(client_t))) == NULL){
        perror("malloc failed: \n");
        exit(1);
    }
    // Client socket.
    client->cxstr = cxstr;
    int err;
    // Creates thread;
    if ((err = pthread_create(&client->thread, 0, run_client, client))){
        handle_error_en(err, "pthread_create");
    }
    if ((err = pthread_detach(client->thread))){
        handle_error_en(err, "pthread_detach");
    }
}
/*
 * Shuts down client's connection to the server and frees the
 * space allocated for a variable pointing to the client.
 */
void client_destructor(client_t *client) {    
    comm_shutdown(client->cxstr);
    free(client);
}
/*
 * Client threads created are to run this function. In it,
 * the client list is modified to take in the new client, the signal
 * set associated with it is set to ignore SIGPIPE, and starts to wait
 * for and execute commands pertaining to the client.
 */
void *run_client(void *arg) {
    client_t* client = (client_t*) arg;
    if (accepting){
        // Server has stopped accepting clients, indicating EOF.
        client_destructor(client);
        return (void*) -1;
    }
    if (pthread_mutex_lock(&thread_list_mutex)){
        perror("mutex could not be locked: \n");
        exit(1);
    }
    if (!thread_list_head){
        client->next = NULL;
    } else {
        client->next = thread_list_head;
        thread_list_head->prev = client;
    }
    client->prev = NULL;
    thread_list_head = client;
    pthread_cleanup_push(&thread_cleanup, client);
    if (pthread_mutex_unlock(&thread_list_mutex)){
        perror("mutex could not be locked: \n");
        exit(1);
    }
    if (pthread_mutex_lock(&server_control.server_mutex)){
        perror("mutex could not be locked: \n");
        exit(1);
    }    
    server_control.num_client_threads++;
    if (pthread_mutex_unlock(&server_control.server_mutex)){
        perror("mutex could not be locked: \n");
        exit(1);
    }
    char response[BUFLEN];
    char command[BUFLEN];
    memset(&response, 0, BUFLEN);
    memset(&command, 0, BUFLEN);
    while(comm_serve(client->cxstr, response, command) == 0){
        interpret_command(command, response, BUFLEN);
    }
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
    pthread_cleanup_pop(1);
    return NULL;
}
/*
 * Cancels all client threads. Must do this so that pthread_cleanup
 * is called and the appropriate cleaning up of resources is achieved
 * as desired.
 */
void delete_all() {
    // Must be thread-safe.
    if (pthread_mutex_lock(&thread_list_mutex)){
        perror("mutex could not be locked: \n");
        exit(1);
    }
    client_t* curr = thread_list_head;   
    while (curr){
        client_t* next = curr->next;
        pthread_cancel(curr->thread);
        curr = next;
    }
    if (pthread_mutex_unlock(&thread_list_mutex)){
        perror("mutex could not be locked: \n");
        exit(1);
    }   
}

/*
 * Cleanup routine for client threads, called on cancels and exit.
 */
void thread_cleanup(void *arg) {
    // Removes client from the list. Since list is
    // modified in it, this function must be thread-safe.
    if (pthread_mutex_lock(&thread_list_mutex)){
        perror("mutex could not be locked: \n");
        exit(1);
    }
    client_t* client = (client_t*) arg;
    client_t* next = client->next;
    client_t* prev = client->prev;
    if (client == thread_list_head){
        thread_list_head = NULL;
    }                     
    if (next){
        next->prev = prev;  
    }
    if (prev){
        prev->next = next;
    }
    if (pthread_mutex_unlock(&thread_list_mutex)){
        perror("mutex could not be unlocked: \n");
        exit(1);
    }
    // Necessary to shut down client's connection to the server
    // and free up the space allocated for it.
    client_destructor(client);
    if (pthread_mutex_lock(&server_control.server_mutex)){
        perror("mutex could not be locked: \n");
        exit(1);
    }
    // Decrementing global that keeps track of the number of
    // active clients connected to the server.
    server_control.num_client_threads--;
    if (!server_control.num_client_threads){
        if (pthread_cond_signal(&server_control.server_cond)){
            perror("pthread_cond_broadcast failure: \n");
            exit(1);
        }
    }
    if (pthread_mutex_unlock(&server_control.server_mutex)){
        perror("mutex could not be locked: \n");
        exit(1);
    }
}
/*
// Code executed by the signal handler thread. For the purpose of this
// assignment, there are two reasonable ways to implement this.
// The one you choose will depend on logic in sig_handler_constructor.
// 'man 7 signal' and 'man sigwait' are both helpful for making this
// decision. One way or another, all of the server's client threads
// should terminate on SIGINT. The server (this includes the listener
// thread) should not, however, terminate on SIGINT!
void *monitor_signal(void *arg) {
    // TODO: Wait for a SIGINT to be sent to the server process and cancel
    // all client threads when one arrives.
    return NULL;
}

sig_handler_t *sig_handler_constructor() {
    // TODO: Create a thread to handle SIGINT. The thread that this function
    // creates should be the ONLY thread that ever responds to SIGINT.
    return NULL;
}

void sig_handler_destructor(sig_handler_t *sighandler) {
    // TODO: Free any resources allocated in sig_handler_constructor.
    // Cancel and join with the signal handler's thread. 
}

 * Main of program.
 */
int main(int argc, char *argv[]) {
    // Must have exactly one argument. 
    if(argc != 2) {
        fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
        exit(1);
    }
    // Constructs listener thread which constructs clients.
    pthread_t server_thread = start_listener(atoi(argv[1]), &client_constructor);
    // Buffer to store command input to server.
    char server_command[BUFLEN];
    ssize_t read_count;
    memset(server_command, 0, BUFLEN);
    while ((read_count = read(STDIN_FILENO, server_command, BUFLEN)) > 0){
        if (server_command[0] == 'p'){
            char* file = strtok(&server_command[1], " \t\n");
            db_print(file);
        } else {
            if (strtok(&server_command[1], " \t\n")){
                continue;
            }
        }       
        memset(server_command, 0, BUFLEN);
    }
    if (read_count < 0){
        perror("read failure: \n");
        exit(1);
    } 
    if (!read_count){
        // Thread reaches here upon receiving EOF, in which case it increments 
        // global below to make sure no more clients can connect to the server.
        accepting++;
        if (fprintf(stdout, "exiting database\n") < 0){
            perror("fprintf failure: \n");
            exit(1);
        }
    }
    // Removes all active clients.
    delete_all();
    // Must make sure that we have actually removed all clients before we clean up 
    // database to avoid memory issues. Puts listener thread to sleep till the number 
    // of active clients is 0.
    if (pthread_mutex_lock(&server_control.server_mutex)){
        perror("mutex could not be locked: \n");
        exit(1);
    }
    while(server_control.num_client_threads){
        if (pthread_cond_wait(&server_control.server_cond, &server_control.server_mutex)){
            perror("pthread_cond_wait failure: \n");
            exit(1);
        }
    }
    if (pthread_mutex_unlock(&server_control.server_mutex)){
        perror("mutex could not be unlocked: \n");
        exit(1);
    }
    // Cleans up database resources after every client has been removed as desired.
    db_cleanup();
    int err;
    if (err = pthread_cancel(server_thread)){
        handle_error_en(err, "pthread_cancel");
    }  
    if (pthread_mutex_destroy(&client_control.go_mutex)){
        perror("mutex could not be destroyed: \n");
        exit(1);
    }
    if (pthread_mutex_destroy(&server_control.server_mutex)){
        perror("mutex could not be destroyed: \n");
        exit(1);
    }
    if (pthread_mutex_destroy(&thread_list_mutex)){
        perror("mutex could not be destroyed: \n");
        exit(1);
    }
    if (pthread_cond_destroy(&client_control.go)){
        perror("condition could not be destroyed: \n");
        exit(1);
    }
    if (pthread_cond_destroy(&server_control.server_cond)){
        perror("condition could not be destroyed: \n");
        exit(1);
    }

    pthread_exit(0);
}
