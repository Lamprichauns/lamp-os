# Boot script
import machine
import gc
import network

gc.threshold(gc.mem_free() // 4 + gc.mem_alloc())
machine.freq(240000000)

webrplEnable = True

if webrplEnable:
    ap = network.WLAN(network.AP_IF)
    ap.active(True)
    ap.config(essid="lamp-debug", password="password")

    import webrepl
    webrepl.start()