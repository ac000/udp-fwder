/*
 * nsleep.h - nsleep API header
 *
 * Copyright (C) 2009		Andrew Clayton <andrew@digital-domain.net>
 *
 * Licensed under the MIT license.
 * See MIT-LICENSE.txt
 */

#ifndef _NSLEEP_H_
#define _NSLEEP_H_

/*
 * Define a second, milliseond and a microsecond in terms of nanoseconds.
 */
#define NS_SEC		1000000000
#define NS_MSEC		1000000
#define NS_USEC		1000

void nsleep(uint64_t nsecs);

#endif /* _NSLEEP_H_ */
