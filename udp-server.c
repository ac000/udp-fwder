/*
 * udp-server.c
 *
 * Copyright (C) 2014		Andrew Clayton <andrew@dnsandmx.com>
 *
 * Licensed under the MIT license.
 * See MIT-LICENSE.txt
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>

#include <curl/curl.h>

#include "config.h"

#define NAMELEN		16

struct pkt_queue {
	int count;
	int front;
	char pkts[QUEUE_SZ][PKT_SIZE];
	int rear;
};

static unsigned long nr_pkts;
static ssize_t nr_bytes;
static struct pkt_queue pkt_q;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

static void terminate(int signo)
{
	printf("Received %lu packets containing %ld bytes.\n", nr_pkts,
			nr_bytes);
	exit(EXIT_SUCCESS);
}

static void *pkt_fwder(void *arg)
{
	CURL *curl;

	curl = curl_easy_init();
	for ( ; ; ) {
		char url[1024];
		int ret;

		pthread_mutex_lock(&mtx);
		if (pkt_q.count <= 0) {
			pthread_mutex_unlock(&mtx);
			usleep(100000);
			continue;
		}
		if (pkt_q.front == QUEUE_SZ)
			pkt_q.front = 0;
		snprintf(url, sizeof(url), FWD_URL, pkt_q.pkts[pkt_q.front]);
		pkt_q.pkts[pkt_q.front++][0] = '\0';
		pkt_q.count--;
		pthread_mutex_unlock(&mtx);

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
		ret = curl_easy_perform(curl);
		if (ret != CURLE_OK)
			printf("curl_easy_perform() failed: %s\n",
					curl_easy_strerror(ret));
	}
	curl_easy_cleanup(curl);
	pthread_exit(NULL);
}

static void receiver(int sockfd)
{
	char buf[PKT_SIZE];
	struct sockaddr src;
	socklen_t srclen = sizeof(src);

	for ( ; ; ) {
		ssize_t bytes_read = recvfrom(sockfd, &buf, PKT_SIZE, 0,
				(struct sockaddr *)&src, &srclen);
		nr_bytes += bytes_read;
		nr_pkts++;
		buf[bytes_read] = '\0';

		pthread_mutex_lock(&mtx);
		if (pkt_q.count == QUEUE_SZ)
			printf("Queue overflow...\n");
		if (pkt_q.rear == QUEUE_SZ)
			pkt_q.rear = 0;
		snprintf(pkt_q.pkts[pkt_q.rear++], PKT_SIZE, "%s", buf);
		pkt_q.count++;
		pthread_mutex_unlock(&mtx);
	}
}

int main(int argc, char *argv[])
{
	int i;
	int sockfd;
	socklen_t server_len;
	struct sockaddr_in server_addr;
	struct sigaction sa;
	pthread_t tid[20];
	pthread_attr_t attr;

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	server_addr.sin_port = htons(SERVER_PORT);
	server_len = sizeof(server_addr);

	sockfd = socket(server_addr.sin_family, SOCK_DGRAM, 0);
	i = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));
	bind(sockfd, (struct sockaddr *)&server_addr, server_len);

	/* Handle Ctrl-C to terminate and print some stats */
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = terminate;
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);

	pkt_q.count = pkt_q.rear = pkt_q.front = 0;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	for (i = 0; i < 10; i++) {
		char thread_name[NAMELEN];

		pthread_create(&tid[i], &attr, pkt_fwder, NULL);
		snprintf(thread_name, NAMELEN, "udp-fwder-%d", i);
		pthread_setname_np(tid[i], thread_name);
	}
	pthread_attr_destroy(&attr);

	printf("Press Ctrl-C to terminate\n");
	receiver(sockfd);

	exit(EXIT_SUCCESS);
}
