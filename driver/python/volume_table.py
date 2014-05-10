
# volume_table[sample,volume] = (signed)(sample * volume)

# 1ch output range: [ 0,127]
# 2ch output range: [-64,63]
# 3ch output range: [-42,41]
# 4ch output range: [-32,31]

import struct
import sys

print "creating volume table..."

nchannels = int(sys.argv[1])

if nchannels == 1:
	output_min = 0
	output_max = 127
elif nchannels == 2:
	output_min = -64
	output_max = 63
elif nchannels == 3:
	output_min = -42
	output_max = 41
elif nchannels == 4:
	output_min = -32
	output_max = 31

sample_range = output_max - output_min
sample_range = sample_range * 1.0

output = open("python/volume_table.bin","wb")

for volume in range(0,32):
	for sample in range(0,128):
		realvol = volume/32.0
		if( volume == 31 ):
			realvol = 1.0
		sample2 = ((sample - 64.0) * realvol) + 64.0
		sample2 = round(output_min + (sample2 * sample_range / 127.0))
		out = struct.pack("b", int(sample2))
		output.write(out)

output.close()

