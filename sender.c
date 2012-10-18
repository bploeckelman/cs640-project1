#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


int main(int argc, char **argv) {
    if (argc < 11) {
        printf("usage: sender -p <port> -g <requester port> -r <rate> -q <seq_no> -l <length>\n");
        exit(1);
    }

    // ------------------------------------------------------------------------

    char *portStr    = NULL;
    char *reqPortStr = NULL;
    char *rateStr    = NULL;
    char *seqNoStr   = NULL;
    char *lenStr     = NULL;

    int cmd;
    while ((cmd = getopt(argc, argv, "p:g:r:q:l:")) != -1) {
        switch(cmd) {
            case 'p': portStr    = optarg; break;
            case 'g': reqPortStr = optarg; break;
            case 'r': rateStr    = optarg; break;
            case 'q': seqNoStr   = optarg; break;
            case 'l': lenStr     = optarg; break;
            case '?':
                if (optopt == 'p' || optopt == 'g' || optopt == 'r'
                 || optopt == 'q' || optopt == 'l')
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint(optopt))
                    fprintf(stderr, "Unknown option -%c.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt);
                exit(EXIT_FAILURE);
            break;
            default: 
                printf("Unhandled argument: %d\n", cmd);
                exit(EXIT_FAILURE); 
        }
    }

    printf("Port           : %s\n", portStr);
    printf("Requester Port : %s\n", reqPortStr);
    printf("Rate           : %s\n", rateStr);
    printf("Sequence #     : %s\n", seqNoStr);
    printf("Length         : %s\n", lenStr);

    // ------------------------------------------------------------------------

    // TODO: setup sockets and send some packets

    exit(EXIT_SUCCESS);
}

