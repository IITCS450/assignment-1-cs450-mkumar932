#include "common.h"
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static void usage(const char *a) {
 fprintf(stderr, "Usage: %s <pid>\n", a);
 exit(1);
}

static int isnum(const char *s) {
if (!s || *s == '\0') return 0;
for (; *s; s++) {
 if (!isdigit((unsigned char)*s))  return 0;
}
 return 1;
}

int main(int c, char **v) {
 if (c != 2 || !isnum(v[1])) usage(v[0]);

char path[512];
char buffer[4096];

snprintf(path, sizeof(path), "/proc/%s/stat", v[1]);
FILE *fp = fopen(path, "r");
if (!fp) {
if (errno == ENOENT) {
fprintf(stderr, "Error: PID %s not found\n", v[1]);
return 1;
} else if (errno == EACCES) {
fprintf(stderr, "Error: Permission denied\n");
return 1;
} else {
 perror("Could not open stat file");
return 1;
}
}
if (!fgets(buffer, sizeof(buffer), fp)) {
fclose(fp);
fprintf(stderr, "Error: Failed to read stat\n");
return 1;
}
fclose(fp);

char *last_paren = strrchr(buffer, ')');
if (!last_paren || !last_paren[1]) {
fprintf(stderr, "Error: BAD stat format\n");
return 1;
}


char *saveptr = NULL;
char *token = strtok_r(last_paren + 2, " \t\n", &saveptr);

int pos = 1;
char p_state = '?';
int p_ppid = -1;
unsigned long long u_ticks = 0, s_ticks =0;

while (token) {
if (pos == 1) p_state = token[0];
else if (pos == 2) p_ppid = atoi(token);
else if (pos == 12) u_ticks = strtoull(token, NULL, 10);
else if (pos == 13) s_ticks = strtoull(token, NULL, 10);

token = strtok_r(NULL, " \t\n", &saveptr);
pos++;

}

long clk_hz = sysconf(_SC_CLK_TCK);
if (clk_hz <= 0) clk_hz = 100;
double cpu_seconds = (double)(u_ticks + s_ticks) / (double)clk_hz;

long mem_rss = -1;
snprintf(path, sizeof(path), "/proc/%s/status", v[1]);
fp = fopen(path, "r");
if (fp) {
while (fgets(buffer, sizeof(buffer), fp)) {
if (strncmp(buffer, "VmRss:", 6) == 0) {
sscanf(buffer + 6, "%ld", &mem_rss);
break;
}
}
fclose(fp);
}

char full_cmd[1024] = "N/A";
snprintf(path, sizeof(path), "/proc/%s/cmdline", v[1]);
fp = fopen(path, "r");
if (fp) {
size_t n = fread(full_cmd, 1, sizeof(full_cmd) -1, fp);
fclose(fp);
if (n > 0) {
for (size_t i =0; i < n; i++) {
if (full_cmd[i] == '\0') full_cmd[i] = ' ';
}
full_cmd[n] = '\0';
size_t len = strlen(full_cmd);
while (len > 0 && isspace((unsigned char)full_cmd[len - 1])) {
full_cmd[len - 1] = '\0';
len--;
}

if (len == 0) strcpy(full_cmd, "N/A");
}

}

printf("PID:%s\n", v[1]);
printf("State:%c\n", p_state);
printf("PPID:%d\n", p_ppid);
printf("Cmd:%s\n", full_cmd);
printf("CPU:%llu %.3f\n", (u_ticks + s_ticks), cpu_seconds);

if (mem_rss >= 0) printf("VmRSS:%ld\n", mem_rss);
else printf("VmRss:N/A\n");

return 0;
}


