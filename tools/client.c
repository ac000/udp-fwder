/*
 * client.c
 *
 * Copyright (C) 2014 - 2015	Andrew Clayton <andrew@zeta.digital-domain.net>
 *
 * Licensed under the MIT license.
 * See MIT-LICENSE.txt
 */

#define _POSIX_C_SOURCE	201112L		/* getaddrinfo(3), freeaddrinfo(3) */
#define _XOPEN_SOURCE	500		/* random(3), srandom(3) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>

#include "../include/nsleep.h"

#define SERVER		server
#define SERVER_PORT	server_port

#define NR_PKTS		nr_pkts
#define NR_ITER		nr_iter

static const char *server;
static const char *server_port;
static int nr_pkts;
static int nr_iter;
static unsigned long nr_bytes;

static const char *msgs[] = {
		"+RT:AEOS,3456777777,1,10,1,10,10,10,22,11",
		"+RT:AEOS,3456777777,1,10,1,10,10,10,22,20",
		"+RT:AEOS,3456777777,1,10,1,10,10,10,22,33",
		"+RT:AEOS,3456777777,1,10,1,10,10,10,22,5" };

static void disp_usage(void)
{
	printf("Usage: client -s <server> -p <server port> -n <nr packets>\n"		"              -i <nr iterations>\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	int i;
	int j;
	int opt;
	int sockfd;
	struct addrinfo hints;
	struct addrinfo *res;
	struct timespec stp;
	struct timespec etp;

	while ((opt = getopt(argc, argv, "s:p:n:i:h?")) != -1) {
		switch (opt) {
		case 's':
			server = optarg;
			break;
		case 'p':
			server_port = optarg;
			break;
		case 'n':
			nr_pkts = atoi(optarg);
			break;
		case 'i':
			nr_iter = atoi(optarg);
			break;
		default:
			disp_usage();
		}
	}
	if (optind < 9)
		disp_usage();

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	getaddrinfo(SERVER, SERVER_PORT, &hints, &res);

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	clock_gettime(CLOCK_MONOTONIC, &stp);
	for (j = 0; j < NR_ITER; j++) {
		srandom(getpid() + j);
		for (i = 0; i < NR_PKTS; i++) {
			int n = random() % 4;
			nr_bytes += sendto(sockfd, msgs[n], strlen(msgs[n]), 0,
					res->ai_addr, res->ai_addrlen);
			nsleep(NS_USEC * 30);
		}
	}
	clock_gettime(CLOCK_MONOTONIC, &etp);

	printf("Sent %d packets containing %lu bytes in %ums\n",
			NR_PKTS * NR_ITER, nr_bytes,
			(unsigned int)((etp.tv_sec * 1000 + etp.tv_nsec /
					NS_MSEC) -
				(stp.tv_sec * 1000 + stp.tv_nsec / NS_MSEC)));
	freeaddrinfo(res);

	exit(EXIT_SUCCESS);
}
