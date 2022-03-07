import json
import tempfile
import time
from pathlib import Path
from invoke import task

# The time stamp here is used to determine which files have
# changed to allow the flash task to skip ones that haven't
# since the last flash. This file is stored on device
LAST_UPLOADED_TIMESTAMP = ".last-uploaded-at"

# File that holds hardware to lamp pairs for ease of flashing
LAMP_MAPPING = "lamp-mapping.json"

VERSION_FILE = "VERSION"

@task
def clean(c):
    '''
    Clean interim build files.
    '''
    c.run('find . | grep -E "(__pycache__|\.pyc|\.pyo$|.pytest_cache|.mypy_cache)" | xargs rm -rf')
    print("Cleaned test files.")

@task
def configure(c, lamp, port=None):
    '''
    Sets the configuration file (config.json) for specific lamps
    on a target lamp.
    '''
    print(f"Setting configuration to: {lamp}")

    config = {
        "name": _to_upper_camel_case(lamp),
        "module": f"app.lamps.{lamp}",
        "version": _read_first_line(VERSION_FILE)
    }

    if port is None:
        port = _port_from_lamp(lamp)

    with tempfile.NamedTemporaryFile(mode="w+") as file:
        json.dump(config, file)
        file.flush()
        c.run(f"ampy --port {port} put {file.name} config.json")

@task 
def flash(c, lamp, port=None, reboot=True):
    '''
    Flashes all the files need to run the lamp. Will look for the
    `.last-uploaded-at` file to attempt to do progressive uploading
    to speed things up.
    '''
    if port is None:
        port = _port_from_lamp(lamp)

    print(f"Uploading to {port}...")

    last_uploaded_at = _read_uploaded_timestamp(c, port)

    def upload_file_if_changed(path):
        file_modified_at = path.stat().st_mtime
        dest_path = Path(*path.parts[1:])
        if file_modified_at > last_uploaded_at:
            c.run(f"ampy --port {port} put {path} {dest_path}", echo=True)

    if last_uploaded_at == 0:
        # This is a bit of a hack to make sure we don't upload
        # interim build files on fresh installs. Incremental
        # builds only upload python files.
        clean(c)
        c.run(f"ampy --port {port} put src /", echo=True)
    else:
        for path in Path('src').rglob('*.py'):
            upload_file_if_changed(path)

    configure(c, lamp, port)

    if reboot:
        c.run(f"ampy --port {port} reset --hard", hide=True)

    _write_uploaded_timestamp(c, port)
    print("Done!")

@task
def run(c, lamp, port=None, dont_flash=False):
    '''
    Flash and run the target lamp on device.

    To ease use you can map your lamp to a board name in the
    `lamp-mapping.json` so you can just run `inv flash [lamp].
    '''
    if port is None:
        port = _port_from_lamp(lamp)

    if not dont_flash:
        flash(c, lamp, port=port, reboot=False)
    print("----- We Love Lamp -----")
    c.run(f"ampy --port {port} run src/main.py", hide=False)

@task
def wipe(c, lamp, port=None):
    '''
    Clear off all the files from a micropython device.
    '''
    if port is None:
        port = _port_from_lamp(lamp)

    print(f"Delete files on: {port}")

    result = c.run(f"ampy --port {port} ls", hide=True)
    lines = result.stdout.splitlines()
    for line in lines:
        try:
            c.run(f"ampy --port {port} rm {line}", hide=True)
        except:
            c.run(f"ampy --port {port} rmdir {line}", hide=True)
    print("Wiped!")

@task
def test(c):
    '''
    Run all the test scripts in `./tests`
    '''
    c.run("PYTHONPATH=./src pytest --asyncio-mode=auto -q tests/*", pty=True)

def _port_from_lamp(lamp):
    with open(LAMP_MAPPING) as file:
        config = json.load(file)
        return config[lamp]

def _to_upper_camel_case(snake_str):
    components = snake_str.split('_')
    return ''.join(x.title() for x in components)

def _write_uploaded_timestamp(c, port):
    now = time.time()
    with tempfile.NamedTemporaryFile(mode="w") as file:
        file.write(str(now))
        file.flush()
        c.run(f"ampy --port {port} put {file.name} {LAST_UPLOADED_TIMESTAMP}", hide=False)

def _read_uploaded_timestamp(c, port):
    with tempfile.NamedTemporaryFile(mode="r", delete=False) as file:
        try:
            c.run(f"ampy --port {port} get {LAST_UPLOADED_TIMESTAMP} {file.name}", hide=True)
            return float(file.read())
        except:
            return 0

def _read_first_line(filename):
    with open(filename) as file:
        return file.readline()
