#include<stdio.h>
#include<stdlib.h>
#include "shell.h"

int main(){
    char input[1024];
    while(1){
        printf("shell> ");
        fflush(stdout);

        if(!fgets(input,1024,stdin)){
            break;
        }
        run_command(input);
    }
}