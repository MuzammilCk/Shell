#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include "builtins.h"
#include "job_control.h"

#define HISTORY_SIZE 200

static char *history[HISTORY_SIZE];
static int history_len = 0;

void add_history(const char *line) {
    if (!line || *line == '\0') return;
    if (history_len == HISTORY_SIZE) {
        free(history[0]);
        memmove(history, history+1, sizeof(char*)*(HISTORY_SIZE-1));
        history_len--;
    }
    history[history_len++] = strdup(line);
}

int get_history_length(void) {
    return history_len;
}

const char *get_history_item(int index) {
    if (index < 0 || index >= history_len) return NULL;
    return history[index];
}

void free_history(void) {
    for (int i=0;i<history_len;i++) free(history[i]);
}

static void print_help(void) {
    printf("tsh - Tiny enhanced shell\n");
    printf("Built-in commands:\n");
    printf("  cd [dir]      - change directory (HOME if omitted)\n");
    printf("  pwd           - print working directory\n");
    printf("  exit          - exit shell\n");
    printf("  help          - show this help\n");
    printf("  history       - show command history\n");
    printf("  jobs          - list background jobs\n");
    printf("  fg %%jid       - bring background job to foreground\n");
    printf("Features: quotes, multiple pipes, <, >, >>, background (&)\n");
}

int is_builtin(command_t *c) {
    if (!c->argv[0]) return 0;
    const char *b[] = {"cd","pwd","exit","help","history","jobs","fg","bg","export","unset", NULL};
    for (int i=0;b[i];i++) if (strcmp(c->argv[0], b[i])==0) return 1;
    return 0;
}

int handle_builtin(command_t *c) {
    if (!c->argv[0]) return 0;
    if (strcmp(c->argv[0], "exit") == 0) {
        exit(0);
    }
    if (strcmp(c->argv[0], "cd") == 0) {
        const char *dir = c->argv[1] ? c->argv[1] : getenv("HOME");
        if (!dir || chdir(dir) != 0) perror("tsh: cd");
        return 1;
    }
    if (strcmp(c->argv[0], "pwd") == 0) {
        char cwd[4096]; if (getcwd(cwd, sizeof(cwd))) printf("%s\n", cwd);
        return 1;
    }
    if (strcmp(c->argv[0], "help") == 0) { print_help(); return 1; }
    if (strcmp(c->argv[0], "history") == 0) {
        for (int i=0;i<history_len;i++) printf("%4d  %s", i+1, history[i]);
        return 1;
    }
    if (strcmp(c->argv[0], "jobs") == 0) { print_jobs(); return 1; }
    if (strcmp(c->argv[0], "fg") == 0) {
        if (!c->argv[1]) { fprintf(stderr, "tsh: fg: expected %%jid\n"); return 1; }
        int jid = 0;
        if (c->argv[1][0] == '%') jid = atoi(c->argv[1]+1); else jid = atoi(c->argv[1]);
        job_t *j = find_job_by_jid(jid);
        if (!j) { fprintf(stderr, "tsh: fg: job not found\n"); return 1; }
        
        j->state = JOB_RUNNING;
        kill(-j->pgid, SIGCONT);
        
        int status;
        while (waitpid(-j->pgid, &status, WUNTRACED) > 0) {
            if (WIFSTOPPED(status)) {
                j->state = JOB_STOPPED;
                printf("\n[%d] Stopped   %s\n", j->jid, j->cmdline);
                return 1;
            }
        }
        remove_job(j);
        return 1;
    }
    if (strcmp(c->argv[0], "bg") == 0) {
        if (!c->argv[1]) { fprintf(stderr, "tsh: bg: expected %%jid\n"); return 1; }
        int jid = 0;
        if (c->argv[1][0] == '%') jid = atoi(c->argv[1]+1); else jid = atoi(c->argv[1]);
        job_t *j = find_job_by_jid(jid);
        if (!j) { fprintf(stderr, "tsh: bg: job not found\n"); return 1; }
        
        j->state = JOB_RUNNING;
        kill(-j->pgid, SIGCONT);
        printf("[%d] %s\n", j->jid, j->cmdline);
        return 1; 
    }
    if (strcmp(c->argv[0], "export") == 0) {
        if (!c->argv[1]) return 1;
        char *p = strchr(c->argv[1], '=');
        if (p) {
            *p = '\0';
            setenv(c->argv[1], p+1, 1);
        }
        return 1;
    }
    if (strcmp(c->argv[0], "unset") == 0) {
        if (!c->argv[1]) return 1;
        unsetenv(c->argv[1]);
        return 1;
    }
    return 0;
}
