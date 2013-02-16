/**
 *  15-213 Proxy lab
 *  AndrewId1: sadiredd
 *  AndrewId2: himanshp 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csapp.h"
#include "cache.h"

/* functions */
void *thread(void *vargp);
void do_proxy(int fd);
void uri_parsing(char *uri, char *hostname, char *path, int *port);

struct hostent *gethostbyname_ts(char *hostname);
int open_clientfd_ts(char *hostname, int port);

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, 
	char *longmsg);
void reset(char *buffer1,char *buffer2, char * method, char *request,
 char *uri, char *hostname, char *path);

/* Thread lock */
pthread_rwlock_t lock;

/* Mutex */
static sem_t mutex;





/**
 * is_get_method - Check if its a get request
 * @param  method - Store request method
 * @return 0 - its not a get request, 1 - get request
 */
int is_get_method(char * method){
	if (strcasecmp(method, "GET")) { 
		return 0;
	}
	return 1;
}


/* 
	main - Inifite while loop accepts connections
	and spawns off threads to handle requests. 
*/
int main(int argc, char **argv) 
{
	int listenfd, port;
	int *connfdp;
	global_time =0;
	socklen_t clientlen = sizeof(struct sockaddr_in);
	struct sockaddr_in clientaddr;
	pthread_t tid;
	pthread_rwlock_init(&lock, 0);
    Sem_init(&mutex,0,1);
	cache_size = 0;

	/* Check command line args */
	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}
	port = atoi(argv[1]);

	/* Ingore SIGPIPE */
	signal(SIGPIPE, SIG_IGN);
	listenfd = open_listenfd(port);

	while (1) {
		connfdp = Malloc(sizeof(int));
		if(connfdp == NULL){
			continue;
		}
		*connfdp = accept(listenfd, (SA *)&clientaddr, &clientlen);

		/* Create thread and let thread handle functions */
		if(( *connfdp > 0 )){
			Pthread_create(&tid, NULL, thread, connfdp);
		}
	}
	Free(connfdp);
}


/**
 * thread - Handles concurrent request from the client
 * @param vargp - Connection file descriptor received when connection was
 *                accepted from the client
 */
void *thread(void *vargp) {
	int connfd = *((int *) vargp);
	pthread_detach(pthread_self());
	Free(vargp);
	do_proxy(connfd);
	close(connfd);
	return NULL;
}


/**
 * do_proxy - Function handles the Http request from the client. It searches 
 * 			  for the request in the proxy cache, if not available in cache,
 * 			  retreives from the web server. 
 * @param clientfd - Conection file descriptor sent to the function by the
 *                 	 thread
 */
void do_proxy(int clientfd) {
	int serverfd, port, content_len, content_size = 0,count = 0;

	char *buffer1 = Calloc(MAXLINE,1); /* reads http request from the client*/
	char *buffer2 = Calloc(MAXLINE,1); /* reads response from the server*/
	char *uri = Calloc(MAXLINE,1); /* Resource Identifier */
	char *path = Calloc(MAXLINE,1); /* Relative path to be extracted from the 
		 							  uri*/
	char *method = Calloc(MAXLINE,1); /* GET Method*/
	char *request = Calloc(MAXLINE,1); /* Request from proxy(1.0) */
	char *hostname = Calloc(MAXLINE,1); /*Hostname - to be extracted from 
		 							    the uri*/
		 
	char* data = NULL; 
	cache_object * site = NULL;
	rio_t server_rio;
	rio_t client_rio;

	/* Read request line and headers */
	rio_readinitb(&client_rio, clientfd);

	/* Read each line of the header request*/
	while(rio_readlineb(&client_rio, buffer1, MAXLINE) && 
		strcmp(buffer1, "\r\n")){
		/*if first line, retrieve method and uri */
		dbg_printf("---------------------- %d\n", count);
		/* Count can have 0 or 1 value*/
		if(!count){
		    dbg_printf("Reading each line of the header request: %s",buffer1);
			sscanf(buffer1, "%s %s", method, uri);
			/* Parses the uri */
			uri_parsing(uri, hostname, path, &port);
			sprintf(buffer1, "%s %s HTTP/1.0\r\n", method, path);
			strcat(request, buffer1);
			count =1;
		} else {
			/*request is comprised of all lines of header */
			char * word;
			dbg_printf("FILTER :%s",buffer1);
			
			/* change from HTTP/1.1 to HTTP/1.0 */
			if((word = strstr(buffer1, "HTTP/1.1"))){
				strncpy(word, "HTTP/1.0", strlen("HTTP/1.0"));
			}
			
			/* change to Proxy-Connection:close */
			if((word = strstr(buffer1, "Proxy-Connection: "))){
				strcpy(buffer1, "Proxy-Connection: close\r\n");
			} else if((word = strstr(buffer1, "Connection:"))){
				strcpy(buffer1, "Connection: close\r\n");
			}
			
			/* Fail safe - delete Keep-Alive */
			if((word = strstr(buffer1, "Keep-Alive:"))){
				buffer1 = "";
			}
			dbg_printf("FILTERAfter :%s",buffer1);
			strcat(request, buffer1);
		}

	}
	
	dbg_printf("\nDEBUG: %s",request);
	
	/* Add new line at the end of request*/
	strcat(request, "\r\n");

	/* Only handle GET */
	if( !is_get_method( method )) {
		reset(buffer1,buffer2,method,request,uri,hostname,path);
		clienterror(clientfd, method, "501", "Not Implemented", 
			"Proxy does	not implement this.");
		return;
	}
		

	pthread_rwlock_rdlock(&lock);
	/* Find uri in cache and write to clientfd */
	if((site = cache_find(uri)) != NULL){
		rio_writen(clientfd, site->data, site->size); 	
		site -> lru_time_track = global_time++;
		printf("Cache hit\n");
		pthread_rwlock_unlock(&lock);
		reset(buffer1,buffer2,method,request,uri,hostname,path);
		return;
	}
	pthread_rwlock_unlock(&lock);

	if((serverfd = open_clientfd_ts(hostname, port)) == -1){
		printf("Open Serverfd failed.\n");		
		reset(buffer1,buff er2,method,request,uri,hostname,path);
		return;
	}

	/* Associate serverfd  */
	rio_readinitb(&server_rio, serverfd); 

	/* Write HTTP request object to Server through serverfd */
	if(rio_writen( serverfd, request, strlen( request )) < 0){
		//printf("Server request write failed\n");
		close(serverfd);
		reset(buffer1,buffer2,method,request,uri,hostname,path);
		return;
	}

	/* Read response from Server, send to client and add to cache */
	while((content_len = rio_readnb(&server_rio, buffer2, MAXLINE)) > 0){
		rio_writen(clientfd, buffer2, content_len);
		/* Prepare for cache */
		if((data = Realloc(data, content_size + 
			content_len*sizeof(char))) == NULL){
			printf("Realloc error\n");
			close(serverfd);
			reset(buffer1,buffer2,method,request,uri,hostname,path);
			return;
		}
		/* add new data to the memory*/
		memcpy(data + content_size, buffer2, content_len);
		/* Increase memory size*/
		content_size = content_size + content_len;
	}		

	/* We have full site here, Add site to cache */
	/*Adding chek for the size in the add_to_cache. */
	if(!add_to_cache(data, content_size, uri)){
		/* Site is bigger than object size */
		free(data);
	}	

	
	/* Close the connection */
	close(serverfd);
	reset(buffer1,buffer2,method,request,uri,hostname,path);
	return;
}



