#ifndef _UTILITIES_H_
#define _UTILITIES_H_

#include <stdio.h>
#include <stdlib.h>


/* perrorExit
 * ----------
 * Calls perror with the specified msg string,
 * then exits the program with a failure indication.
 */
void perrorExit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

/* ferrorExit
 * ----------
 * Calls fputs(msg, stderr) with the specified msg string,
 * then exits the program with a failure indication.
 */
void ferrorExit(const char *msg) {
    fputs(msg, stderr);
    exit(EXIT_FAILURE);
}

#endif

