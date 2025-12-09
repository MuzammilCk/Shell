#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include "parser.h"
#include "job_control.h"
#include "builtins.h"
#include "readline.h"
#include <ctype.h>

static volatile pid_t fg_pgid = 0;
static int last_exit_status = 0;

void sigint_handler(int sig) {
    (void)sig;
    if (fg_pgid > 0) {
        kill(-fg_pgid, SIGINT);
    }
}

void sigtstp_handler(int sig) {
    (void)sig;
    if (fg_pgid > 0) {
        kill(-fg_pgid, SIGTSTP);
    }
}

static void execute_pipeline(command_t cmds[], int ncmds, int background, char *origline) {
    sigset_t mask, prev_mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &prev_mask);

    int pipes[2*(MAX_CMDS)];
    for (int i=0;i<ncmds-1;i++) if (pipe(pipes + i*2) < 0) { perror("pipe"); sigprocmask(SIG_SETMASK, &prev_mask, NULL); return; }

    pid_t pgid = 0;

    for (int i=0;i<ncmds;i++) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); return; }
        if (pid == 0) {
            signal(SIGINT, SIG_DFL);
            signal(SIGCHLD, SIG_DFL);
            sigprocmask(SIG_SETMASK, &prev_mask, NULL); // Unblock signals in child
            if (pgid == 0) pgid = getpid();
            setpgid(0, pgid);

            if (i > 0) {
                if (dup2(pipes[(i-1)*2], STDIN_FILENO) < 0) { perror("dup2"); _exit(1); }
            }
            if (i < ncmds-1) {
                if (dup2(pipes[i*2 + 1], STDOUT_FILENO) < 0) { perror("dup2"); _exit(1); }
            }

            for (int j=0;j<2*(ncmds-1);j++) close(pipes[j]);

            if (cmds[i].infile) {
                int fd = open(cmds[i].infile, O_RDONLY);
                if (fd < 0) { perror("tsh: input redir"); _exit(1); }
                dup2(fd, STDIN_FILENO); close(fd);
            }
            if (cmds[i].outfile) {
                int flags = O_WRONLY | O_CREAT | (cmds[i].append ? O_APPEND : O_TRUNC);
                int fd = open(cmds[i].outfile, flags, 0644);
                if (fd < 0) { perror("tsh: output redir"); _exit(1); }
                dup2(fd, STDOUT_FILENO); close(fd);
            }

            execvp(cmds[i].argv[0], cmds[i].argv);
            fprintf(stderr, "tsh: %s: %s\n", cmds[i].argv[0], strerror(errno));
            _exit(127);
        }
        if (pgid == 0) pgid = pid;
        setpgid(pid, pgid);
    }

    for (int j=0;j<2*(ncmds-1);j++) close(pipes[j]);

    if (background) {
        int jid = add_job(pgid, origline);
        if (jid < 0) fprintf(stderr, "tsh: cannot add job\n");
        else printf("[%d] %d\n", jid, pgid);
        sigprocmask(SIG_SETMASK, &prev_mask, NULL); // Unblock
    } else {
        fg_pgid = pgid;
        int status;
        
        // SIGCHLD is blocked, so waitpid will see the changes, preventing race with handler
        while (waitpid(-pgid, &status, WUNTRACED) > 0) {
            if (WIFSTOPPED(status)) {
                printf("\n");
                int jid = add_job(pgid, origline);
                job_t *j = find_job_by_jid(jid);
                if (j) {
                    j->state = JOB_STOPPED;
                    printf("[%d] Stopped   %s\n", jid, origline);
                }
                break;
            }
            if (WIFEXITED(status)) {
                last_exit_status = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                last_exit_status = 128 + WTERMSIG(status);
            }
        }
        fg_pgid = 0;
        sigprocmask(SIG_SETMASK, &prev_mask, NULL); // Unblock
    }
}

static char* expand_variables(char *input) {
    static char buf[MAX_LINE];
    char *p = input;
    char *q = buf;
    while (*p && (q - buf < MAX_LINE - 100)) { // Safety margin
        if (*p == '$') {
            p++; // skip '$'
            if (*p == '?') {
                p++;
                q += sprintf(q, "%d", last_exit_status);
            } else if (isalpha((unsigned char)*p) || *p == '_') {
                char varname[256];
                int v = 0;
                while (*p && (isalnum((unsigned char)*p) || *p == '_') && v < 255) {
                    varname[v++] = *p++;
                }
                varname[v] = '\0';
                char *val = getenv(varname);
                if (val) {
                    // Copy value ensuring no overflow
                    while (*val && (q - buf < MAX_LINE - 1)) *q++ = *val++;
                }
            } else {
                *q++ = '$'; // Not a var, keep $
            }
        } else {
            *q++ = *p++;
        }
    }
    *q = '\0';
    return buf;
}

void run_command(char *input) {
    char *expanded = expand_variables(input);

    char *line = strdup(expanded);
    if (!line) return;
    size_t L = strlen(line); if (L>0 && line[L-1]=='\n') line[L-1]='\0';
    if (line[0] == '\0') { free(line); return; }

    add_history(input);

    command_t cmds[MAX_CMDS];
    int background = 0;
    int ncmds = parse_line(line, cmds, &background);
    if (ncmds <= 0) { free(line); return; }

    if (ncmds == 1 && is_builtin(&cmds[0])) {
        handle_builtin(&cmds[0]);
        free(line);
        return;
    }

    execute_pipeline(cmds, ncmds, background, line);

    if (!background) free(line);
}

int main(void) {
    init_jobs();

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);

    signal(SIGINT, sigint_handler);
    signal(SIGTSTP, sigtstp_handler);

    char *input = NULL;

    while (1) {
        char host[256];
        if (gethostname(host, sizeof(host)) != 0) strcpy(host, "unknown");
        char cwd[4096];
        if (!getcwd(cwd, sizeof(cwd))) strcpy(cwd, "?");
        
        char *user = getenv("USER");
        if (!user) user = "user";

        char *path = cwd;
        char *home = getenv("HOME");
        if (home && strncmp(cwd, home, strlen(home)) == 0) {
            path = cwd + strlen(home);
            if (*path == '\0') path = "~";
        }
        char prompt[8192];
        char display_path[4096];
        if (home && strncmp(cwd, home, strlen(home)) == 0) {
             snprintf(display_path, sizeof(display_path), "~%s", cwd + strlen(home));
        } else {
             strncpy(display_path, cwd, sizeof(display_path));
        }
        
        snprintf(prompt, sizeof(prompt), "\033[1;32m%s@%s\033[0m:\033[1;34m%s\033[0m$ ", user, host, display_path);

        input = tsh_readline(prompt);
        if (!input) {
            printf("\n");
            break; 
        }
        
        run_command(input);
        free(input);
    }

    free_history();
    free_jobs();
    return 0;
}