#ifndef _UTILITIES_H_
#define _UTILITIES_H_

// ----------------------------------------------------------------------------
// Print error from errno and exit with a failure indication
// ----------------------------------------------------------------------------
void perrorExit(const char *msg);

// ----------------------------------------------------------------------------
// Print error to stderr and exit with a failure indication
// ----------------------------------------------------------------------------
void ferrorExit(const char *msg);

#endif

