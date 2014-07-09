/*
 * client.c
 *
 * Copyright (C) 2014		Andrew Clayton <andrew@dnsandmx.com>
 *
 * Licensed under the MIT license.
 * See MIT-LICENSE.txt
 */

#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

#define SERVER_IP	"127.0.0.1"
#define SERVER_PORT	3232

#define NS_SEC		1000000000
#define NS_MSEC		1000000
#define NS_USEC		1000

#define NR_PKTS		10000
#define NR_ITER		10

#define MSG		"+RT:AEOS,3456777777,1,10,1,10,10,10,22,10"
//			"+RT:AEOS,3456777777,1,10,1,10,10,10,22,10"
//			"+RT:AEOS,3456777777,1,10,1,10,10,10,22,10"
//			"+RT:AEOS,3456777777,1,10,1,10,10,10,22,10"
//			"+RT:AEOS,3456777777,1,10,1,10,10,10,22,10"

int main(int argc, char *argv[])
{
	int i;
	int j;
	int sockfd;
	socklen_t server_len;
	struct sockaddr_in server_addr;
	struct timespec stp;
	struct timespec etp;

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	server_addr.sin_port = htons(SERVER_PORT);
	server_len = sizeof(server_addr);

	sockfd = socket(server_addr.sin_family, SOCK_DGRAM, 0);

	clock_gettime(CLOCK_MONOTONIC, &stp);
	for (j = 0; j < NR_ITER; j++) {
		for (i = 0; i < NR_PKTS; i++) {
			sendto(sockfd, MSG, strlen(MSG), 0,
				(struct sockaddr *)&server_addr, server_len);
			usleep(30);
		}
	}
	clock_gettime(CLOCK_MONOTONIC, &etp);

	printf("Sent %d, %lu byte packets in %ums\n", NR_PKTS * NR_ITER,
			strlen(MSG),
			(unsigned int)((etp.tv_sec * 1000 + etp.tv_nsec /
					NS_MSEC) -
				(stp.tv_sec * 1000 + stp.tv_nsec / NS_MSEC)));
	exit(EXIT_SUCCESS);
}
