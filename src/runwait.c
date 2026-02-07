#include "common.h"
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>


static void usage(const char *a){
fprintf(stderr, "Usage: %s <cmd> [args]\n", a);
 exit(1);
}
static double d(struct timespec a, struct timespec b){
 double sec = (double)(b.tv_sec - a.tv_sec);
double nsec = (double)(b.tv_nsec - a.tv_nsec) / 1e9;
return sec + nsec;
}
int main(int c, char **v) {
if (c < 2) usage(v[0]);

struct timespec start, end;
if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
perror("clock_gettime");
return 1;
}

pid_t pid = fork();
if (pid < 0) {
perror("fork");
return 1;
}

if (pid == 0) {
execvp(v[1], &v[1]);
perror("execvp");
_exit(127);
}

int status = 0;
if (waitpid(pid, &status, 0) < 0) {
perror("waitpid");
return 1;
}

if (clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
perror("clock_gettime");
return 1;
}

double elapsed = d(start, end);


if (WIFEXITED(status)) {
int code = WEXITSTATUS(status);
printf("pid=%d elapsed=%.3f exit=%d\n", (int)pid, elapsed, code);
return code;
}

if (WIFSIGNALED(status)) {
 int sig = WTERMSIG(status);
printf("pid=%d elapsed=%.3f signal=%d\n", (int)pid, elapsed, sig);
return 128 + sig;
}

printf("pid=%d elapsed=%.3f exit=%d\n", (int)pid, elapsed, 1);
return 1;
}

