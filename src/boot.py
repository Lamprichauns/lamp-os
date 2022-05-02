# Boot script
import gc
import machine
import network

# pylint: disable=no-member
gc.threshold(gc.mem_free() // 4 + gc.mem_alloc())
machine.freq(240000000)

# pylint: disable=invalid-name
webrplEnable = False

if webrplEnable:
    ap = network.WLAN(network.AP_IF)
    ap.active(True)
    ap.config(essid="lamp-debug", password="password")

    import webrepl
    webrepl.start()
