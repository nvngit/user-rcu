#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include "list.h"

typedef struct _thrd_cfg {
	int nodecnt;
	int thrdid;
} thrd_cfg;

unsigned int
calculate_time_diff (struct timespec *s, struct timespec *e)
{
	unsigned int usec;

	usec = (e->tv_sec - s->tv_sec) * 1000000 + (e->tv_nsec - s->tv_nsec)/1000;

	return usec;
}
	
/* Naive linked list implementation */

list *dll = NULL;
thrd_cfg gcfg;

list *
list_create(int nodecnt)
{
	list *l = (list *) malloc(sizeof(list));

	l->count = 0;
	l->head = NULL;
	l->tail = NULL;
	l->id2ptr = malloc (sizeof(list_item *)*nodecnt);
	if (l->id2ptr == NULL) {
		printf ("malloc failed\n");
		return NULL;
	}

	memset (l->id2ptr, 0, sizeof(list_item *)*nodecnt);
	pthread_mutex_init(&(l->mutex), NULL);
	return l;
}

void
list_free(list *l)
{
	list_item *li, *tmp;

	pthread_mutex_lock(&(l->mutex));

	if (l != NULL) {
	  li = l->head;
	  while (li != NULL) {
		tmp = li->next;
		free(li);
		li = tmp;
	  }
	}

	pthread_mutex_unlock(&(l->mutex));
	pthread_mutex_destroy(&(l->mutex));
	free(l);
}


list_item *
list_add_element(list *l, void *ptr)
{
	list_item *li;
	static int idc = 0;

	pthread_mutex_lock(&(l->mutex));

	li = (list_item *) malloc(sizeof(list_item));
	li->value = ptr;
	li->id = idc++;
	li->next = NULL;
	li->prev = l->tail;

	if (l->tail == NULL) {
		l->head = l->tail = li;
	}
	else {
		l->tail->next = li;
		l->tail = li;
	}
	l->count++;
	l->id2ptr[li->id] = (int)li;

	pthread_mutex_unlock(&(l->mutex));

	return li;
}


int
list_remove_element(list *l, void *ptr, int id)
{
	int result = 0;
	list_item *li = l->head;

	pthread_mutex_lock(&(l->mutex));

	while (li != NULL) {
	  if (li->id == id) {
		if (li->prev == NULL) {
		  l->head = li->next;
		}
		else {
		  li->prev->next = li->next;
		}

		if (li->next == NULL) {
		  l->tail = li->prev;
		}
		else {
		  li->next->prev = li->prev;
		}
		l->count--;
		free(li);
		result = 1;
		break;
	  }
	  li = li->next;
	}

	pthread_mutex_unlock(&(l->mutex));

	return result;
}

int
list_element(list *l, int id)
{
	list_item *li;
	int rc = -1;

	pthread_mutex_lock(&(l->mutex));

	li = (list_item *)(l->id2ptr[id]);
	if (li == NULL) return rc;

	if (li->id != id) return rc;

	rc = 0;

	pthread_mutex_unlock(&(l->mutex));
	return rc;
}

int
list_element_wo_cache(list *l, int id)
{
	list_item *li;
	int rc = -1;

	pthread_mutex_lock(&(l->mutex));

	li = l->head;
	while (li != NULL) {
	  if (li->id == id) {
		rc = 0;
		break;
	  }
	  li = li->next;
	}

	pthread_mutex_unlock(&(l->mutex));
	return rc;
}

void
list_each_element(list *l, int (*func)(list_item *))
{
	list_item *li;

	pthread_mutex_lock(&(l->mutex));

	li = l->head;
	while (li != NULL) {
	  if (func(li) == 1) {
		break;
	  }
	  li = li->next;
	}

	pthread_mutex_unlock(&(l->mutex));
}

void *
readthrd_body (void *ptr)
{
	int ii;
	int rc;
	int cnt = 0;
	int retrycnt = 0;
	struct timespec start;
	struct timespec end;
	unsigned int duration;
	int thrdid = (int)ptr;
	int nodecnt = gcfg.nodecnt;


	//usleep (1000);
	sleep (2);
	//printf ("read thread (id: %d) start\n", thrdid);
#if 0
	while ((dll->count) < 100) {
		sleep (5);
	}
#endif

retry:
	clock_gettime (CLOCK_REALTIME, &start);

	cnt = 0;
	for (ii = 0; ii < nodecnt; ii++) {
		rc = list_element (dll, ii);

		if (rc == 0) cnt++;

#if 0
		if (ii && (ii % 10000 == 0)) {
			printf ("read thrd id: %d, processed %d nodes\n", thrdid, ii);
		}
#endif
	}

	if (cnt < nodecnt) {
		retrycnt++;
		goto retry;
	}

	clock_gettime (CLOCK_REALTIME, &end);
	duration = calculate_time_diff (&start, &end);
	printf ("thrdid: %d, nodes processed %d, retrycnt: %d, duration (usec): %d\n", thrdid, cnt, retrycnt, duration);

	return 0;
}

int
main (int argc, char **argv)
{
	int ii;
	pthread_t *readthrd;
  	list_item *li;
	struct timespec start;
	struct timespec end;
	unsigned int duration;
    int nodecnt;
    int thrdcnt;

    if (argc != 3) {
        printf ("Usage:./a.out nodecnt thrdcnt\n");
        exit (0);
    }

    nodecnt = atoi (argv[1]);
    thrdcnt = atoi (argv[2]);

	readthrd = (pthread_t *)malloc (sizeof(pthread_t)*thrdcnt);

	gcfg.nodecnt = nodecnt;

	dll = list_create(nodecnt);

	for (ii = 1; ii <= thrdcnt; ii++) {
		pthread_create (&(readthrd[ii]), NULL, readthrd_body, (void *)ii);
	}

	//printf ("main thread start\n");

	clock_gettime (CLOCK_REALTIME, &start);
	for (ii = 0; ii < nodecnt; ii++) {
		li = list_add_element (dll, NULL);
		//printf ("write, id: %d\n", li->id);
	}
	clock_gettime (CLOCK_REALTIME, &end);
	duration = calculate_time_diff (&start, &end);
	printf ("main thread: time taken in adding %d nodes (usec): %d\n", nodecnt, duration);

	sleep (100000);
}
