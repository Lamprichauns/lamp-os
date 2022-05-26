# merges b into a to a depth of 1
def merge_configs(a, b):
    if isinstance(b, dict):
        for key in b:
            try:
                if isinstance(a[key], dict):
                    a[key].update(b[key])
            except KeyError:
                pass
