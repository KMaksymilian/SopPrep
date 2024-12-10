#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

#define MAX_LINE 999;

void usage(char *name)
{
    printf("%s pattern\n", name);
    printf("pattern - string pattern to search at standard input\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
    if(argc<2 || argc>3) usage(argv[0]);
    
    char *pattern = argv[argc-1];

    size_t line_len = MAX_LINE;
    char* line;
    int numeral = 0;
    int c=0;
   
    while((c=getopt(argc,argv,"n"))!=-1){
        switch(c){
            case 'n':
                numeral = 1;
                break;
            case '?':
            default:
                usage(argv[0]);
        }
    }

    int currentLine=1;
    while (getline(&line, &line_len, stdin) != -1)  // man 3p getdelim
    {
        if (strstr(line, pattern)){
            if(numeral==1) printf("%d,%s",currentLine,line);
            else printf("%s",line);  // getline() should return null terminated data
        }
        currentLine++;
    }

    if (line)
        free(line);

    return EXIT_SUCCESS;
}