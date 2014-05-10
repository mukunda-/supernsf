# saturation_table[s] = saturate(sample+bias)

import struct
import sys

nchannels = int(sys.argv[1])

print "creating saturation table..."

if nchannels < 3:
	input_min = -64.0
	input_max = 63.0
elif nchannels == 3:
	input_min = -42.0
	input_max = 41.0
elif nchannels == 4:
	input_min = -32.0
	input_max = 31.0

convert = 127.0/(input_max-input_min)

output = open("python/saturation_table.bin","wb")

for sample in range(0,256):
	# add bias
	value = sample
	if( value >= 128 ):
		value = value-256
	else:
		value = value
	
	# saturate
	if( value < input_min ):
		value = input_min
	if( value > input_max ):
		value = input_max
	value -= input_min
	
	value *= convert
	value = round(value)
	
	out = struct.pack("b", int(value))
	output.write(out)

output.close()

