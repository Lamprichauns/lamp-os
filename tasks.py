from invoke import task

@task 
def flash(c, port, lamp):
    c.run(f"ampy --port {port} put ./src/lamp.py lamp.py")
    c.run(f"ampy --port {port} put ./src/lamps/{lamp}.py main.py")
    print(f"{lamp} flashed to %{port}")
    