#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source) \
    (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define UNUSED(x) (void)(x)

void usage(const char *pname)
{
    fprintf(stderr, "USAGE: %s t k n p\n", pname);
    fprintf(stderr, "\tt - simulation time in seconds (1-100)\n");
    fprintf(stderr, "\tk - time after ill child is picked up by their parents (1-100)\n");
    fprintf(stderr, "\tn - number of children\n");
    fprintf(stderr, "\tp - likelihood of contracting the disease after contact with the virus\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    UNUSED(argc);
    usage(argv[0]);
    
    exit(EXIT_SUCCESS);
}