/* 
	uri_parsing - Uses function strpbrk to break up URI and extract the
	port and path. strpbrk returns a pointer to the start of the 
	position of the string that contains the key. In this case, temp 
	will hold the last portion of the URI: ":[PORT]/path". Default case
	of port is 80.
*/
void uri_parsing(char *uri, char *hostname, char *path, int *port) {	
	dbg_printf("uri_parsing");
	char *temp;
	*port = 0;
	strcpy(path, "/");
	if((temp = strpbrk(strpbrk(uri, ":") + 1, ":"))){
		sscanf(temp, "%d %s", port, path);
		sscanf(uri, "http://%[^:]s", hostname);
	} else {
		sscanf(uri, "http://%[^/]s/", hostname);
		sscanf(uri, "http://%*[^/]%s", path);
	}
	if((*port) == 0){
		*port = 80;
	}

}




/*
    Thread Safety
	open_clientfd_ts - calls thread safe gethostbyname_ts
*/
int open_clientfd_ts(char *hostname, int port)
{
 	/* From csapp */

    int clientfd;
    struct hostent *hp;
    struct sockaddr_in serveraddr;
    
    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		return -1;
	}

	/* Use thread-safe gethostbyname_ts */
    if ((hp = gethostbyname_ts(hostname)) == NULL){
	  return -2; /* check h_errno for cause of error */
	}
    
	bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;    
    bcopy((char *)hp->h_addr_list[0], 
    (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
    
	serveraddr.sin_port = htons(port);

    /* Establish a connection with the server */
  	if (connect(clientfd, (SA *) &serveraddr, sizeof(serveraddr)) < 0){
		return -1;
	}
    return clientfd;
}

/*
	clienterror - returns an error message to the client
*/
void clienterror(int fd, char *cause, char *errnum, 
	char *shortmsg, char *longmsg) 
{
	/* From csapp */

	char buffer1[MAXLINE], body[MAXBUF];

	/* Build the HTTP response body */
	sprintf(body, "<html><title>Tiny Error</title>");
	sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
	sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
	sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
	sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

	/* Print the HTTP response */
	sprintf(buffer1, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
	Rio_writen(fd, buffer1, strlen(buffer1));
	sprintf(buffer1, "Content-type: text/html\r\n");
	Rio_writen(fd, buffer1, strlen(buffer1));
	sprintf(buffer1, "Content-length: %d\r\n\r\n", (int)strlen(body));
	Rio_writen(fd, buffer1, strlen(buffer1));
	Rio_writen(fd, body, strlen(body));
}


/*
 	gethostbyname_ts - threadsafe version of gethostbyname
*/ 

struct hostent *gethostbyname_ts(char *hostname) {
  	struct hostent *host_ent;
  	/* Make it atomic instruction*/
  	P(&mutex);
  		host_ent = Gethostbyname(hostname);
  	V(&mutex);

	return host_ent;
}


/*
	reset - handles freeing of do_proxy
*/
void reset(char *buffer1,char *buffer2, char * method, char *request, 
	char *uri, char *hostname, char *path){
	free(buffer1); 
	free(buffer2); 
	free(method); 
	free(request); 
	free(uri); 
	free(hostname); 
	free(path);
}