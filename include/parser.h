#ifndef PARSER_H
#define PARSER_H

#define MAX_ARGS 128
#define MAX_CMDS 32
#define MAX_LINE 8192

typedef struct command {
    char *argv[MAX_ARGS];
    char *infile;
    char *outfile;
    int append;
} command_t;

int parse_line(char *line, command_t cmds[], int *background);

#endif
