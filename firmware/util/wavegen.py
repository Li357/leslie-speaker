import math

dac_resolution = 185
steps = 16*4
phases = [2 * math.pi / steps * i for i in range(steps)]
print([int((dac_resolution / 2) * math.cos(phase) + (dac_resolution / 2)) for phase in phases])
