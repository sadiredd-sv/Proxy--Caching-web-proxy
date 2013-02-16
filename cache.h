#include "csapp.h"
#ifndef _KERNEL_TIMER_DRIVER_H_
#define _KERNEL_TIMER_DRIVER_H_

#define DEBUGx
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif



#define MAX_CACHE_SIZE (1<<20) /* 1MB */
#define MAX_OBJECT_SIZE (100<<10) /* 100kb */

/* cache_object in linked list */
struct linkedlist_cache {
    char * data; /* HTML code */
	int size; /* size of the data */
	long int lru_time_track; /* least recently used */
	char * uri;	/* identify cache objects by uri */
	struct linkedlist_cache * next; 
};

typedef struct linkedlist_cache cache_object;


/* Thread lock */
pthread_rwlock_t lock;

int add_to_cache(char * data, int site_size, char* uri);
void remove_from_cache();
cache_object* cache_find(char * uri);





/* Head of linked list cache_objects */
cache_object * head  ;
int cache_size;
long int global_time;



#endif