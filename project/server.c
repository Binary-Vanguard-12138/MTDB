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

void *run_client(void *arg);
void *monitor_signal(void *arg);
void thread_cleanup(void *arg);

// Called by client threads to wait until progress is permitted
void client_control_wait() {
    // TODO: Block the calling thread until the main thread calls
    // client_control_release(). See the client_control_t struct.
}

// Called by main thread to stop client threads
void client_control_stop() {
    // TODO: Ensure that the next time client threads call client_control_wait()
    // at the top of the event loop in run_client, they will block.
}

// Called by main thread to resume client threads
void client_control_release() {
    // TODO: Allow clients that are blocked within client_control_wait()
    // to continue. See the client_control_t struct.
}

// Called by listener (in comm.c) to create a new client thread
void client_constructor(FILE *cxstr) {
    // You should create a new client_t struct here and initialize ALL
    // of its fields. Remember that these initializations should be
    // error-checked.
    //
    // TODO:
    // Step 1: Allocate memory for a new client and set its connection stream
    // to the input argument.
    // Step 2: Create the new client thread running the run_client routine.
    // Step 3: Detach the new client thread 
}

void client_destructor(client_t *client) {
    // TODO: Free and close all resources associated with a client.
    // Whatever was malloc'd in client_constructor should
    // be freed here!
}

// Code executed by a client thread
void *run_client(void *arg) {
    // TODO:
    // Step 1: Make sure that the server is still accepting clients.
    // Step 2: Add client to the client list and push thread_cleanup to remove
    //       it if the thread is canceled.
    // Step 3: Loop comm_serve (in comm.c) to receive commands and output
    //       responses. Execute commands using interpret_command (in db.c)   
    // Step 4: When the client is done sending commands, exit the thread
    //       cleanly.
    //
    // Keep stop and go in mind when writing this function!
    return NULL;
}

void delete_all() {
    // TODO: Cancel every thread in the client thread list with the
    // pthread_cancel function.
}

// Cleanup routine for client threads, called on cancels and exit.
void thread_cleanup(void *arg) {
    // TODO: Remove the client object from thread list and call
    // client_destructor. This function must be thread safe! The client must
    // be in the list before this routine is ever run.
}

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

// The arguments to the server should be the port number.
int main(int argc, char *argv[]) {
    // TODO:
    // Step 1: Set up the signal handler.
    // Step 2: block SIGPIPE so that the server does not abort when a client disocnnects
    // Step 3: Start a listener thread for clients (see start_listener in
    //       comm.c).
    // Step 4: Loop for command line input and handle accordingly until EOF.
    // Step 5: Destroy the signal handler, delete all clients, cleanup the
    //       database, cancel and join with the listener thread
    //
    // You should ensure that the thread list is empty before cleaning up the
    // database and canceling the listener thread. Think carefully about what
    // happens in a call to delete_all() and ensure that there is no way for a
    // thread to add itself to the thread list after the server's final
    // delete_all().

    return 0;
}
