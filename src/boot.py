# Boot script
import gc
import machine

# pylint: disable=no-member
gc.collect()
gc.threshold(gc.mem_free() // 4 + gc.mem_alloc())

machine.freq(240000000)
