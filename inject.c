
#define _GNU_SOURCE
#include <dlfcn.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <SDL.h>

#define D(fmt, ...) (fprintf(stderr, "RKB %s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__))
#define E(fmt, ...) (fprintf(stderr, "RKB %s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__), exit(1))

#define QSIZE 128

typedef struct {
	SDL_Event ev[QSIZE];
	int size;
} equeue_t;

typedef int (*sdlpoll_t)(SDL_Event* ev);
equeue_t global_eq;
sdlpoll_t global_org_poll;
int global_portno = 1234;
pthread_mutex_t lock;
pthread_t server_thread;
void *global_libsdl;
int global_init = 0;

static void init_all (void);
static void* server_func(void *p);

static void eq_add_cmd(equeue_t *eq, const uint8_t *buf, int len)
{
	int i;
	pthread_mutex_lock(&lock);
	int nbevents = len / sizeof(eq->ev[0]);
	int left = len % sizeof(eq->ev[0]);
	int off = 0;
	if (left != 0) {
		D("skipping incomplete recvd event... (skipping %d bytes)", left);
	}
	for (i = 0; i < nbevents; i++) {
		memset(&eq->ev[eq->size], 0, sizeof(eq->ev[0]));
		memcpy(&eq->ev[eq->size], buf+off, sizeof(eq->ev[0]));
		off += sizeof(eq->ev[0]);
		eq->size++;
	}
	pthread_mutex_unlock(&lock);
}

static void eq_inject_sdl_event(equeue_t *eq)
{
	int i;
	pthread_mutex_lock(&lock);
	for (i = 0; i < eq->size; i++) {
		SDL_PushEvent(&eq->ev[i]);
	}
	eq->size = 0;
	pthread_mutex_unlock(&lock);
}


static void init_all (void)
{
	const char* x;
	const char *sdlpath = "/usr/lib64/libSDL2-2.0.so.0";

	D("init injection...");

	x = getenv("REMOTEKB_PORT");
	if (x) {
		global_portno = atoi(x);
	}

	x = getenv("REMOTEKB_LIBSDL_PATH");
	if (x) {
		sdlpath = x;
	}

	global_libsdl = dlopen(sdlpath, RTLD_NOW);
	if (global_libsdl == NULL) {
		E("dlopen");
	}

	global_org_poll = (sdlpoll_t)dlsym(RTLD_NEXT,"SDL_PollEvent");
	if (global_org_poll == NULL) {
		E("dlsym");
	}

	if (pthread_mutex_init(&lock, NULL) != 0) {
		E("mutex init");
	}

	pthread_create(&server_thread, NULL, server_func, NULL);
	global_init = 1;
}


#define BUFSIZE 1024

static void* server_func(void *p)
{
	int sockfd;		/* socket */
	socklen_t clientlen;		/* byte size of client's address */
	struct sockaddr_in serveraddr;	/* server's addr */
	struct sockaddr_in clientaddr;	/* client addr */
	struct hostent *hostp;	/* client host info */
	char *hostaddrp;	/* dotted decimal host addr string */
	int optval;		/* flag value for setsockopt */
	int n;			/* message byte size */
	int portno = global_portno;
	uint8_t buf[BUFSIZE];

	/* unused */
	(void)p;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
		E("socket failed");

	/* setsockopt: Handy debugging trick that lets
	 * us rerun the server immediately after we kill it;
	 * otherwise we have to wait about 20 secs.
	 * Eliminates "ERROR on binding: Address already in use" error.
	 */
	optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
		   (const void *)&optval, sizeof(int));

	/*
	 * build the server's Internet address
	 */
	bzero((char *)&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((unsigned short)portno);

	/*
	 * bind: associate the parent socket with a port
	 */
	if (bind(sockfd, (struct sockaddr *)&serveraddr,
		 sizeof(serveraddr)) < 0)
		E("bind failed");

	D("starting udp server on port %d", portno);

	clientlen = sizeof(clientaddr);
	while (1) {
		D("waiting for packet...");
		n = recvfrom(sockfd, buf, BUFSIZE, 0,
			     (struct sockaddr *)&clientaddr, &clientlen);
		if (n < 0)
			E("recvfrom failed");

		/*
		 * gethostbyaddr: determine who sent the datagram
		 */
		hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
				      sizeof(clientaddr.sin_addr.s_addr),
				      AF_INET);
		if (hostp == NULL)
			E("gethostbyaddr failed");
		hostaddrp = inet_ntoa(clientaddr.sin_addr);
		if (hostaddrp == NULL)
			E("inet_ntoa failed");

		D("%s sent %d bytes", hostaddrp, n);

		eq_add_cmd(&global_eq, buf, n);

		/*
		 * sendto: echo the input back to the client
		 */
		/* n = sendto(sockfd, buf, n, 0, */
		/* 	   (struct sockaddr *)&clientaddr, clientlen); */
		/* if (n < 0) */
		/* 	error("ERROR in sendto"); */
	}
	return NULL;
}


int SDL_PollEvent(SDL_Event *ev)
{
	if (!global_init)
		init_all();

	eq_inject_sdl_event(&global_eq);
	return global_org_poll(ev);
}
