import json
import tempfile
import time
from pathlib import Path
from invoke import task


@task
def erase(c, port):
    c.run(f"esptool.py --port {port} erase_flash")

@task
def flash(c, port):
    c.run(f"esptool.py --chip esp32 --port {port} write_flash -z 0x1000 esp32-20220618-v1.19.1.bin")
