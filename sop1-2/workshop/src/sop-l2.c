#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>

#define MAX 256
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t lastSignal = 0;
volatile sig_atomic_t ifINT = 0;

ssize_t bulk_read(int fd, char* buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0)
            return c;
        if (c == 0)
            return len;  // EOF
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

ssize_t bulk_write(int fd, char* buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0)
            return c;
        buf += c;
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

void betterSleep(struct timespec t, char buf, int out)
{
    struct timespec rest;
    while(nanosleep(&t, &rest)!=0)
    {
        if(errno == EINTR)
        {
            if(lastSignal == SIGINT)
            {
                if((bulk_write(out, &buf, 1)) < 0) ERR("write");
                if(close(out) < 0) ERR("close");
                exit(EXIT_SUCCESS);
            }
        }
        t=rest;
    }

}

void sighandler(int sig)
{
    lastSignal = sig;
}

void sighandler_int(int sig)
{
    ifINT = sig;
}


void sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void child_work(char * text, int i, char* filename)
{
    sigset_t empty;
    int index, out;
    struct timespec t ={0,250000000};
    char buf;

    char name[MAX];
    snprintf(name, sizeof(name), "%s-%d",filename, i);

    if ((out = TEMP_FAILURE_RETRY(open(name, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0777))) < 0)
        ERR("open");

    sigemptyset(&empty);
    while(lastSignal != SIGUSR1 &&  lastSignal != SIGINT) 
        sigsuspend(&empty);

    
    if(lastSignal == SIGINT)
    {
        if(close(out) < 0) ERR("close");
        exit(EXIT_SUCCESS);
    }

    for(index = 0; index<strlen(text); index = index+2)
    {

        if((text[index] >= 65 && text[index] <= 90) || (text[index] >= 97 && text[index] <= 122))
        {
            if(text[index] <= 90)
                text[index] = tolower(text[index]);
            else
                text[index] = toupper(text[index]);
            betterSleep(t, text[index],out);
            buf = text[index];
            if(bulk_write(out, &buf,1) < 0)
                ERR("write");  
        }
    }
    if(close(out)) ERR("close");
    if (TEMP_FAILURE_RETRY(fprintf(stderr, "PID: %d text: %s \n", getpid(), text)) < 0)
           ERR("fprintf");
}

void parent_work(char * name, int n)
{
    struct stat sb;
    if(stat(name, &sb) == -1) ERR("stat");
    off_t len = sb.st_size;

    int bufLen = len/n;
    int missing = len - (n-1)*bufLen;
    int in;
    int pointer = 0;

    if ((in = TEMP_FAILURE_RETRY(open(name, O_RDONLY))) < 0)
        ERR("open");
    
    while(n-- > 0)
    {
        if(n == 1) bufLen = missing;
        char *text = malloc(bufLen + 1);
        if (!text) ERR("malloc");
        switch(fork())
        {
            case 0:
                if(lseek(in, pointer, SEEK_SET)<0)
                {
                    close(in);
                    ERR("lseek");
                }
                ssize_t count = bulk_read(in, text, bufLen);
                if(count < 0)
                    ERR("bulk_read");
                text[count] = '\0';
                close(in);
                child_work(text, n, name);
                free(text);
                exit(EXIT_SUCCESS);
                
            case -1:
                ERR("Fork:");
                exit(EXIT_FAILURE);
        }
        free(text);
        pointer += bufLen;
    }
    if(kill(0,SIGUSR1)<0) ERR("kill");
}


void sigchld_handler(int sig)
{
    pid_t pid;
    for (;;)
    {
        pid = waitpid(0, NULL, WNOHANG);
        if (pid == 0)
            return;
        if (pid <= 0)
        {
            if (errno == ECHILD)
                return;
            ERR("waitpid");
        }
    }
}


void usage(int argc, char* argv[])
{
    printf("%s n f \n", argv[0]);
    printf("\tf - file to be processed\n");
    printf("\t0 < n < 10 - number of child processes\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
    if(argc < 3)
        usage(argc, argv);
    char*name  = argv[1];
    int n = atoi(argv[2]);
    if(n <= 0 || n >= 10)
        usage(argc, argv);
    sethandler(sigchld_handler, SIGCHLD);
    sethandler(sighandler, SIGUSR1);
    sethandler(sighandler_int, SIGINT);
    parent_work(name,n);
    while (wait(NULL) > 0)
        ;
    return EXIT_SUCCESS;
}
