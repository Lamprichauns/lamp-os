import utime

# pylint: disable=unused-argument
def timed_function(f, *args, **kwargs):
    '''
    Handy little method for printing profiling info:
    @timed_function
    def some_method:
        print('time me!')
    '''
    myname = str(f).split(' ')[1]
    def new_func(*args, **kwargs):
        t = utime.ticks_us()
        result = f(*args, **kwargs)
        delta = utime.ticks_diff(utime.ticks_us(), t)
        print('Function {} Time = {:6.3f}ms'.format(myname, delta/1000))
        return result
    return new_func
