#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sched.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#define THREADS 32
#define MAXWAIT 128
#define STACKSIZE 65536
#define FILESIZE (32000 * 65536)

struct thread {
	pid_t pid;
	int fd;
};

struct thread threads[THREADS];
int sockfd;
char *path;

#define info (*((struct thread *) data))

int thr(void *data)
{
	int dfd;
	off_t zero;
        char req[4096];
        int len;
        char *nlnl;

        if((dfd = open(path, O_RDONLY)) < 0) {
          fprintf(stderr, "could not open path to serve\n");
          exit(1);
        }

	while (1) {
		info.fd = accept(sockfd, NULL, NULL);

                *(uint32_t *)req = 0;
                while((len = read(info.fd, req + 4, sizeof(req) - 8)) > 0) {
                  for(nlnl = req; nlnl < req + len + 4; ++nlnl) {
                    if(*(uint16_t *)nlnl == 0x0A0A) goto req_complete;
                    if(*(uint32_t *)nlnl == 0x0A0D0A0Dul) goto req_complete;
                  }
                  *(uint32_t *)req = *(uint32_t *)(req + len);
                }

                req_complete:

		zero = 0;
		sendfile(info.fd, dfd, &zero, FILESIZE);
		shutdown(info.fd, SHUT_WR);
                close(info.fd);
	}
	return 0;
}

void term(int no_use)
{
	exit(0);
}

int main(int argc, char *argv[])
{
	struct sockaddr_in http_addr;
	int nt;
	char *stack;
	int yes = 1;
        int port;

	if(setuid(65535) < 0) {
		fprintf(stderr, "setuid failed\n");
		return 1;
	}

        if(argc != 3) {
          fprintf(stderr, "Usage: sfhttpd <port> <file>\n");
          return 1;
        }

        if(sscanf(argv[1], "%d", &port) != 1) {
          fprintf(stderr, "Usage: sfhttpd <port> <file>\n");
          return 1;
        }

        path = argv[2];

	http_addr.sin_family = AF_INET;
	http_addr.sin_port = htons(port);
	http_addr.sin_addr.s_addr = INADDR_ANY;
	
	if ((sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		fprintf(stderr, "could not create socket\n");
		return 1;
	}

	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
		fprintf(stderr, "could not set SO_REUSEADDR");
		return 1;
	}
	if (bind(sockfd, (struct sockaddr *) &http_addr, sizeof(http_addr)) < 0) {
		fprintf(stderr, "could not bind to port %d\n", port);
		return 1;
	}
	if (listen(sockfd, MAXWAIT) < 0) {
		fprintf(stderr, "could not listen\n");
		return 1;
	}

	signal(SIGINT, term);
	signal(SIGPIPE, SIG_IGN);

	stack = malloc(THREADS * STACKSIZE);
	for (nt = 0; nt < THREADS; nt++) {
		if ((threads[nt].pid = clone(thr, stack += STACKSIZE,
				CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_VM,
				threads + nt)) < 0) {
			fprintf(stderr, "thread creation failed\n");
			return 1;
		}
	}

	pause();
	return 0;
}
