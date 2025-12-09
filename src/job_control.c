#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include "job_control.h"

static job_t jobs[MAX_JOBS];
static int next_jid = 1;

void init_jobs(void) {
    memset(jobs, 0, sizeof(jobs));
    next_jid = 1;
}

int add_job(pid_t pgid, const char *cmdline) {
    for (int i = 0; i < MAX_JOBS; ++i) {
        if (jobs[i].pgid == 0) {
            jobs[i].pgid = pgid;
            jobs[i].jid = next_jid++;
            jobs[i].cmdline = strdup(cmdline);
            jobs[i].state = JOB_RUNNING;
            return jobs[i].jid;
        }
    }
    return -1;
}

job_t* find_job_by_jid(int jid) {
    for (int i = 0; i < MAX_JOBS; ++i) if (jobs[i].pgid != 0 && jobs[i].jid == jid) return &jobs[i];
    return NULL;
}

job_t* find_job_by_pgid(pid_t pgid) {
    for (int i = 0; i < MAX_JOBS; ++i) if (jobs[i].pgid == pgid) return &jobs[i];
    return NULL;
}

void remove_job(job_t *j) {
    if (!j) return;
    if (j->cmdline) free(j->cmdline);
    j->cmdline = NULL;
    j->pgid = 0;
    j->jid = 0;
    j->running = 0;
}

void print_jobs(void) {
    for (int i = 0; i < MAX_JOBS; ++i) {
        if (jobs[i].pgid != 0) {
            const char *state_str = "Unknown";
            switch (jobs[i].state) {
                case JOB_RUNNING: state_str = "Running"; break;
                case JOB_STOPPED: state_str = "Stopped"; break;
                case JOB_DONE:    state_str = "Done";    break;
            }
            printf("[%d] %s   %s\n", jobs[i].jid, state_str, jobs[i].cmdline);
        }
    }
}

void sigchld_handler(int sig) {
    (void)sig;
    int saved_errno = errno;
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        job_t *j = find_job_by_pgid(pid);
        if (!j) {
            for (int i = 0; i < MAX_JOBS; ++i) {
                if (jobs[i].pgid != 0) {
                    if (kill(-jobs[i].pgid, 0) == -1 && errno == ESRCH) {
                        j = &jobs[i];
                        break;
                    }
                }
            }
        }
        if (j) {
            if (WIFSTOPPED(status)) {
                j->state = JOB_STOPPED;
                char buf[512];
                int n = snprintf(buf, sizeof(buf), "\n[%d] Stopped   %s\n", j->jid, j->cmdline);
                if (n > 0) write(STDOUT_FILENO, buf, n);
            } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
                j->state = JOB_DONE;
                char buf[512];
                int n = snprintf(buf, sizeof(buf), "\n[%d] Done   %s\n", j->jid, j->cmdline);
                if (n > 0) write(STDOUT_FILENO, buf, n);
            }
        }
    }
    errno = saved_errno;
}

void free_jobs(void) {
    for (int i = 0; i < MAX_JOBS; ++i) {
        if (jobs[i].pgid != 0) {
            free(jobs[i].cmdline);
            jobs[i].cmdline = NULL;
        }
    }
}
