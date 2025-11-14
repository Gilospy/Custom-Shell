/*
 * client.c
 *
 * Remote shell client for ICT374 Assignment (Part 2).
 *
 * Protocol:
 *   Server sends:
 *       RSH-WELCOME 1.0
 *       LOGIN-REQUIRED
 *   Client sends:
 *       USER <username>\n
 *       PASS <password>\n
 *   Server replies:
 *       AUTH-OK <username>\n
 *   or:
 *       AUTH-FAIL <reason>\n
 *       GOODBYE <reason>\n
 *
 * After AUTH-OK, client switches to "shell mode":
 *   - Read from stdin, send to server.
 *   - Read from server, print to stdout.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "protocol.h"

/* ====== Defaults ====== */

#define DEFAULT_PORT 50000
#define DEFAULT_HOST "localhost"
#define MAX_LINE     1024

/* ====== Helper declarations ====== */

static void usage(const char *progname);
static int  connect_to_server(const char *host, int port);
static ssize_t read_line(int fd, char *buf, size_t maxlen);
static ssize_t writen(int fd, const void *buf, size_t n);
static void trim_newline(char *s);
static void shell_mode(int sockfd);

/* ====== Main ====== */

int main(int argc, char *argv[])
{
    const char *host = DEFAULT_HOST;
    int port = DEFAULT_PORT;
    int sockfd;
    char line[MAX_LINE];
    char username[128];
    char password[128];

    /* Parse command line: client [host] [port] */
    if (argc >= 2) {
        host = argv[1];
    }
    if (argc >= 3) {
        port = atoi(argv[2]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Invalid port: %s\n", argv[2]);
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    sockfd = connect_to_server(host, port);
    if (sockfd < 0) {
        fprintf(stderr, "Failed to connect to %s:%d\n", host, port);
        exit(EXIT_FAILURE);
    }

    /* --- Read greeting line --- */
    if (read_line(sockfd, line, sizeof(line)) <= 0) {
        fprintf(stderr, "Server closed connection (no greeting)\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    fputs(line, stdout);  /* Usually: RSH-WELCOME 1.0 */

    /* Optionally check it matches PROTO_WELCOME */
    if (strcmp(line, PROTO_WELCOME) != 0) {
        fprintf(stderr, "Unexpected greeting from server.\n");
        /* Not fatal; continue anyway */
    }

    /* --- Read LOGIN-REQUIRED line --- */
    if (read_line(sockfd, line, sizeof(line)) <= 0) {
        fprintf(stderr, "Server closed connection (no login prompt)\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    fputs(line, stdout);  /* Usually: LOGIN-REQUIRED */

    /* --- Prompt for username & password --- */
    printf("Username: ");
    fflush(stdout);
    if (fgets(username, sizeof(username), stdin) == NULL) {
        fprintf(stderr, "Input error\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    trim_newline(username);

    printf("Password: ");
    fflush(stdout);
    if (fgets(password, sizeof(password), stdin) == NULL) {
        fprintf(stderr, "Input error\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    trim_newline(password);
    /* Note: password is echoed visibly; hiding it is optional for assignment. */

    /* --- Send USER line --- */
    snprintf(line, sizeof(line), "%s %s\n", PROTO_CMD_USER, username);
    if (writen(sockfd, line, strlen(line)) < 0) {
        perror("write USER");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    /* --- Send PASS line --- */
    snprintf(line, sizeof(line), "%s %s\n", PROTO_CMD_PASS, password);
    if (writen(sockfd, line, strlen(line)) < 0) {
        perror("write PASS");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    /* --- Read authentication response --- */
    if (read_line(sockfd, line, sizeof(line)) <= 0) {
        fprintf(stderr, "Server closed connection after PASS\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (IS_AUTH_OK(line)) {
        /* Auth success */
        fputs(line, stdout);
        printf("Login successful, entering remote shell.\n");
        fflush(stdout);

        /* Go into shell mode (interactive) */
        shell_mode(sockfd);
        close(sockfd);
        return 0;

    } else if (IS_AUTH_FAIL(line)) {
        /* Auth failed - print reason, read GOODBYE if any, then exit */
        fputs(line, stdout);

        if (read_line(sockfd, line, sizeof(line)) > 0) {
            /* probably GOODBYE line */
            fputs(line, stdout);
        }

        close(sockfd);
        return 1;

    } else {
        /* Unexpected response */
        fprintf(stderr, "Unexpected response from server: %s", line);
        close(sockfd);
        return 1;
    }
}

/* ====== Helper functions ====== */

static void usage(const char *progname)
{
    fprintf(stderr,
            "Usage: %s [host] [port]\n"
            "  host : server hostname or IP (default: %s)\n"
            "  port : TCP port (default: %d)\n",
            progname, DEFAULT_HOST, DEFAULT_PORT);
}

/*
 * Resolve host:port and connect.
 * Returns connected socket fd on success, -1 on error.
 */
static int connect_to_server(const char *host, int port)
{
    int sockfd = -1;
    char port_str[16];
    struct addrinfo hints;
    struct addrinfo *res = NULL, *rp;
    int rc;

    snprintf(port_str, sizeof(port_str), "%d", port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;       /* IPv4 */
    hints.ai_socktype = SOCK_STREAM;   /* TCP */

    rc = getaddrinfo(host, port_str, &hints, &res);
    if (rc != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
        return -1;
    }

    for (rp = res; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd < 0)
            continue;

        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) {
            /* Connected */
            break;
        }

        close(sockfd);
        sockfd = -1;
    }

    freeaddrinfo(res);

    if (sockfd < 0) {
        perror("connect");
        return -1;
    }

    return sockfd;
}

/*
 * read_line: reads up to maxlen-1 bytes or until '\n'.
 * Appends '\0'. Returns bytes read (excluding '\0'), 0 on EOF, -1 on error.
 */
static ssize_t read_line(int fd, char *buf, size_t maxlen)
{
    size_t n = 0;

    while (n < maxlen - 1) {
        char c;
        ssize_t rc = read(fd, &c, 1);
        if (rc == 1) {
            buf[n++] = c;
            if (c == '\n')
                break;
        } else if (rc == 0) {
            /* EOF */
            break;
        } else {
            if (errno == EINTR)
                continue;
            return -1;
        }
    }

    buf[n] = '\0';
    return (ssize_t)n;
}

/*
 * writen: write exactly n bytes unless an error occurs.
 * Returns 0 on success, -1 on error.
 */
static ssize_t writen(int fd, const void *buf, size_t n)
{
    const char *p = (const char *)buf;
    size_t left = n;

    while (left > 0) {
        ssize_t rc = write(fd, p, left);
        if (rc <= 0) {
            if (rc < 0 && errno == EINTR)
                continue;
            return -1;
        }
        left -= (size_t)rc;
        p    += rc;
    }
    return 0;
}

/*
 * Remove trailing '\n' if present.
 */
static void trim_newline(char *s)
{
    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n') {
        s[len - 1] = '\0';
    }
}

/*
 * shell_mode:
 *   After AUTH-OK, relay data between stdin and server socket:
 *
 *   - stdin  -> socket
 *   - socket -> stdout
 *
 *   Uses select() to multiplex.
 */
static void shell_mode(int sockfd)
{
    fd_set rfds;
    int maxfd;
    char buf[4096];
    int done = 0;

    printf("Remote shell ready. Type commands as usual.\n");
    fflush(stdout);

    while (!done) {
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        FD_SET(sockfd, &rfds);

        maxfd = (STDIN_FILENO > sockfd ? STDIN_FILENO : sockfd) + 1;

        int rc = select(maxfd, &rfds, NULL, NULL, NULL);
        if (rc < 0) {
            if (errno == EINTR)
                continue;
            perror("select");
            break;
        }

        /* stdin -> server */
        if (FD_ISSET(STDIN_FILENO, &rfds)) {
            ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
            if (n < 0) {
                if (errno == EINTR)
                    continue;
                perror("read stdin");
                done = 1;
            } else if (n == 0) {
                /* EOF from user (Ctrl-D) */
                shutdown(sockfd, SHUT_WR);  /* half-close: signal EOF to server */
                FD_CLR(STDIN_FILENO, &rfds);
            } else {
                if (writen(sockfd, buf, (size_t)n) < 0) {
                    perror("write to server");
                    done = 1;
                }
            }
        }

        /* server -> stdout */
        if (FD_ISSET(sockfd, &rfds)) {
            ssize_t n = read(sockfd, buf, sizeof(buf));
            if (n < 0) {
                if (errno == EINTR)
                    continue;
                perror("read from server");
                done = 1;
            } else if (n == 0) {
                /* Server closed connection */
                printf("\n[Connection closed by remote host]\n");
                done = 1;
            } else {
                if (writen(STDOUT_FILENO, buf, (size_t)n) < 0) {
                    perror("write to stdout");
                    done = 1;
                }
            }
        }
    }
}
