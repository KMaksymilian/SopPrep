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

volatile sig_atomic_t lastSignal = 0;

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

void sighandler(int sig)
{
    lastSignal = sig;
}

void alarm_handler()
{
    printf("dead %d", getpid());
    exit(EXIT_SUCCESS);
}


void sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

void sigchld_handler()
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

void betterSleep(struct timespec t)
{
    struct timespec rest;
    while(nanosleep(&t, &rest)!=0)
    {
        if(errno == EINTR)
        {
            if(lastSignal == SIGTERM)
            {
                printf("pid: %d\n", getpid());
                exit(EXIT_SUCCESS);
            }
        }
        t=rest;
    }

}

void child_work(int Plague, int p, int k)
{
    int isIll = Plague;
    int prob;
    int t;


    sigset_t empty;
    sigemptyset(&empty);
    while(1)
    {  
        if(isIll == 0)
        {
            while(lastSignal != SIGTERM && lastSignal != SIGUSR1) 
                sigsuspend(&empty);
            if(lastSignal == SIGTERM)
            {
                printf("pid: %d\n", getpid());
                exit(EXIT_SUCCESS);
            }
            prob = rand() % 101;
            if(prob<=p)
            {
                isIll = 1;
                alarm(k);   
            }
        } 
        else
        {   
            
            while (lastSignal != SIGALRM)
            {
                t = (50 + rand() % 151) * 1000000;
                struct timespec time = {0,t};
                betterSleep(time);
                if(kill(0,SIGUSR1)<0)
                ERR("kill");
            }

        }



    }
    /*srand(getpid());
    int t = 300 + rand()  % 701;
    struct timespec time = {0,t};
    betterSleep(time);*/
}

void parent_work(int n, int t, int p, int k)
{
    int i;
    for(i=0; i<n; i++)
    {
        switch(fork())
        {
            case 0:
                if(i == 0) child_work(1,p, k);
                else child_work(0, p, k); 
                exit(EXIT_SUCCESS);
                
            case -1:
                ERR("Fork:");
        }
    }
    
    alarm(t);
    while (lastSignal != SIGALRM)
    {
        ;
    }
    if(kill(0,SIGTERM) < 0)
        ERR("kill");

}



int main(int argc, char *argv[])
{
    if(argc < 5)
        usage(argv[0]);
    int t = atoi(argv[1]);
    int k = atoi(argv[2]);
    int n = atoi(argv[3]);
    int p = atoi(argv[4]);
    if(t < 1 || t > 100 || k < 1 || k>100 || n<1 || n>30 || p<1 || p>100)
        usage(argv[0]);
    sethandler(sigchld_handler, SIGCHLD);
    sethandler(sighandler, SIGUSR1);
    sethandler(sighandler,SIGTERM);
    sethandler(sighandler, SIGALRM);
    parent_work(n,t,p);
    while (TEMP_FAILURE_RETRY(wait(NULL)) > 0)
        ;

    exit(EXIT_SUCCESS);
}
