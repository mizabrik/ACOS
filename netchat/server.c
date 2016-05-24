#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>

#define TRUE 1
#define FALSE 0

void my_error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, new_sockfd, portno;
	int len = -1;
	int desc_ready, end_server, compress_array;
	desc_ready, end_server, compress_array = FALSE;
	int close_conn;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n, rc, on, timeout;
	struct pollfd fds[200];
	int nfds, current_size, i, j;
	nfds = 1;
	current_size = 1;
	on = 1;

    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        my_error("ERROR opening socket");
	rc = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));

	if(rc < 0) {
		close(sockfd);
		my_error("ERROR allow to be reusable\n");
	}

	rc = ioctl(sockfd, FIONBIO, (char*)&on);

	if(rc < 0) {
		close(sockfd);
		my_error("ERROR set socket to be nonblocking\n");
	}


	bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        my_error("ERROR on binding\n");
	listen(sockfd, 5);

	memset(fds, 0, sizeof(fds));
	fds[0].fd = sockfd;
	fds[0].events = POLLIN;

	timeout = 3 * 60 * 1000;
	
	do {
		printf("Waiting on poll()...\n");
		rc = poll(fds, nfds, timeout);
		if(rc < 0) {
			perror("poll() failed");
			break;
		}

		if(rc == 0) {
			printf("poll() timed out");
			break;
		}

		current_size = nfds;
		for(i = 0; i < current_size; ++i) {
			if(fds[i].revents == 0) {
				continue;
			}

			if(fds[i].revents != POLLIN) {
				printf("revents = %d\n", fds[i].revents);
				end_server = TRUE;
				break;
			}

			if(fds[i].fd == sockfd) {
				printf("Listening socket is readable\n");

				do {
					new_sockfd = accept(sockfd, NULL, NULL);
					if(new_sockfd < 0) {
						if(errno != EWOULDBLOCK) {
							perror("ERROR accept()");
							end_server = TRUE;
						}
						break;
					}
					printf("New incoming connection - %d\n", new_sockfd);
					fds[nfds].fd = new_sockfd;
					fds[nfds].events = POLLIN;
					nfds++;
				} while(new_sockfd != -1);
			} else {
				printf("Descriptor %d is readable\n", fds[i].fd);
				close_conn = FALSE;

				rc = recv(fds[i].fd, buffer, sizeof(buffer), 0);
				if(rc < 0) {
					if(errno != EWOULDBLOCK) {
						perror("recv() failed");
						close_conn = TRUE;
					} else {
						printf("Get EWOULDBLOCK\n");
					}
					break;
				}

				if(rc == 0) {
					printf("Connection closed");
					close_conn = TRUE;
					break;
				}

				len = rc;
				printf("%d bytes received\n", len);

				for(j = 0; j < current_size; ++j) {
					if(fds[j].fd != sockfd && i != j) {
						rc = send(fds[j].fd, buffer, len, 0);
					}
				} 
				/*do {
					rc = recv(fds[i].fd, buffer, sizeof(buffer), 0);
					if(rc < 0) {
						if(errno != EWOULDBLOCK) {
							perror("recv() failed");
							close_conn = TRUE;
						} else {
							printf("Get EWOULDBLOCK\n");
						}
						break;
					}

					if(rc == 0) {
						printf("Connection closed");
						close_conn = TRUE;
						break;
					}

					len = rc;
					printf("%d bytes received\n", len);

					for(j = 0; j < current_size; ++j) {
						if(fds[j].fd != sockfd && i != j) {
							rc = send(fds[j].fd, buffer, len, 0);
						}
					} 
				} while(TRUE);*/
				if(close_conn) {
					close(fds[i].fd);
					fds[i].fd = -1;
					compress_array = TRUE;
				}
			}
		}
		if(compress_array) {
			compress_array = FALSE;
			for(i = 0; i < nfds; i++) {
				if(fds[i].fd == -1) {
					for(j = i; j < nfds; j++) {
						fds[j].fd = fds[j+1].fd;
					}
					i--;
					nfds--;
				}
			}
		}
	}
	while(end_server == FALSE && !feof(stdin));
	
	for(i = 0; i < nfds; i++) {
		if(fds[i].fd >= 0) {
			close(fds[i].fd);
		}
	}

	return EXIT_SUCCESS;
}
