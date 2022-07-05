#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Run an OSC server with asynchronous I/O handling via the uasync framework.
"""
import socket
import uasyncio as asyncio
from uasyncio.core import get_event_loop
from vendor.uosc.server import handle_osc

loop = get_event_loop()
MAX_DGRAM_SIZE = 1472

class Log():
    def exc(self, e, _):
        print(e)
    def error(self, e):
        print(e)
    def warning(self, e):
        print(e)
    def info(self, e):
        print(e)
    def debug(self, e):
        print(e)

log = Log()

async def run_server(host, port, client_coro, **params):
    log.debug("Running OSC Server")
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setblocking(False)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((host, port))

    try:
        while True:
            try:
                data, caddr = sock.recvfrom(MAX_DGRAM_SIZE)
                client_coro(data, caddr, **params)
            except:
                pass

            await asyncio.sleep(0)

    except Exception as e:
        print(e)

class OSCMessage:
    def __init__(self):
        self.count = 0
        self.address = ""
        self.type = ""
        self.arguments = ""

    def __call__(self, t, msg):
        self.count += 1
        self.address = msg[0]
        self.type = msg[1]
        self.arguments = msg[2]

async def async_osc(host="127.0.0.1", port=57121, dispatch=None):
    coroutine = run_server(host, port, client_coro=handle_osc, dispatch=dispatch)
    loop.create_task(coroutine)
    loop.run_forever()
