/*
 * auth.c
 *
 * Implementation of a simple in-memory authentication system
 * for the ICT374 Remote Shell (Part 2) assignment.
 *
 * The database is intentionally minimal and hard-coded, because:
 *   - Persistent storage is not required by the specification.
 *   - The assignment explicitly allows plaintext passwords.
 *
 * You can extend the user list if you want more accounts.
 */

#include "auth.h"
#include <string.h>

/* 
 * In-memory user record.
 */
typedef struct {
    const char *username;
    const char *password;
} AuthUser;

/*
 * User database.
 *
 * IMPORTANT:
 *   The assignment requires at least the following account:
 *     username: test
 *     password: test
 *
 * You can safely add more entries if needed.
 */
static AuthUser user_db[] = {
    { "test",  "test"  },   /* required test account */
    { "user1", "pass1" },   /* optional extra account */
    { NULL,    NULL    }    /* terminator */
};

/*
 * auth_user_exists:
 *   Returns 1 if the given username exists, 0 otherwise.
 */
int auth_user_exists(const char *username)
{
    int i;

    if (username == NULL) {
        return 0;
    }

    for (i = 0; user_db[i].username != NULL; i++) {
        if (strcmp(user_db[i].username, username) == 0) {
            return 1;
        }
    }

    return 0;
}

/*
 * auth_check_password:
 *   Returns 1 if (username, password) is a valid combination, 0 otherwise.
 */
int auth_check_password(const char *username, const char *password)
{
    int i;

    if (username == NULL || password == NULL) {
        return 0;
    }

    for (i = 0; user_db[i].username != NULL; i++) {
        if (strcmp(user_db[i].username, username) == 0 &&
            strcmp(user_db[i].password, password) == 0) {
            return 1;
        }
    }

    return 0;
}
