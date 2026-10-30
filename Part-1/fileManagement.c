#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAXLINE 1024

/*
 * printf always writes to fd 1 and scanf always reads from fd 0. To write a
 * file with printf we point fd 1 at it with dup2, and to read a file with
 * scanf we point fd 0 at it. stdio keeps its own buffer above the fd, so that
 * buffer must be empty whenever an fd is switched, or text meant for the
 * terminal ends up in the file (and the other way round):
 *   stdout: fflush before switching to the file and before switching back.
 *   stdin:  unbuffered (set in main), so scanf never reads ahead.
 */

static void print_error(const char *what) {
    printf("error: %s: %s\n", what, strerror(errno));
}

/* point fd 1 at path. returns a copy of the old fd 1 for stdout_restore,
   or -1 on error. */
static int stdout_to(const char *path) {
    int fd = open(path, O_WRONLY | O_APPEND);
    if (fd < 0) {
        print_error(path);
        return -1;
    }
    fflush(stdout);
    int saved = dup(1);
    if (saved < 0 || dup2(fd, 1) < 0) {
        print_error("dup2");
        if (saved >= 0)
            close(saved);
        close(fd);
        return -1;
    }
    close(fd);
    return saved;
}

static void stdout_restore(int saved) {
    fflush(stdout);
    dup2(saved, 1);
    close(saved);
}

/* point fd 0 at path. returns a copy of the old fd 0 for stdin_restore,
   or -1 on error. */
static int stdin_from(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        print_error(path);
        return -1;
    }
    int saved = dup(0);
    if (saved < 0 || dup2(fd, 0) < 0) {
        print_error("dup2");
        if (saved >= 0)
            close(saved);
        close(fd);
        return -1;
    }
    close(fd);
    return saved;
}

static void stdin_restore(int saved) {
    dup2(saved, 0);
    close(saved);
    clearerr(stdin); /* reading the file to its end set the EOF flag */
}

/* read one line from stdin into buf, without the newline.
   returns 0 at end of input. */
static int read_line(char *buf) {
    buf[0] = '\0'; /* an empty line matches nothing and leaves buf as is */
    if (scanf("%1024[^\n]", buf) == EOF)
        return 0;
    /* consume the newline */
    if (scanf("%*c") == EOF) {
        /* the last line of the file had no newline, nothing to consume */
    }
    return 1;
}

/* print lines from..to-1 (counting from 0) of path. returns the number of
   lines in the file, or -1 if it could not be opened. */
static int print_lines(const char *path, int from, int to) {
    int saved = stdin_from(path);
    if (saved < 0)
        return -1;
    char line[MAXLINE + 1];
    int count = 0;
    while (read_line(line)) {
        if (count >= from && count < to)
            printf("%s\n", line);
        count++;
    }
    stdin_restore(saved);
    return count;
}

static void print_last(const char *path, int n) {
    int total = print_lines(path, 0, 0); /* first pass only counts */
    if (total > 0)
        print_lines(path, total - n, total);
}

static void append_line(const char *path, const char *text) {
    int saved = stdout_to(path);
    if (saved < 0)
        return;
    printf("%s\n", text);
    stdout_restore(saved);
}

/* log "cmd", or "cmd n" when n >= 0 */
static void add_log(const char *cmd, int n) {
    int saved = stdout_to("logs.txt");
    if (saved < 0)
        return;
    if (n < 0)
        printf("%s\n", cmd);
    else
        printf("%s %d\n", cmd, n);
    stdout_restore(saved);
}

static int create_empty(const char *path) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        print_error(path);
        return 0;
    }
    close(fd);
    return 1;
}

/* split a command line into its first word and an optional number (-1 when
   absent). returns 0 for a blank line (*word is NULL), more than two words,
   or a second word that is not a non-negative integer. */
static int parse(char *line, char **word, int *n) {
    const char *sep = " \t\r";
    *word = strtok(line, sep);
    if (*word == NULL)
        return 0;
    char *arg = strtok(NULL, sep);
    if (strtok(NULL, sep) != NULL)
        return 0;
    *n = -1;
    if (arg != NULL) {
        char *end;
        errno = 0;
        long v = strtol(arg, &end, 10);
        if (*end != '\0' || errno != 0 || v < 0 || v > INT_MAX)
            return 0;
        *n = (int)v;
    }
    return 1;
}

int main(void) {
    setvbuf(stdin, NULL, _IONBF, 0);

    char dir[32];
    snprintf(dir, sizeof dir, "folder_%d", (int)getpid());
    if (mkdir(dir, 0755) < 0 && errno != EEXIST) {
        print_error(dir);
        return 1;
    }
    if (chdir(dir) < 0) {
        print_error(dir);
        return 1;
    }
    if (!create_empty("content.txt") || !create_empty("logs.txt"))
        return 1;

    char line[MAXLINE + 1], text[MAXLINE + 1];
    char *word;
    int n;
    for (;;) {
        printf("Enter command:");
        fflush(stdout);
        if (!read_line(line))
            break; /* input ended without STOP */
        if (!parse(line, &word, &n)) {
            if (word != NULL)
                printf("Unknown command.\n");
            continue;
        }

        if (strcmp(word, "INPUT") == 0 && n < 0) {
            printf("INPUT -> ");
            fflush(stdout);
            if (!read_line(text))
                break;
            append_line("content.txt", text);
            add_log("INPUT", -1);
        } else if (strcmp(word, "PRINT") == 0 && n < 0) {
            add_log("PRINT", -1);
            print_lines("content.txt", 0, INT_MAX);
        } else if (strcmp(word, "FIRST") == 0 && n >= 0) {
            add_log("FIRST", n);
            print_lines("content.txt", 0, n);
        } else if (strcmp(word, "LAST") == 0 && n >= 0) {
            add_log("LAST", n);
            print_last("content.txt", n);
        } else if (strcmp(word, "LOG") == 0 && n >= 0) {
            print_last("logs.txt", n);
        } else if (strcmp(word, "STOP") == 0 && n < 0) {
            add_log("STOP", -1);
            break;
        } else {
            printf("Unknown command.\n");
        }
    }
    return 0;
}
