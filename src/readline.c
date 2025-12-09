#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>
#include <dirent.h>
#include "readline.h"
#include "builtins.h"

#define MAX_NAME_LEN 1024

#define BUF_SIZE 4096

static struct termios orig_termios;
static int raw_mode_enabled = 0;

static void disable_raw_mode(void) {
    if (raw_mode_enabled) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        raw_mode_enabled = 0;
    }
}

static void enable_raw_mode(void) {
    if (!isatty(STDIN_FILENO)) return;
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) return;
    
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_iflag &= ~(IXON);
    
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) return;
    raw_mode_enabled = 1;
}

char *tsh_readline(const char *prompt) {
    if (!isatty(STDIN_FILENO)) {
        char *line = NULL;
        size_t len = 0;
        printf("%s", prompt); fflush(stdout);
        if (getline(&line, &len, stdin) == -1) { free(line); return NULL; }
        size_t L = strlen(line);
        if (L>0 && line[L-1]=='\n') line[L-1]='\0';
        return line;
    }

    char buf[BUF_SIZE];
    int pos = 0;
    int len = 0;
    buf[0] = '\0';
    
    int history_idx = get_history_length();
    char *saved_current_line = NULL;

    printf("%s", prompt); fflush(stdout);

    enable_raw_mode();

    while (1) {
        char c;
        int nread = read(STDIN_FILENO, &c, 1);
        if (nread <= 0) break;

        if (c == '\r' || c == '\n') {
            printf("\r\n");
            break;
        } else if (c == 127 || c == 8) {
            if (pos > 0) {
                if (pos < len) {
                    memmove(buf+pos-1, buf+pos, len-pos);
                }
                pos--;
                len--;
                buf[len] = '\0';
                printf("\b%s \b", buf+pos);
                for (int i=0;i<(len-pos)+1;i++) printf("\b");
                fflush(stdout);
            }
        } else if (c == '\033') {
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) == 1 && read(STDIN_FILENO, &seq[1], 1) == 1) {
                if (seq[0] == '[') {
                    if (seq[1] == 'A') {
                         if (history_idx > 0) {
                             if (history_idx == get_history_length()) {
                                 if (saved_current_line) free(saved_current_line);
                                 buf[len] = '\0';
                                 saved_current_line = strdup(buf);
                             }
                             history_idx--;
                             const char *h = get_history_item(history_idx);
                             if (h) {
                                 while (pos > 0) { printf("\b \b"); pos--; }
                                 strncpy(buf, h, BUF_SIZE-1); buf[BUF_SIZE-1] = '\0';
                                 len = strlen(buf);
                                 pos = len;
                                 pos = len;
                                 printf("%s", buf);
                                 fflush(stdout);
                             }
                         }
                    } else if (seq[1] == 'B') {
                        if (history_idx < get_history_length()) {
                            history_idx++;
                            const char *h = NULL;
                            if (history_idx == get_history_length()) {
                                h = saved_current_line ? saved_current_line : "";
                            } else {
                                h = get_history_item(history_idx);
                            }
                            if (h) {
                                while (pos > 0) { printf("\b \b"); pos--; }
                                strncpy(buf, h, BUF_SIZE-1); buf[BUF_SIZE-1] = '\0';
                                len = strlen(buf);
                                pos = len;
                                pos = len;
                                printf("%s", buf);
                                fflush(stdout);
                            }
                        }
                    }
                }
            }
        } else if (c == 3) {
            printf("^C\r\n");
            fflush(stdout);
            buf[0] = '\0';
            len = 0;
            pos = 0;
            break;
        } else if (c == 4) {
             if (len == 0) {
                 disable_raw_mode();
                 if (saved_current_line) free(saved_current_line);
                 return NULL;
             }
        } else if (c == '\t') { // Tab Completion
             // Identify word prefix
             int start = pos;
             while (start > 0 && !isspace((unsigned char)buf[start-1])) start--;
             
             int prefix_len = pos - start;
             if (prefix_len > 0) {
                 char prefix[MAX_NAME_LEN];
                 if (prefix_len >= MAX_NAME_LEN) prefix_len = MAX_NAME_LEN - 1;
                 strncpy(prefix, buf + start, prefix_len);
                 prefix[prefix_len] = '\0';
                 
                 // Scan directory
                 DIR *d = opendir(".");
                 if (d) {
                     struct dirent *dir;
                     char match[MAX_NAME_LEN];
                     int match_count = 0;
                     
                     while ((dir = readdir(d)) != NULL) {
                         if (strncmp(dir->d_name, prefix, prefix_len) == 0) {
                             // Match found
                             if (match_count == 0) strncpy(match, dir->d_name, MAX_NAME_LEN);
                             match_count++;
                         }
                     }
                     closedir(d);
                     
                     if (match_count == 1) {
                         // Unique match, complete it
                         int suffix_len = strlen(match) - prefix_len;
                         if (len + suffix_len < BUF_SIZE - 1) {
                             if (pos < len) {
                                 memmove(buf+pos+suffix_len, buf+pos, len-pos);
                             }
                             strncpy(buf + pos, match + prefix_len, suffix_len);
                             len += suffix_len;
                             pos += suffix_len;
                             buf[len] = '\0';
                             printf("%s", match + prefix_len);
                             // If we were in middle, we need to reprint rest?
                             // Yes if pos < len we moved rest.
                             if (pos < len) {
                                 printf("%s", buf + pos);
                                 for(int i=0;i<(len-pos);i++) printf("\b");
                             }
                             fflush(stdout);
                         }
                     } else if (match_count > 1) {
                         // Optional: could list matches, but request said "If exactly one match is found... append"
                         // Maybe ring bell?
                         // printf("\a");
                     }
                 }
             }
        } else if (!iscntrl(c)) {
             if (len < BUF_SIZE - 1) {
                 if (pos < len) {
                     memmove(buf+pos+1, buf+pos, len-pos);
                 }
                 buf[pos] = c;
                 len++;
                 pos++;
                 buf[len] = '\0';
                 printf("%c%s", c, buf+pos);
                 for (int i=0;i<(len-pos);i++) printf("\b");
                 fflush(stdout);
             }
        }
    }

    disable_raw_mode();
    if (saved_current_line) free(saved_current_line);
    buf[len] = '\0';
    return strdup(buf);
}
