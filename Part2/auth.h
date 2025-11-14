/*
 * auth.h
 *
 * Simple authentication module for ICT374 Remote Shell (Part 2).
 *
 * This module provides a minimal in-memory user database and
 * functions to validate usernames and passwords.
 *
 * Required by assignment:
 *   - Must include at least one account with:
 *       username: test
 *       password: test
 */

#ifndef AUTH_H
#define AUTH_H

/* 
 * Returns 1 if the username exists in the user database, 0 otherwise.
 */
int auth_user_exists(const char *username);

/*
 * Returns 1 if the username/password combination is valid, 0 otherwise.
 */
int auth_check_password(const char *username, const char *password);

#endif /* AUTH_H */
