import pymic as mic
import numpy as np
import sys


if len(sys.argv) > 1 :
    m = int(sys.argv[1])
    n = int(sys.argv[2])
else :
	m, n = 5, 6

np.random.seed(10)
a = np.random.random(m * n).reshape((m, n))
np_result = np.sum(a)
r = np.empty(1)

device = mic.devices[0]
stream = device.get_default_stream()

offl_a = stream.bind(a)
offl_r = stream.bind(r)
stream.sync()

offl_r = offl_a.sum
result = offl_r.update_host().array
stream.sync()

print("--------------------------------")
print("Sum with default axis")
print("Checksum :")
print(result)
print(np_result)
print("--------------------------------")



