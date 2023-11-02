#define _POSIX_C_SOURCE 200112L
#include "utils.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/time.h>

int TIMEOUT_SECONDS = 31;
int create_listening_socket(char* service) {
	int re, s, sockfd;
	struct addrinfo hints, *res;

	// Create address we're going to listen on (with given port number)
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6;       // IPv6
	hints.ai_socktype = SOCK_STREAM; // Connection-mode byte streams
	hints.ai_flags = AI_PASSIVE;     // for bind, listen, accept
	// node (NULL means any interface), service (port), hints, res
	s = getaddrinfo(NULL, service, &hints, &res);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	// Create socket
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	// Reuse port if possible
	re = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &re, sizeof(int)) < 0) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	// Bind address to the socket
	if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(res);

	return sockfd;
}

int create_commute_socket(char *addr, int port) {
    int sockfd, s, new_sockfd;
	struct addrinfo hints, *servinfo, *rp;

	// Create address
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;

    char port_str[6];  // 5 digits for port number + 1 for '\0'
    sprintf(port_str, "%d", port);
	// Get addrinfo of server.  
	s = getaddrinfo(addr, port_str, &hints, &servinfo);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	// Connect to first valid result
	for (rp = servinfo; rp != NULL; rp = rp->ai_next) {
		sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sockfd == -1)
			continue;

		if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) {
			struct timeval tv;
			tv.tv_sec = TIMEOUT_SECONDS;  // Set this to your desired timeout in seconds
			tv.tv_usec = 0;  // Not setting microseconds part of timeout
			if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv)) {
				perror("setsockopt");
				exit(EXIT_FAILURE);
			}

			new_sockfd = sockfd;

			break; 
		}

		perror("connect");
		close(sockfd);
	}

	if (rp == NULL) {
		exit(EXIT_FAILURE);
		return 0;
	}
	freeaddrinfo(servinfo);

    return new_sockfd;
}

uint64_t htonll(uint64_t value) {
    // The answer is 12345678
    int num = 1;
    // Check the endianness
    if(*(char *)&num == 1) {
        uint32_t high_part = htonl((uint32_t)(value >> 32));
        uint32_t low_part = htonl((uint32_t)(value & 0xFFFFFFFFLL));

        return (((uint64_t)low_part << 32) | high_part);
    } else {
        return value;
    }
}

uint64_t ntohll(uint64_t value) {
    const int num = 1;
    if(*(char *)&num == 1) {
        uint32_t high_part = ntohl((uint32_t)(value >> 32));
        uint32_t low_part = ntohl((uint32_t)(value & 0xFFFFFFFFLL));

        return (((uint64_t)low_part << 32) | high_part);
    } else {
        return value;
    }
}
