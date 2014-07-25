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

#include "../include/nsleep.h"

#define SERVER_IP	server_ip
#define SERVER_PORT	server_port

#define NR_PKTS		nr_pkts
#define NR_ITER		nr_iter

#define MSG		"+RT:AEOS,3456777777,1,10,1,10,10,10,22,10"
//			"+RT:AEOS,3456777777,1,10,1,10,10,10,22,10"
//			"+RT:AEOS,3456777777,1,10,1,10,10,10,22,10"
//			"+RT:AEOS,3456777777,1,10,1,10,10,10,22,10"
//			"+RT:AEOS,3456777777,1,10,1,10,10,10,22,10"

static const char *server_ip;
static int server_port;
static int nr_pkts;
static int nr_iter;

static void disp_usage(void)
{
	printf("Usage: client -s <server IP> -p <server port> -n <nr packets>\n"		"              -i <nr iterations>\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	int i;
	int j;
	int opt;
	int sockfd;
	sa_family_t family;
	socklen_t addr_len;
	struct sockaddr_in addr4;
	struct sockaddr_in6 addr6;
	struct sockaddr_storage *addr;
	struct timespec stp;
	struct timespec etp;

	while ((opt = getopt(argc, argv, "s:p:n:i:")) != -1) {
		switch (opt) {
		case 's':
			server_ip = optarg;
			break;
		case 'p':
			server_port = atoi(optarg);
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

	if (!strchr(SERVER_IP, ':')) {
		memset(&addr4, 0, sizeof(addr4));
		addr4.sin_family = family = AF_INET;
		inet_pton(AF_INET, SERVER_IP, &addr4.sin_addr);
		addr4.sin_port = htons(SERVER_PORT);

		addr = (struct sockaddr_storage *)&addr4;
		addr_len =  sizeof(addr4);
	} else {
		memset(&addr6, 0, sizeof(addr6));
		addr6.sin6_family = family = AF_INET6;
		inet_pton(AF_INET6, SERVER_IP, &addr6.sin6_addr);
		addr6.sin6_port = htons(SERVER_PORT);

		addr = (struct sockaddr_storage *)&addr6;
		addr_len =  sizeof(addr6);
	}

	sockfd = socket(family, SOCK_DGRAM, 0);

	clock_gettime(CLOCK_MONOTONIC, &stp);
	for (j = 0; j < NR_ITER; j++) {
		for (i = 0; i < NR_PKTS; i++) {
			sendto(sockfd, MSG, strlen(MSG), 0,
					(struct sockaddr *)addr, addr_len);
			nsleep(NS_USEC * 30);
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
