#include "shell.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdio.h>

void run_command(char *input){
    char *args[64];
    char *token = strtok(input," \t\n");
    int i=0;
    while(token){
        args[i++]=token;
        token=strtok(NULL," \t\n");
    }
    args[i]=NULL;
    if(args[0]==NULL)return;
    if(strcmp(args[0],"exit")==0){
        exit(0);
    }
    if(strcmp(args[0],"cd")==0){
        if(args[1]){
            chdir(args[1]);
        }
        return;
    }
    pid_t pid=fork();
    if(pid==0){
        execvp(args[0],args);
        perror("execvp failed");
        exit(1);
    }
    else{
        wait(NULL);
    }
}