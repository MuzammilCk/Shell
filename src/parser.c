#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parser.h"

static char *next_token(char **s) {
    char *p = *s;
    if (!p) return NULL;
    while (isspace((unsigned char)*p)) p++;
    if (*p == '\0') { *s = NULL; return NULL; }

    char *start = p;
    if (*p == '"' || *p == '\'') {
        char quote = *p++;
        start = p;
        while (*p && *p != quote) {
            if (*p == '\\' && p[1]) p += 2; else p++;
        }
        if (*p == quote) {
            *p = '\0';
            p++;
        }
        *s = p;
        return start;
    }

    if (p[0] == '>' && p[1] == '>') {
        *s = p + 2;
        return ">>";
    }
    if (*p == '<' || *p == '>' || *p == '|' || *p == '&') {
        static char op[2];
        op[0] = *p;
        op[1] = '\0';
        *s = p + 1;
        return op;
    }

    start = p;
    while (*p && !isspace((unsigned char)*p) && *p != '<' && *p != '>' && *p != '|' && *p != '&') p++;
    if (*p) {
        *p = '\0';
        *s = p + 1;
    } else {
        *s = NULL;
    }
    return start;
}

int parse_line(char *line, command_t cmds[], int *background) {
    char *p = line;
    int cmd_idx = 0;
    int arg_idx = 0;
    cmds[0].infile = cmds[0].outfile = NULL; cmds[0].append = 0;
    for (int i=0;i<MAX_CMDS;i++) for (int j=0;j<MAX_ARGS;j++) cmds[i].argv[j]=NULL;

    *background = 0;
    char *tok;
    while ((tok = next_token(&p))) {
        if (strcmp(tok, "|") == 0) {
            cmd_idx++;
            if (cmd_idx >= MAX_CMDS) { fprintf(stderr, "tsh: too many piped commands\n"); return -1; }
            arg_idx = 0;
            cmds[cmd_idx].infile = cmds[cmd_idx].outfile = NULL; cmds[cmd_idx].append = 0;
            continue;
        }
        if (strcmp(tok, "<") == 0) {
            char *file = next_token(&p);
            if (!file) { fprintf(stderr, "tsh: syntax error: expected filename after '<'\n"); return -1; }
            cmds[cmd_idx].infile = file;
            continue;
        }
        if (strcmp(tok, ">") == 0 || strcmp(tok, ">>") == 0) {
            int append = (strcmp(tok, ">>") == 0);
            char *file = next_token(&p);
            if (!file) { fprintf(stderr, "tsh: syntax error: expected filename after '>'\n"); return -1; }
            cmds[cmd_idx].outfile = file;
            cmds[cmd_idx].append = append;
            continue;
        }
        if (strcmp(tok, "&") == 0) {
            *background = 1;
            continue;
        }
        cmds[cmd_idx].argv[arg_idx++] = tok;
        if (arg_idx >= MAX_ARGS-1) { fprintf(stderr, "tsh: too many args\n"); return -1; }
    }
    return cmd_idx+1;
}
