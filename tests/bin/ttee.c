#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define PERROR_FMT(fmt, ...) \
    fprintf(stderr, "error: " fmt ": %s\n", __VA_ARGS__, strerror(errno))

static int open_term_pair(int *masterfd, int *slavefd)
{
    int ptmfd;
    if ((ptmfd = posix_openpt(O_RDWR|O_NOCTTY)) == -1) {
        perror("posix_openpt");
        return -1;
    }
    if (grantpt(ptmfd) == -1) {
        perror("grantpt");
        return -1;
    }
    if (unlockpt(ptmfd) == -1) {
        perror("unlockpt");
        return -1;
    }
    char *ptsn;
    if ((ptsn = ptsname(ptmfd)) == NULL) {
        perror("ptsname");
        return -1;
    }
    int ptsfd;
    if ((ptsfd = open(ptsn, O_RDWR|O_NOCTTY)) == -1) {
        perror("open(ptsn)");
        return -1;
    }
    *masterfd = ptmfd;
    *slavefd = ptsfd;
    return 0;
}

// cfmakeraw is not POSIX, but termios(3) mentions it does this:
static void set_raw_mode(struct termios *tos)
{
    tos->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                      | INLCR | IGNCR | ICRNL | IXON);
    tos->c_oflag &= ~OPOST;
    tos->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tos->c_cflag &= ~(CSIZE | PARENB);
    tos->c_cflag |= CS8;
}

static ssize_t write_safe(int fd, const void *buf, size_t count)
{
    ssize_t rc, written = 0;
    while ((size_t)written < count) {
        if ((rc = write(fd, buf, count)) == -1) {
            if (errno == EINTR)
                continue;
            perror("write");
            return -1;
        }
        written += rc;
    }
    return written;
}

static void print_help(void)
{
    fprintf(stderr,
            "usage: ttee [options] prog [progarg]...\n"
            "Runs prog in a new pseudoterminal, relaying its output to ttee's\n"
            "stdout and, optionally, duplicating it a log file.\n"
            "\n"
            "  -f   log file name\n"
            "  -a   append to log file, do not truncate\n");
}

int main(int argc, char **argv)
{
    char *logfile = NULL;
    int logflags = O_WRONLY | O_CREAT | O_TRUNC;

    // '+' and '-' are unfortunately GNU extensions to getopt(3)
    int posixly_correct_set = 0;
    if (!getenv("POSIXLY_CORRECT")) {
        if (setenv("POSIXLY_CORRECT", "1", 0) == -1) {
            perror("setenv");
            return 1;
        }
        posixly_correct_set = 1;
    }
    int c;
    while ((c = getopt(argc, argv, "f:ah")) != -1) {
        switch (c) {
            case 'f':
                logfile = optarg;
                break;
            case 'a':
                logflags |= O_APPEND;
                logflags &= ~O_TRUNC;
                break;
            case 'h':
                print_help();
                return 0;
            case '?':
                return 1;
        }
    }
    if (posixly_correct_set) {
        if (unsetenv("POSIXLY_CORRECT") == -1) {
            perror("unsetenv");
            return 1;
        }
    }

    argc -= optind;
    argv += optind;

    if (argc < 1) {
        print_help();
        return 1;
    }

    int logfd = -1;
    if (logfile) {
        if ((logfd = open(logfile, logflags, 0644)) == -1) {
            PERROR_FMT("open: %s", logfile);
            goto err;
        }
    }

    int ptm = -1, pts = -1;
    if (open_term_pair(&ptm, &pts) == -1)
        goto err;

    struct termios tos;
    if (tcgetattr(pts, &tos) == -1) {
        perror("tcgetattr");
        goto err;
    }
    set_raw_mode(&tos);
    // this is not fully reliable, but good enough
    if (tcsetattr(pts, TCSANOW, &tos) == -1) {
        perror("tcgetattr");
        goto err;
    }

    switch (fork()) {
        case -1:
            perror("fork");
            goto err;
        case 0:
            if (close(ptm) == -1) {
                perror("close(ptm)");
                goto err;
            }
            ptm = -1;
            if (logfd != -1) {
                if (close(logfd) == -1) {
                    perror("close(logfd)");
                    goto err;
                }
                logfd = -1;
            }
            close(0);
            close(1);
            if (dup2(pts, 0) == -1) {
                perror("dup2(pts,0)");
                goto err;
            }
            if (dup2(pts, 1) == -1) {
                perror("dup2(pts,1)");
                goto err;
            }
            close(2);
            dup2(pts, 2);
            close(pts);
            pts = -1;
            if (execvp(argv[0], argv) == -1)
                PERROR_FMT("exec: %s", argv[0]);
            return 1;
        default:
            if (close(pts) == -1) {
                perror("close(pts)");
                goto err;
            }
            pts = -1;
            break;
    }

    ssize_t bytes;
    char buf[64];
    while ((bytes = read(ptm, buf, sizeof(buf))) > 0) {
        if (write_safe(1, buf, bytes) == -1)
            goto err;
        if (logfd != -1) {
            if (write_safe(logfd, buf, bytes) == -1)
                goto err;
        }
    }

    int wstatus;
    if (wait(&wstatus) == -1) {
        perror("wait");
        goto err;
    }
    int rc = WEXITSTATUS(wstatus);

    close(logfd);
    close(ptm);
    return rc;

err:
    close(logfd);
    close(ptm);
    close(pts);
    return 1;
}
