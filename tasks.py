from invoke import task
import glob, os

@task 
def flash(c, port, lamp):
    for filename in glob.glob('src/*.py'):
        file = os.path.basename(filename)
        if file == "main.py":
            continue

        c.run(f"ampy --port {port} put {filename} {file} ") 
        print(f"{filename} flashed to %{port}")

    lampfile = os.path.join("src","lamps",f"{lamp}.py")
    c.run(f"ampy --port {port} put {lampfile} main.py")

    print(f"{lamp} flashed to %{port}")
    