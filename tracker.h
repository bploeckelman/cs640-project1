#ifndef _TRACKER_H_
#define _TRACKER_H_

#define TRACKER "tracker.txt"
#define LINE_LEN 1024
#define TOK_PER_LINE 4


enum token { FILENAME, ID, SENDER_HOSTNAME, SENDER_PORT };

struct file_info {
    char *filename;
    struct file_part *parts;
    struct file_info *next_file_info;
};

struct file_part {
    int id;
    char *sender_hostname;
    int sender_port;
    struct file_part *next_part;
};


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

    printf("\n");
    struct file_info *next_file = info->next_file_info;
    while (next_file != NULL) {
        printFileInfo(next_file);
        next_file = info->next_file_info;
    }
}

// ----------------------------------------------------------------------------
struct file_info *findFileInfo(struct file_info *infos, const char *filename) {
    // Handle empty list
    if (infos == NULL) return NULL;

    // Otherwise walk the list of infos, checking for filename matches
    struct file_info *info = infos;
    while (info != NULL) {
        if (strcmp(info->filename, filename) == 0) {
            return info;
        }
        info = info->next_file_info;        
    }
    return NULL;
}


// ----------------------------------------------------------------------------
// TODO: work-in-progress, still have some bugs to iron out
// ----------------------------------------------------------------------------
struct file_info *parseTracker() {
    // Open the tracker file
    FILE *file = fopen(TRACKER, "r");
    if (file == NULL) perrorExit("Tracker open error");
    else              puts("\nTracker file opened.\n");

    // Setup the file_info array
    struct file_info *fileInfos = NULL;

    // Read in a line at a time
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

        // Check if there is an existing file by this name
        struct file_info *info = findFileInfo(fileInfos, tokens[FILENAME]);

        if (info == NULL) { // No file_info for this file yet...
            // Create a new file_info structure
            info = malloc(sizeof(struct file_info));
            bzero(info, sizeof(struct file_info));

            char fname[256];
            bzero(fname, sizeof(fname));
            info->filename = strcpy(fname, tokens[FILENAME]);

            // Link it in to the list of file_infos
            if (fileInfos == NULL) {
                // Make this the start of a new list of file infos
                fileInfos = info;
            } else {
                // Add this to the end of the list of file_info structures
                struct file_info *i = fileInfos;
                while (i->next_file_info != NULL) i = i->next_file_info;
                i->next_file_info = info;
            }
        }

        // Create a new file_part structure 
        struct file_part *part = malloc(sizeof(struct file_part));
        bzero(part, sizeof(struct file_part));
        part->id              = atoi(tokens[ID]);
        part->sender_port     = atoi(tokens[SENDER_PORT]);
        part->sender_hostname = tokens[SENDER_HOSTNAME];

        // Link it in to the list of file_parts for this file_info
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

        // Get the next tracker line
        free(line);
        line = NULL;
        bytesRead = getline(&line, &lineLen, file);
    }
    free(line);

    // Close the tracker file
    if (fclose(file) != 0) perrorExit("Tracker close error");
    else                   puts("Tracker file closed.\n");

    return fileInfos; // success
}

#endif

