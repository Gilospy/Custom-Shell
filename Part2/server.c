/*
 * server.c
 *
 * Remote shell server for ICT374 Assignment (Part 2).
 *
 * Features:
 *   - TCP server listening on a configurable port (default: DEFAULT_PORT).
 *   - Simple text-based login protocol:
 *       RSH-WELCOME 1.0
 *       LOGIN-REQUIRED
 *     Client must send:
 *       USER <username>\n
 *       PASS <password>\n
 *   - Authentication using auth.c (in-memory user database),
 *     including at least username: test, password: test.
 *   - On successful authentication:
 *       AUTH-OK <username>
 *     Then the server starts a shell process and relays I/O
 *     between the client and the shell (like a minimal telnet).
 *   - Handles multiple clients concurrently using fork().
 *
 * Usage:
 *   ./server [-p port] [-s shell_path]
 *
 *   -p port       Listening TCP port (default: 50000 – change to your assigned port)
 *   -s shell_path Path to shell executable (default: /bin/bash or your Part 1 shell)
 *
 * For final submission, you will typically run:
 *   ./server -p <your_assigned_port> -s ../Part1/myshell
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/select.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#include "auth.h"
#include "protocol.h"

/* ====== Configuration defaults ====== */

/* TODO: change this to your assigned port */
#define DEFAULT_PORT   50000

/* Default shell path: for development/testing. For final, use Part 1 shell. */
#define DEFAULT_SHELL  "./myshell"

#define BACKLOG        10           /* listen queue size */
#define MAX_LINE       1024         /* maximum length of protocol line */

/* ====== Function declarations ====== */

static void usage(const char *progname);
static void parse_args(int argc, char *argv[], int *port, char *shell_path, size_t shell_size);
static int  create_listening_socket(int port);
static void install_signal_handlers(void);
static void sigchld_handler(int sig);

static void server_loop(int listen_fd, const char *shell_path);
static void handle_client(int client_fd, const struct sockaddr_in *client_addr, const char *shell_path);

static ssize_t read_line(int fd, char *buf, size_t maxlen);
static ssize_t writen(int fd, const void *buf, size_t n);

static int start_shell_session(int client_fd, const char *shell_path, const char *username);

/* ====== Main ====== */

int main(int argc, char *argv[])
{
    int port = DEFAULT_PORT;
    char shell_path[256];
    int listen_fd;

    /* Copy default shell path */
    strncpy(shell_path, DEFAULT_SHELL, sizeof(shell_path) - 1);
    shell_path[sizeof(shell_path) - 1] = '\0';

    parse_args(argc, argv, &port, shell_path, sizeof(shell_path));
    install_signal_handlers();

    /* Create TCP listening socket */
    listen_fd = create_listening_socket(port);
    if (listen_fd < 0) {
        fprintf(stderr, "Error: failed to create listening socket\n");
        exit(EXIT_FAILURE);
    }

    printf("server: listening on port %d, shell = %s\n", port, shell_path);
    fflush(stdout);

    server_loop(listen_fd, shell_path);

    close(listen_fd);
    return 0;
}

/* ====== Argument parsing & usage ====== */

static void usage(const char *progname)
{
    fprintf(stderr,
            "Usage: %s [-p port] [-s shell_path]\n"
            "  -p port       Listening TCP port (default: %d)\n"
            "  -s shell_path Path to shell executable (default: %s)\n",
            progname, DEFAULT_PORT, DEFAULT_SHELL);
}

static void parse_args(int argc, char *argv[], int *port, char *shell_path, size_t shell_size)
{
    int opt;

    opterr = 0;  /* let us control error messages */
    while ((opt = getopt(argc, argv, "p:s:h")) != -1) {
        switch (opt) {
        case 'p': {
            int p = atoi(optarg);
            if (p <= 0 || p > 65535) {
                fprintf(stderr, "Invalid port: %s\n", optarg);
                usage(argv[0]);
                exit(EXIT_FAILURE);
            }
            *port = p;
            break;
        }
        case 's':
            strncpy(shell_path, optarg, shell_size - 1);
            shell_path[shell_size - 1] = '\0';
            break;
        case 'h':
        default:
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }
}

/* ====== Signal handling ====== */

static void sigchld_handler(int sig)
{
    (void)sig;  /* unused */
    /* Reap all dead children, non-blocking */
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        /* nothing else to do */
    }
}

