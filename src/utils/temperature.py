# slice a temperature value into multiple indexes
# eg. 21C -> 20C[1, 2, 3, 4, 5]40C | outputs: 1
def get_temperature_index(temperature, minimum, maximum, number_stages):
    temperature_division = (maximum - minimum) / number_stages
    if temperature_division < 0:
        return 0

    index = int((temperature - minimum) / temperature_division)

    if index < 0:
        return 0
    if index > number_stages - 1:
        return number_stages - 1

    return index
