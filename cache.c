#include "cache.h"


/*
	add_to_cache - add a cache_object to the cache
	Write lock the function to not let other threads
	edit shared memory space. Other processes will wait
	until this function is done, including removing 
	nodes from the cache linked list. 
*/

int add_to_cache(char * data, int site_size, char* uri) {
	if (site_size + sizeof(cache_object) <= MAX_OBJECT_SIZE)
	{
		return 0;
	} else {
		dbg_printf("add_to_cache");
		/* Writing to the cache - only one thread should be able to write to it */
		pthread_rwlock_wrlock(&lock); /* lock on the resource*/
		/* Free enough space for the new site in cache*/
		while(cache_size + site_size + sizeof(cache_object) >= MAX_CACHE_SIZE ){
			remove_from_cache();
		}
		/* Add the site node to the start of the cache list*/
		cache_object * site = (cache_object *)malloc(sizeof(cache_object));
		site -> next = head;
		site -> data = data;
		site -> lru_time_track = global_time++;
		site -> size = site_size;
		site -> uri = malloc(( strlen( uri ) + 1 ) * sizeof( char ));
		strcpy( site -> uri, uri );
		head = site;
		cache_size +=  sizeof( cache_object ) + site_size + 
						strlen( uri ) + 1;

		pthread_rwlock_unlock( &lock );
		return 1;
	}
}


/*
	cache_find - returns cache_object if found, otherwise NULL.
*/

cache_object* cache_find( char * uri ){
	dbg_printf( "cache_find" );
	cache_object * site = NULL;
    printf("INSIDE Cache_find %s\n",uri);
	if( head != NULL ){
		printf( "here" );
		for ( site = head; site != NULL ; 
			site = site -> next ){
			printf("here 2\n");
			printf("%s\n",site ->uri );
			printf("%s\n",uri );
			if (!strcmp( site -> uri, uri ))
			{
				printf("Cache hittt\n");
				break;
			}			
		}
	} else {
		printf(" Cache miss\n ");
		return NULL;
	}
	if(site != NULL)
		printf(" Cache Hit\n ");
	return site;
}


/*
	remove_from_cache - add a cache_object to the cache
*/
void remove_from_cache(){
	cache_object * p ;  	/* Previous pointer */
	cache_object * q ;		/* Current Pointer */
	cache_object * temp ;	/* pointer to free */

	if( head != NULL) {
		/* find lru */
		/* traverse the list */
		for (q = head, p = head, temp =head ; q -> next != NULL; 
			q = q -> next) {
			if(( (q -> next) -> lru_time_track) < (temp -> lru_time_track)) {
				temp = q -> next;
				p = q;
			}
		}

		/* free required node from the list*/
		if(temp == head) {
			head = head -> next; /*Handle the base case*/
		} else {
			p->next = temp->next;	
		}

		/* Change cache size(free temp node) */
		cache_size = cache_size - (temp -> size) - sizeof(cache_object) - 
			strlen(temp -> uri) - 1;

		/* Free temp node*/
		free(temp->data);
		free(temp->uri);
		free(temp);
	} 
	return;
}
