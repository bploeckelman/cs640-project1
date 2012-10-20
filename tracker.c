#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tracker.h"
#include "utilities.h"

#define TRACKER "tracker.txt"
#define TOK_PER_LINE 4

enum token { FILENAME, ID, SENDER_HOSTNAME, SENDER_PORT };


// ----------------------------------------------------------------------------
// Parse the tracker file.
//   Builds a file_info struct for the specified file consisting of 
//   a linked list of file_part structures that contain the location 
//   and sequence information from the tracker for the specified file.
// ----------------------------------------------------------------------------
struct file_info *parseTracker(const char *filename) {
    if (filename == NULL) ferrorExit("ParseTracker: invalid filename");

    // Setup the file_info array
    struct file_info *info = malloc(sizeof(struct file_info));
    bzero(info, sizeof(struct file_info));
    info->filename = strdup(filename);

    // Open the tracker file
    FILE *file = fopen(TRACKER, "r");
    if (file == NULL) perrorExit("Tracker open error");
    else              puts("\nTracker file opened.");

    // Read in a line at a time from the tracker file
    char *line = NULL;
    size_t lineLen = 0;
    size_t bytesRead = getline(&line, &lineLen, file);
    if (bytesRead == -1) perrorExit("Getline error");
    while (bytesRead != -1) { 
        // Tokenize line
        int n = 0;
        char *tokens[TOK_PER_LINE];
        char *tok = strtok(line, " ");
        while (tok != NULL) {
            tokens[n++] = tok;
            tok  = strtok(NULL, " ");
        }

        // Only process this line if it is for the specified filename
        if (strcmp(tokens[FILENAME], filename) == 0) {
            // Create a new file_part structure 
            struct file_part *part = malloc(sizeof(struct file_part));
            bzero(part, sizeof(struct file_part));
            part->id              = atoi(tokens[ID]);
            part->sender_port     = atoi(tokens[SENDER_PORT]);
            part->sender_hostname = strdup(tokens[SENDER_HOSTNAME]);

            // Link it in to the list of file_parts for the file_info
            if (info->parts == NULL) {
                // Make this the start of a new list of file parts
                info->parts = part;
            } else {
                // Add it to the list for this file_info
                // TODO: link in proper position, ordered by part->id
                struct file_part *p = info->parts;
                while (p->next_part != NULL) p = p->next_part;
                p->next_part = part;
            }
        }

        // Get the next tracker line
        free(line);
        line = NULL;
        bytesRead = getline(&line, &lineLen, file);
    }
    free(line);
    printf("Tracker file parsed for file \"%s\"\n", filename);

    // Close the tracker file
    if (fclose(file) != 0) perrorExit("Tracker close error");
    else                   puts("Tracker file closed.\n");

    printFileInfo(info);
    return info; // success
}

// ----------------------------------------------------------------------------
void printFileInfo(struct file_info *info) {
    if (info == NULL) {
        fprintf(stderr, "Cannot print null file_info.\n");
        return;
    }

    printf("file_info for \"%s\" [%p]:\n", info->filename, info);
    printf("--------------------------------------\n");
    struct file_part *part = info->parts;
    while (part != NULL) {
        printFilePartInfo(part);
        part = part->next_part;
    }

    puts("");
}

// ----------------------------------------------------------------------------
void printFilePartInfo(struct file_part *part) {
    if (part == NULL) {
        fprintf(stderr, "Cannot print null file_part info.\n");
        return;
    }

    printf("  file_part info   : [%p]\n", part);
    printf("    id             : %d\n", part->id);
    printf("    sender hostname: %s\n", part->sender_hostname);
    printf("    sender port    : %d\n", part->sender_port);
    printf("    next part      : [%p]\n", part->next_part);
}

// ----------------------------------------------------------------------------
void freeFileInfo(struct file_info *info) {
    if (info == NULL) return;

    // Walk the parts list, freeing each part
    struct file_part *part = info->parts;
    while (part != NULL) {
        struct file_part *p = part;
        part = part->next_part;
        free(p->sender_hostname);
        free(p);
    }

    free(info->filename);
    free(info);
}

