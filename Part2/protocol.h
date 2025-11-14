/*
 * protocol.h
 *
 * Protocol constants and helper definitions for the
 * ICT374 Remote Shell (Part 2) assignment.
 *
 * This header defines all protocol messages exchanged
 * between the client and the server.
 *
 * NOTE:
 *   No protocol.c file is required. All protocol elements
 *   are defined here as macros or static inline helpers.
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

/* ============================
 *  Protocol Messages
 * ============================ */

/* Greeting */
#define PROTO_WELCOME        "RSH-WELCOME 1.0\n"

/* Server requests login */
#define PROTO_LOGIN_REQUIRED "LOGIN-REQUIRED\n"

/* Authentication status */
#define PROTO_AUTH_OK        "AUTH-OK"
#define PROTO_AUTH_FAIL      "AUTH-FAIL"

/* Goodbye message */
#define PROTO_GOODBYE        "GOODBYE"

/* ============================
 *  Protocol helper macros
 * ============================ */

/*
 * Check if the line starts with AUTH-OK
 * e.g., "AUTH-OK test\n"
 */
#define IS_AUTH_OK(line)     (strncmp((line), PROTO_AUTH_OK, strlen(PROTO_AUTH_OK)) == 0)

/*
 * Check if the line starts with AUTH-FAIL
 */
#define IS_AUTH_FAIL(line)   (strncmp((line), PROTO_AUTH_FAIL, strlen(PROTO_AUTH_FAIL)) == 0)

/*
 * Check if the line starts with GOODBYE
 */
#define IS_GOODBYE(line)     (strncmp((line), PROTO_GOODBYE, strlen(PROTO_GOODBYE)) == 0)

/* ============================
 *  Client → Server Commands
 * ============================ */

/*
 * USER <username>\n
 * PASS <password>\n
 */
#define PROTO_CMD_USER       "USER"
#define PROTO_CMD_PASS       "PASS"

/* Helpers: parse "USER <name>" or "PASS <pw>" */
#define PARSE_USER(line, userbuf)  (sscanf((line), "USER %127s",  (userbuf)) == 1)
#define PARSE_PASS(line, passbuf)  (sscanf((line), "PASS %127s",  (passbuf)) == 1)

/* ============================
 *  Inline helpers (optional)
 * ============================ */

#include <unistd.h>
#include <string.h>

/* 
 * proto_send_line:
 *   Convenience wrapper around write()
 *   Does NOT append newline automatically.
 */
static inline int proto_send_line(int fd, const char *line)
{
    size_t len = strlen(line);
    return (write(fd, line, len) == (ssize_t)len) ? 0 : -1;
}

#endif  /* PROTOCOL_H */
