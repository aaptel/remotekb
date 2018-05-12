# SDL remote keyboard

This software allows you to send SDL keyboard events from multiple
client machines to any SDL-based program over the network, as if all
the keyboards were connected to the server machine.

# How does it work?

The server is implemented as a shared library that is "injected" in
the SDL program. This lib overrides SDL event polling function so that
it can insert keyboard events from the network before calling the
original function.

The injection process is automatically handled by the `remotekb_wrap`
helper script.

On the client side we run a very simple SDL program that just opens a
window, gather inputs and sends them over the network.

The network protocol is extrememy basic. It works over UDP and simply
sends SDL keyboard-type events raw over the network (literally copying
the bytes of the `SDL_Event` structure on the socket). The keyboard
events are only inserted inside the SDL program and it is not possible
to interact with anything outside of it (like the window manager or
other windows). However there is no authentification or encryption,
use at your own risk.

# Compilation

You will need the development files for SDL2 to compile this.

    $ make

# Usage

On the server machine, we run our SDL program via the wrapper script:

    $ remotekb_wrap -p 4321 ./my_super_game --some option
    RKB init_all:73 init injection...
    RKB server_func:158 starting udp server on port 1234
    RKB server_func:169 waiting for packet...

On a client machine, we run the client program:

    $ ./client 192.168.1.42 4321
	RKB sdl_window:80 connecting...

Typing in a client program will now send the events to the server program:

    (CLIENT) RKB sdl_window:114 sending key event down 40
    (CLIENT) RKB sdl_window:114 sending key event up 40
    (SERVER) RKB server_func:187 127.0.0.1 sent 56 bytes
    (SERVER) RKB server_func:169 waiting for packet...
    (SERVER) RKB server_func:187 127.0.0.1 sent 56 bytes
    (SERVER) RKB server_func:169 waiting for packet...


You can chose the exact version of SDL to override for the program
using ldd and the -l option of the wrapper script:

    $ ldd my-game | grep SDL
            libSDL2-2.0.so.0 => /usr/lib64/libSDL2-2.0.so.0 (0x00007f2510ce4000)
	$ remotekb_wrap -l libSDL2-2.0.so.0 ./my_super_game --some option
