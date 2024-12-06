import math

dac_resolution = 100
steps = 16*4
phases = [2 * math.pi / steps * i for i in range(steps)]
print([int(dac_resolution * math.cos(phase)) for phase in phases])
