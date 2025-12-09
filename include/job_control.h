#ifndef JOB_CONTROL_H
#define JOB_CONTROL_H

#include <sys/types.h>

#define MAX_JOBS 128

typedef enum {
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE
} job_state_t;

typedef struct job {
    pid_t pgid;
    int jid;
    char *cmdline;
    job_state_t state;
} job_t;

void init_jobs(void);
int add_job(pid_t pgid, const char *cmdline);
job_t* find_job_by_jid(int jid);
job_t* find_job_by_pgid(pid_t pgid);
void remove_job(job_t *j);
void print_jobs(void);
void sigchld_handler(int sig);
void free_jobs(void);

#endif
