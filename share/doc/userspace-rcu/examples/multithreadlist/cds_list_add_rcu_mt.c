/*
 * Copyright (C) 2013  Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 *
 * THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED
 * OR IMPLIED.  ANY USE IS AT YOUR OWN RISK.
 *
 * Permission is hereby granted to use or copy this program for any
 * purpose,  provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is
 * granted, provided the above notices are retained, and a notice that
 * the code was modified is included with the above copyright notice.
 *
 * This example shows how to add into a linked-list safely against
 * concurrent RCU traversals.
 */

#include <stdio.h>
#include <time.h>

#include <urcu.h>		/* Userspace RCU flavor */
#include <urcu/rculist.h>	/* RCU list */
#include <urcu/compiler.h>	/* For CAA_ARRAY_SIZE */

/*
 * Nodes populated into the list.
 */
struct mynode {
	int index;			/* Node content */
	int value;			/* Node content */
	struct cds_list_head node;	/* Linked-list chaining */
};

typedef struct _thrd_cfg {
	int nodecnt;
	int thrdid;
} thrd_cfg;

thrd_cfg gcfg;

CDS_LIST_HEAD(mylist);		/* Defines an empty list head */

unsigned int
calculate_time_diff (struct timespec *s, struct timespec *e)
{
	unsigned int usec;

	usec = (e->tv_sec - s->tv_sec) * 1000000 + (e->tv_nsec - s->tv_nsec)/1000;

	return usec;
}

void *
readthrd_body (void *ptr)
{
	int rc;
	int cnt = 0;
	int retrycnt = 0;
	struct timespec start;
	struct timespec end;
	unsigned int duration;
	int thrdid = (int)ptr;
	int nodecnt = gcfg.nodecnt;
	struct mynode *mnode;

	rcu_register_thread();

	//usleep (1000);
	sleep (2);

	//printf ("read thread (id: %d) start\n", thrdid);

#if 0
	while ((dll->count) < 100) {
		sleep (5);
	}
#endif

retry:
	cnt = 0;
	clock_gettime (CLOCK_REALTIME, &start);

	//rcu_read_lock();
    cds_list_for_each_entry_rcu(mnode, &mylist, node) {
		if (mnode->value == (mnode->index*10))
        	cnt++;
		else
			printf ("invalid value %d, cnt: %d\n", mnode->value, cnt);
    }
	//rcu_read_unlock();

	clock_gettime (CLOCK_REALTIME, &end);
	duration = calculate_time_diff (&start, &end);
	if (cnt != nodecnt) {
		retrycnt++;
		goto retry;
	}

	printf ("thrdid: %d: nodes processed %d, retrycnt: %d, duration (usec): %d\n", thrdid, cnt, retrycnt, duration);

	rcu_unregister_thread();

	return 0;
}

int
main (int argc, char **argv)
{
	int ii;
	pthread_t *readthrd;
	struct timespec start;
	struct timespec end;
	unsigned int duration;
	struct mynode *node;
	int nodecnt;
	int thrdcnt;

	if (argc != 3) {
		printf ("Usage:./a.out nodecnt thrdcnt\n");
		exit (0);
	}

	nodecnt = atoi (argv[1]);
	thrdcnt = atoi (argv[2]);

	rcu_register_thread();

	readthrd = (pthread_t *)malloc (sizeof(pthread_t)*thrdcnt);

	gcfg.nodecnt = nodecnt;

	for (ii = 1; ii <= thrdcnt; ii++) {
		pthread_create (&(readthrd[ii]), NULL, readthrd_body, (void *)ii);
	}

	//printf ("main thread start\n");

	//rcu_read_lock();
	clock_gettime (CLOCK_REALTIME, &start);
	for (ii = 0; ii < nodecnt; ii++) {
		node = malloc(sizeof(struct mynode));
		node->index = ii;
		node->value = ii*10;
		cds_list_add_rcu(&node->node, &mylist);
	}
	clock_gettime (CLOCK_REALTIME, &end);
	//rcu_read_unlock();
	duration = calculate_time_diff (&start, &end);
	printf ("main thread: time taken in adding %d nodes (usec): %d\n", nodecnt, duration);

	sleep (100000);

	rcu_unregister_thread();
}
