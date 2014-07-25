/*
 * nsleep.c - A nanosleep wrapper
 *
 * Copyright (C) 2009   Andrew Clayton <andrew@digital-domain.net>
 *
 * Licensed under the MIT license.
 * See MIT-LICENSE.txt
 */

#define _POSIX_C_SOURCE	199309L

#include <stdint.h>
#include <time.h>

#include "../include/nsleep.h"

void nsleep(uint64_t period)
{
	struct timespec req;
	struct timespec rem;
	int ret;

	if (period < NS_SEC) {
		req.tv_sec = 0;
		req.tv_nsec = period;
	} else {
		req.tv_sec = period / NS_SEC;
		req.tv_nsec = period % NS_SEC;
	}

do_sleep:
	ret = nanosleep(&req, &rem);
	if (ret) {
		req.tv_sec = rem.tv_sec;
		req.tv_nsec = rem.tv_nsec;
		goto do_sleep;
	}
}
