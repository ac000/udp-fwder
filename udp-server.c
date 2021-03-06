/*
 * udp-server.c
 *
 * Copyright (C) 2014 - 2016, 2019	Andrew Clayton
 * 					<andrew@zeta.digital-domain.net>
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
#include <netdb.h>
#include <signal.h>
#include <poll.h>
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
static unsigned long nr_bytes;
static struct pkt_queue pkt_q;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static void terminate(int signo __attribute__((unused)))
{
	printf("Received %lu packets containing %lu bytes.\n", nr_pkts,
			nr_bytes);
	exit(EXIT_SUCCESS);
}

static void *pkt_fwder(void *arg __attribute__((unused)))
{
	CURL *curl;

	curl = curl_easy_init();
	for ( ; ; ) {
		char url[1024];
		int ret;

		pthread_mutex_lock(&mtx);
		while (pkt_q.count <= 0)
			pthread_cond_wait(&cond, &mtx);

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

static void receiver(struct pollfd socks[])
{
	for ( ; ; ) {
		int ret;
		int i;

		ret = poll(socks, 2, -1);
		if (ret < 1)
			continue;
		for (i = 0; i < 2; i++) {
			ssize_t bytes_read;
			char buf[PKT_SIZE];
			struct sockaddr_storage src;
			socklen_t srclen = sizeof(src);

			if (!(socks[i].revents & POLLIN))
				continue;

			bytes_read = recvfrom(socks[i].fd, &buf, PKT_SIZE, 0,
					(struct sockaddr *)&src, &srclen);
			nr_bytes += bytes_read;
			nr_pkts++;
			buf[bytes_read] = '\0';

			pthread_mutex_lock(&mtx);
			if (pkt_q.count == QUEUE_SZ)
				printf("Queue overflow...\n");
			if (pkt_q.rear == QUEUE_SZ)
				pkt_q.rear = 0;
			memcpy(pkt_q.pkts[pkt_q.rear++], buf, bytes_read + 1);
			pkt_q.count++;
			pthread_cond_broadcast(&cond);
			pthread_mutex_unlock(&mtx);
		}
	}
}

static int bind_socket(const char *addr)
{
	int optval;
	int buf;
	int sockfd;
	struct addrinfo hints;
	struct addrinfo *res;
	socklen_t optlen = sizeof(optval);
	char port[6];	/* 0..65535 + '\0' */
	char baddr[INET6_ADDRSTRLEN];
	const char *ap_fmt;
	FILE *fp;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV | AI_PASSIVE;
	snprintf(port, sizeof(port), "%d", SERVER_PORT);
	getaddrinfo(addr, port, &hints, &res);

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd == -1)
		goto out;
	optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);
	if (res->ai_family == AF_INET6)
		setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &optval, optlen);
	/*
	 * Attempt to increase the receive socket buffer size. We try to
	 * set it to the value in /proc/sys/net/core/rmem_max. To go
	 * above that would require the SO_RCVBUFFORCE option with
	 * CAP_NET_ADMIN. If we can't read from
	 * /proc/sys/net/core/rmem_max or it's larger than 1MB then we
	 * set the buffer to a max of 1MB.
	 *
	 * NOTE: /proc/sys/net/core/rmem_default is roughly double
	 * what the actual buffer limit is as the kernel doubles it
	 * (for TCP) to give it headroom for bookkeeping etc.
	 * AFAICT /proc/sys/net/core/rmem_max does not include this
	 * doubling.
	 */
	fp = fopen("/proc/sys/net/core/rmem_max", "r");
	/* Clamp it to a sane limit (1MB) */
	optval = 1024 * 1024;
	if (fp) {
		int ret;

		ret = fscanf(fp, "%d", &buf);
		if (ret > 0 && buf < 1024 * 1024)
			optval = buf;
		fclose(fp);
	}
	getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &buf, &optlen);
	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &optval, optlen);
	getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &optval, &optlen);
	if (optval > buf)
		printf("udp-server: increased receive socket buffer size: "
				"%d -> %d\n", buf / 2, optval / 2);

	bind(sockfd, res->ai_addr, res->ai_addrlen);
	getnameinfo(res->ai_addr, sizeof(struct sockaddr_storage), baddr,
			INET6_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);
	if (res->ai_family == AF_INET)
		ap_fmt = "%s:%s\n";
	else
		ap_fmt = "[%s]:%s\n";
	printf("udp-server: bound to ");
	printf(ap_fmt, baddr, port);

out:
	freeaddrinfo(res);
	return sockfd;
}

int main(void)
{
	int i = 0;
	struct sigaction sa;
	struct pollfd socks[2];
	pthread_t tid[NR_FWD_THR];
	pthread_attr_t attr;
	char *server_ip;

	memset(&socks, -1, sizeof(socks));
	if (SERVER_IP4[0] != '-') {
		if (strlen(SERVER_IP4) == 0)
			server_ip = "0.0.0.0";
		else
			server_ip = SERVER_IP4;
		socks[i].fd = bind_socket(server_ip);
		socks[i].events = POLLIN;
		i++;
	}
	if (SERVER_IP6[0] != '-') {
		if (strlen(SERVER_IP6) == 0)
			server_ip = "::";
		else
			server_ip = SERVER_IP6;
		socks[i].fd = bind_socket(server_ip);
		socks[i].events = POLLIN;
	}

	/* Handle Ctrl-C to terminate and print some stats */
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = terminate;
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);

	pkt_q.count = pkt_q.rear = pkt_q.front = 0;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	for (i = 0; i < NR_FWD_THR; i++) {
		char thread_name[NAMELEN];

		pthread_create(&tid[i], &attr, pkt_fwder, NULL);
		snprintf(thread_name, NAMELEN, "udp-fwder-%d", i);
		pthread_setname_np(tid[i], thread_name);
	}
	pthread_attr_destroy(&attr);

	printf("Press Ctrl-C to terminate\n");
	receiver(socks);

	exit(EXIT_SUCCESS);
}
