#!/usr/bin/python
#
# TinyPacks pack/unpack example
#

import tinypacks

data = {"text": "Hello world!", "status": True, "count": 123}
print("Data: " +  repr(data))

packed_data = tinypacks.pack(data)
print("Packed: " + repr(packed_data))

(unpacked_data, remaining_data) = tinypacks.unpack(packed_data)
print("Unpacked: " + repr(unpacked_data))
