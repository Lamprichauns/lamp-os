import micropython

# This helper is derived from python easing functions but only uses integer math
# to better support micropython. The LUTs are derived by calling the python easing lib
# and multiplying by 100
# lut = [0] * 101
# for i in range(101):
#    lut[i] = int(ease(0, 100, 100, i)*100)
quad_ease_in_out = [0, 2, 8, 18, 32, 50, 72, 98, 128, 162, 200, 242, 288, 338, 392, 450, 512, 578, 648, 722, 800, 881, 968, 1058, 1152, 1250, 1352, 1458, 1568, 1682, 1800, 1922, 2048, 2178, 2312, 2449, 2592, 2738, 2888, 3042, 3200, 3361, 3527, 3697, 3872, 4050, 4232, 4418, 4608, 4802, 5000, 5198, 5392, 5582, 5768, 5950, 6128, 6301, 6471, 6638, 6800, 6958, 7111, 7261, 7408, 7549, 7688, 7822, 7952, 8077, 8199, 8318, 8432, 8542, 8648, 8750, 8848, 8942, 9032, 9118, 9200, 9278, 9352, 9422, 9488, 9550, 9608, 9661, 9712, 9758, 9800, 9838, 9872, 9902, 9927, 9949, 9968, 9982, 9992, 9998, 10000]

linear = [0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000, 2100, 2200, 2300, 2400, 2500, 2600, 2700, 2800, 2899, 3000, 3100, 3200, 3300, 3400, 3500, 3600, 3700, 3800, 3900, 4000, 4100, 4200, 4300, 4400, 4500, 4600, 4700, 4800, 4900, 5000, 5100, 5200, 5300, 5400, 5500, 5600, 5699, 5799, 5900, 6000, 6100, 6200, 6300, 6400, 6500, 6600, 6700, 6800, 6900, 7000, 7100, 7200, 7300, 7400, 7500, 7600, 7700, 7800, 7900, 8000, 8100, 8200, 8300, 8400, 8500, 8600, 8700, 8800, 8900, 9000, 9100, 9200, 9300, 9400, 9500, 9600, 9700, 9800, 9900, 10000]

@micropython.native
def ease(start, end, duration, current_step, easing_function=None):
    lst = easing_function or quad_ease_in_out
    factor = (lst[(current_step*100//duration*100)//100])

    return ((end-start)*factor//10000)+start

@micropython.native
def pingpong_ease(start, middle, end, steps, current_step, easing_function_in = None, easing_function_out = None):
    loop_point = steps//2
    if current_step < loop_point:
        return ease(start, middle, loop_point, current_step, easing_function_in or quad_ease_in_out)

    return ease(middle, end, loop_point, current_step % loop_point, easing_function_out or quad_ease_in_out)
