import json
import tempfile
import time
import os
from pathlib import Path
from invoke import task


@task
def erase(c, port):
    c.run(f"esptool.py --port {port} erase_flash")

@task
def flash(c, port):
    c.run(f"esptool.py --chip esp32 --port {port} write_flash -z 0x1000 esp32-20230426-v1.20.0.bin")

@task
def upload(c, port): 
    for path in Path('src').rglob('*'):
        dest_path = Path(*path.parts[1:])

        if os.path.isdir(path):
            c.run(f"ampy -d 1 --port {port} --baud 115200 mkdir {dest_path} ", echo=True)
        else:
            c.run(f"ampy -d 1 --port {port} --baud 115200 put {path} {dest_path}", echo=True)
    print("Done!")

@task 
def update(c, port, lamp): 
    c.run(f"ampy --port {port} put src/main.py main.py", echo=True)    
    c.run(f"ampy --port {port} put src/lamps/{lamp}.py lamps/{lamp}.py", echo=True)
    print("Done!")  

@task
def setup(c, port):
    erase(c,port)
    flash(c,port)
    upload(c,port)
    