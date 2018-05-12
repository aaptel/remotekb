
all: sdlinject.so client

sdlinject.so: inject.c
	gcc -shared -std=c11 -Wall -Wextra -fPIC `sdl2-config --cflags` -ldl $^ -o $@


client: client.c
	gcc -Wall -Wextra  `sdl2-config --cflags --libs` $^ -o $@