static void install_signal_handlers(void)
{
    struct sigaction sa;

    /* Ignore SIGPIPE so writes to closed sockets don't kill the server */
    signal(SIGPIPE, SIG_IGN);

    /* Handle SIGCHLD to avoid zombie processes */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;  /* restart system calls when possible */

    if (sigaction(SIGCHLD, &sa, NULL) < 0) {
        perror("sigaction(SIGCHLD)");
        /* not fatal, but recommended */
    }
}

/* ====== Networking: create listening socket & main loop ====== */

static int create_listening_socket(int port)
{
    int fd;
    int optval = 1;
    struct sockaddr_in addr;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("setsockopt(SO_REUSEADDR)");
        /* continue anyway */
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);   /* listen on all interfaces */
    addr.sin_port        = htons((unsigned short)port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(fd);
        return -1;
    }

    if (listen(fd, BACKLOG) < 0) {
        perror("listen");
        close(fd);
        return -1;
    }

    return fd;
}

static void server_loop(int listen_fd, const char *shell_path)
{
    for (;;) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int client_fd;
        pid_t pid;

        client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addrlen);
        if (client_fd < 0) {
            if (errno == EINTR) {
                continue; /* interrupted by signal */
            }
            perror("accept");
            continue;
        }

        pid = fork();
        if (pid < 0) {
            perror("fork");
            close(client_fd);
            continue;
        }

        if (pid == 0) {
            /* Child process: handle this client */
            close(listen_fd);
            handle_client(client_fd, &client_addr, shell_path);
            close(client_fd);
            _exit(0);
        } else {
            /* Parent process: no need for client fd */
            close(client_fd);
        }
    }
}

/* ====== Utility: robust line I/O ====== */

/*
 * read_line: reads up to maxlen-1 bytes or until '\n' (inclusive).
 * Appends '\0' at the end. Returns number of bytes read (excluding '\0'),
 * 0 on EOF, or -1 on error.
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
            return -1; /* error */
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

/* ====== Core: handle a single client (protocol + shell) ====== */

static void handle_client(int client_fd, const struct sockaddr_in *client_addr, const char *shell_path)
{
    char addr_str[INET_ADDRSTRLEN];
    char line[MAX_LINE];
    char username[128] = {0};
    char password[128] = {0};

    if (inet_ntop(AF_INET, &client_addr->sin_addr, addr_str, sizeof(addr_str)) == NULL) {
        strncpy(addr_str, "unknown", sizeof(addr_str));
        addr_str[sizeof(addr_str) - 1] = '\0';
    }

    printf("New connection from %s:%d\n",
           addr_str,
           (int)ntohs(client_addr->sin_port));
    fflush(stdout);

    /* Send greeting and login request */
    proto_send_line(client_fd, PROTO_WELCOME);
    proto_send_line(client_fd, PROTO_LOGIN_REQUIRED);

    /* ---- Read USER line ---- */
    ssize_t n = read_line(client_fd, line, sizeof(line));
    if (n <= 0) {
        /* client disconnected or error */
        return;
    }

    /* Expect: USER <username> */
    if (!PARSE_USER(line, username)) {
        proto_send_line(client_fd, "AUTH-FAIL PROTOCOL-ERROR\n");
        proto_send_line(client_fd, "GOODBYE PROTOCOL-ERROR\n");
        return;
    }

    if (!auth_user_exists(username)) {
        proto_send_line(client_fd, "AUTH-FAIL UNKNOWN-USER\n");
        proto_send_line(client_fd, "GOODBYE UNKNOWN-USER\n");
        return;
    }

    /* ---- Read PASS line ---- */
    n = read_line(client_fd, line, sizeof(line));
    if (n <= 0) {
        return;
    }

    /* Expect: PASS <password> */
    if (!PARSE_PASS(line, password)) {
        proto_send_line(client_fd, "AUTH-FAIL PROTOCOL-ERROR\n");
        proto_send_line(client_fd, "GOODBYE PROTOCOL-ERROR\n");
        return;
    }

    if (!auth_check_password(username, password)) {
        proto_send_line(client_fd, "AUTH-FAIL BAD-PASSWORD\n");
        proto_send_line(client_fd, "GOODBYE BAD-PASSWORD\n");
        return;
    }

    /* Authentication successful */
    {
        char ok_msg[256];
        snprintf(ok_msg, sizeof(ok_msg), "%s %s\n", PROTO_AUTH_OK, username);
        proto_send_line(client_fd, ok_msg);
    }

    /* Start the remote shell session */
    if (start_shell_session(client_fd, shell_path, username) != 0) {
        proto_send_line(client_fd, "GOODBYE INTERNAL-ERROR\n");
    }
}

