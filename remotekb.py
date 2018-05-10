#!/usr/bin/python3

import socketserver
import os
import sys
import re
import subprocess
import argparse
from pprint import pprint as P
import time
import pygame


HOST, PORT = "localhost", 9999

def main():
    ap = argparse.ArgumentParser(description="remote kb")
    ap.add_argument("-s", "--server", action="store_true", help="start server")
    args = ap.parse_args()
    if args.server:
        start_server()
    else:
        start_client()

def key_down(k):
    print("xdotool kd "+k)
    subprocess.call(['xdotool', 'keydown', k])

def key_up(k):
    print("xdotool ku "+k)
    subprocess.call(['xdotool', 'keyup', k])

class RemoteKbHandler(socketserver.BaseRequestHandler):
    def handle(self):
        print("{} connected".format(self.client_address[0]))
        data = self.request[0].strip().decode('utf-8')
        if data[0] == 'd':
            key_down(data[1])
        elif data[0] == 'u':
            key_up(data[1])
        else:
            return


class ClientSock:
    def __init__(h, p):
        self.host = h
        self.port = p
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    def send(s):
        self.sock.sendto(bytes(s, "utf-8"), (self.host, self.port))

    def key_up(k):
        self.send("u"+chr(k))

    def key_down(k):
        self.send("d"+chr(k))


def start_server():
    # Create the server, binding to localhost on port 9999
    server = socketserver.UDPServer((HOST, PORT), RemoteKbHandler)
    server.allow_reuse_address = True
    # Activate the server; this will keep running until you
    # interrupt the program with Ctrl-C
    server.serve_forever()


def start_client():
    sock = ClientSock(HOST, PORT)

    background_colour = (255,255,255)
    (width, height) = (300, 200)
    screen = pygame.display.set_mode((width, height))
    pygame.display.set_caption('remote kb client')
    screen.fill(background_colour)
    pygame.display.flip()
    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                print("send kd %d", even.key)
                sock.key_down(event.key)
            elif event.type == pygame.KEYUP:
                print("send ku %d", even.key)
                sock.key_up(event.key)

if __name__ == '__main__':
    main()
