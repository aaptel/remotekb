#define _POSIX_C_SOURCE 200809L

#include <SDL.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define D(fmt, ...) (fprintf(stderr, "RKB %s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__))
#define E(fmt, ...) (fprintf(stderr, "RKB %s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__), exit(1))

#define EPE(fmt, ...) (fprintf(stderr, "RKB %s:%d " fmt "(%s)\n", __func__, __LINE__, ##__VA_ARGS__, strerror(errno)), exit(1))

#define PE(fmt, ...) (fprintf(stderr, "RKB %s:%d " fmt "(%s)\n", __func__, __LINE__, ##__VA_ARGS__, strerror(errno)))


#define QSIZE 128
#define QCMDSIZE 16

typedef struct {
	char cmd[QSIZE][QCMDSIZE];
	int size;
} equeue_t;

int get_socket (const char *host, const char *port)
{
	struct addrinfo *addr, *info;
	struct addrinfo hints;
	int res;
	int sock;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;

	res = getaddrinfo(host, port, &hints, &info);
	if (res != 0)
		PE("getaddrinfo");

	for (addr = info; addr; addr = addr->ai_next) {
		sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
		if (sock < 0)
			continue;
		res = connect(sock, addr->ai_addr, addr->ai_addrlen);
		if (res != 0) {
			PE("connect");
			close(sock);
		} else {
			break;
		}
	}

	if (addr == NULL)
		E("could not connect");

	freeaddrinfo(info);
	return sock;
}

void sdl_window (const char *host, const char *port)
{
	int r;
	SDL_Window *win;
	SDL_Renderer *rdr;
	SDL_Event ev;
	int w = 200;
	int h = 150;
	int sockfd;

	D("connecting...");
	sockfd = get_socket(host, port);

	r = SDL_Init(SDL_INIT_VIDEO);
	if (r < 0)
		E("sdl init failed");

	win = SDL_CreateWindow("remotekb", 100, 100, w, h, SDL_WINDOW_SHOWN);
	if (!win) {
		D("sdl create window failed (%s)", SDL_GetError());
		goto err_1;
	}


	rdr = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
	if (!rdr) {
		D("sdl create renderer failed (%s)", SDL_GetError());
		goto err_2;
	}


	while (1) {
		r = SDL_WaitEvent(&ev);
		if (r == 0) {
			D("sdl wait event failed (%s)", SDL_GetError());
			goto err_2;
		}
		if (ev.type == SDL_QUIT) {
			D("quitting...");
			goto err_2;
		}
		if (ev.type == SDL_KEYUP || ev.type == SDL_KEYDOWN) {
			D("sending key event %s %d",
			  ev.type == SDL_KEYUP ? "up" : "down",
			  ev.key.keysym.scancode);
			write(sockfd, &ev, sizeof(ev));
		}
	}
err_2:
	SDL_DestroyWindow(win);

err_1:
	SDL_Quit();

}

int main (int argc, char **argv)
{
	if (argc != 3) {
		E("Usage: %s HOST PORT", argv[0]);
	}

	sdl_window(argv[1], argv[2]);
	return 0;
}
