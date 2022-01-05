class Behaviour:
    def __init__(self,lamp):
        self.lamp = lamp

    def __str__(self):
        return self.__class__.__name__
