#include "shell.h"

/* Setup signal handlers */
void setupSignalHandler(void) {
    struct sigaction sa_chld, sa_int, sa_quit, sa_tstp;
    
    /* SIGCHLD handler - reap zombie processes */
    sa_chld.sa_handler = sigchildHandler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa_chld, NULL) < 0) {
        perror("sigaction SIGCHLD");
    }
    
    /* SIGINT handler (Ctrl-C) - ignore in shell */
    sa_int.sa_handler = SIG_IGN;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    if (sigaction(SIGINT, &sa_int, NULL) < 0) {
        perror("sigaction SIGINT");
    }
    
    /* SIGQUIT handler (Ctrl-\) - ignore in shell */
    sa_quit.sa_handler = SIG_IGN;
    sigemptyset(&sa_quit.sa_mask);
    sa_quit.sa_flags = 0;
    if (sigaction(SIGQUIT, &sa_quit, NULL) < 0) {
        perror("sigaction SIGQUIT");
    }
    
    /* SIGTSTP handler (Ctrl-Z) - ignore in shell */
    sa_tstp.sa_handler = SIG_IGN;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = 0;
    if (sigaction(SIGTSTP, &sa_tstp, NULL) < 0) {
        perror("sigaction SIGTSTP");
    }
}

/* SIGCHLD handler - reap zombie processes */
void sigchildHandler() {
    int more = 1;
    pid_t pid;
    int status;
    int saved_errno = errno;
    
    /* Reap all zombie children */
    while (more) {
        pid = waitpid(-1, &status, WNOHANG);
        if (pid <= 0) {
            more = 0;
        }
    }
    
    errno = saved_errno;
}