/* ====== Shell session: pipes + select relay ====== */

static int start_shell_session(int client_fd, const char *shell_path, const char *username)
{
    int in_to_child[2];      /* server writes -> child stdin */
    int out_from_child[2];   /* child stdout/stderr -> server reads */
    pid_t pid;

    if (pipe(in_to_child) < 0) {
        perror("pipe(in_to_child)");
        return -1;
    }
    if (pipe(out_from_child) < 0) {
        perror("pipe(out_from_child)");
        close(in_to_child[0]);
        close(in_to_child[1]);
        return -1;
    }

    pid = fork();
    if (pid < 0) {
        perror("fork");
        close(in_to_child[0]);
        close(in_to_child[1]);
        close(out_from_child[0]);
        close(out_from_child[1]);
        return -1;
    }

    if (pid == 0) {
        /* ---- Child: shell process ---- */
        /* stdin: from in_to_child[0] */
        /* stdout/stderr: to out_from_child[1] */

        /* Close unused ends */
        close(in_to_child[1]);
        close(out_from_child[0]);

        if (dup2(in_to_child[0], STDIN_FILENO) < 0) {
            perror("dup2 stdin");
            _exit(1);
        }
        if (dup2(out_from_child[1], STDOUT_FILENO) < 0) {
            perror("dup2 stdout");
            _exit(1);
        }
        if (dup2(out_from_child[1], STDERR_FILENO) < 0) {
            perror("dup2 stderr");
            _exit(1);
        }

        close(in_to_child[0]);
        close(out_from_child[1]);

        /* Optionally set some environment variables */
        if (username != NULL) {
            setenv("RSH_USER", username, 1);
        }

        /* Execute the shell */
        execl(shell_path, shell_path, (char *)NULL);

        /* If execl returns, it failed */
        perror("execl shell");
        _exit(1);

    } else {
        /* ---- Parent: session relay ---- */
        fd_set rfds;
        int maxfd;
        char buf[4096];
        int child_stdin  = in_to_child[1];
        int child_stdout = out_from_child[0];
        int done = 0;

        /* Close unused ends */
        close(in_to_child[0]);
        close(out_from_child[1]);

        /*
         * Relay loop:
         *   - Read from client_fd, forward to child_stdin.
         *   - Read from child_stdout, forward to client_fd.
         */
        while (!done) {
            FD_ZERO(&rfds);
            FD_SET(client_fd, &rfds);
            FD_SET(child_stdout, &rfds);

            maxfd = (client_fd > child_stdout ? client_fd : child_stdout) + 1;

            int rc = select(maxfd, &rfds, NULL, NULL, NULL);
            if (rc < 0) {
                if (errno == EINTR)
                    continue;
                perror("select");
                break;
            }

            /* Data from client -> shell stdin */
            if (FD_ISSET(client_fd, &rfds)) {
                ssize_t n = read(client_fd, buf, sizeof(buf));
                if (n < 0) {
                    if (errno == EINTR)
                        continue;
                    perror("read from client");
                    break;
                } else if (n == 0) {
                    /* client closed connection */
                    done = 1;
                    /* Close shell stdin so it may terminate */
                    close(child_stdin);
                } else {
                    if (writen(child_stdin, buf, (size_t)n) < 0) {
                        perror("write to shell stdin");
                        done = 1;
                    }
                }
            }

            /* Data from shell stdout/stderr -> client */
            if (FD_ISSET(child_stdout, &rfds)) {
                ssize_t n = read(child_stdout, buf, sizeof(buf));
                if (n < 0) {
                    if (errno == EINTR)
                        continue;
                    perror("read from shell stdout");
                    break;
                } else if (n == 0) {
                    /* shell closed its stdout/stderr -> shell exited */
                    done = 1;
                } else {
                    if (writen(client_fd, buf, (size_t)n) < 0) {
                        /* client may have disconnected */
                        done = 1;
                    }
                }
            }
        }

        /* Cleanup */
        close(child_stdin);
        close(child_stdout);

        /* Wait for shell to exit (avoid zombie) */
        waitpid(pid, NULL, 0);

        return 0;
    }
}
