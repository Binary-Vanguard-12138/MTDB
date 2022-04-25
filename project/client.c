#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFSIZE 1024

/*
 * Helper that opens a TCP socket representing the server.
 * Returns the file descriptor on success, -1 on failure.
 */
int get_socket(const char *server, const char *port) {
    // setup for getaddrinfo
    int sock;
    struct addrinfo hints;
    struct addrinfo *result;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int err;
    if ((err = getaddrinfo(server, port, &hints, &result)) != 0) {
        fprintf(stderr, "Error in getaddrinfo: %s\n", gai_strerror(err));
        return -1;
    }

    // find the right interface
    struct addrinfo *res;
    for (res = result; res != NULL; res = res->ai_next) {
        if ((sock = socket(res->ai_family, res->ai_socktype,
                           res->ai_protocol)) < 0) {
            continue;
        }
        if (connect(sock, res->ai_addr, res->ai_addrlen) >= 0) {
            break;
        }
        close(sock);
    }

    freeaddrinfo(result);

    if (res == NULL) {
        fprintf(stderr, "Failed to connect to '%s'!\n", server);
        return -1;
    }

    return sock;
}

/*
 * Forks off a process that attempts to connect to the server, and then run the
 * script in the file provided.
 * Returns the pid of the child process.
 */
pid_t create_occurence(const char *server, const char *port,
                       const char *script) {
    pid_t pid;

    // create a process for the client
    if ((pid = fork()) == 0) {
        // Step 2: open the script if present, but default to stdin
        FILE *infile;
        if (script != NULL) {
            if ((infile = fopen(script, "r")) == NULL) {
                perror("Error opening script file");
                exit(1);
            }
        } else {
            infile = stdin;
        }

        // Step 3: set up a new connection to the server
        int sock;
        if ((sock = get_socket(server, port)) == -1) {
            exit(1);
        }

        // Step 4: loop, sending queries and printing responses
        FILE *cxn = fdopen(sock, "w+");
        char rbuf[BUFSIZE], qbuf[BUFSIZE];
        rbuf[0] = '\0';

        while (1) {
            // if there are no more commands, so we can clean up and exit
            if (fgets(qbuf, sizeof(qbuf), infile) == NULL) {
                qbuf[0] = EOF;
                fputs(qbuf, cxn);
                fflush(cxn);
                fclose(cxn);
                fclose(infile);
                printf("Client terminated cleanly.\n");
                exit(0);
            } else {
                // otherwise, send the command
                if (fputs(qbuf, cxn) == EOF) {
                    fprintf(stderr, "No connection!\n");
                    exit(1);
                }
                fflush(cxn);
            }

            // wait for the response and print it
            if (fgets(rbuf, BUFSIZE, cxn) == NULL) {
                fprintf(stderr, "Connection terminated.\n");
                exit(1);
            }
            printf("%s", rbuf);
        }
    }

    // return pid of child
    return pid;
}

/*
 * Prints a usage tip.
 */
void usage_error(const char *cmd) {
    fprintf(stderr,
            "Usage: %s <servername> <port> "
            "[<script> <occurences>]\n",
            cmd);
}

/*
 * The arguments to the client should be servername, port number,
 * [script-file, number of occurences].
 *
 * Step 1: fork to create as many clients as number of occurences argument
 *
 * Step 2: open the script-file
 *
 * Step 3: find the server address, set up socket for TCP and connect to server
 *
 * Step 4: set up an infinite loop that sends queries from the
 *         script-file to the server and prints responses (if any exist)
 */
int main(int argc, const char *argv[]) {
    // parse args
    if (argc != 3 && argc != 5) {
        usage_error(argv[0]);
        return 1;
    }

    int i, occurences = 1;
    const char *script = NULL;
    const char *server = argv[1];
    const char *port = argv[2];

    if (argc == 5) {
        script = argv[3];
        occurences = atoi(argv[4]);
    }

    // Step 1: create clients, they'll do the rest
    for (i = 0; i < occurences; i++) {
        if (create_occurence(server, port, script) == -1) {
            perror("Error forking off process");
            return 1;
        }
    }

    // wait for children to terminate
    for (i = 0; i < occurences; i++) {
        if (wait(0) == -1) {
            perror("wait");
            return 1;
        }
    }

    return 0;
}
