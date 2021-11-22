from time import sleep

class Gestures:
    # Reset to the configured color
    def reset(self): 
        self.pixels.fill(self.color)
        self.pixels.write()    
        sleep(0.5)