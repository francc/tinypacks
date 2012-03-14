#!/usr/bin/python
#
#  json2tinypacks - Copyright (c) 2012 Francisco Castro <http://fran.cc>
#
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in
#  all copies or substantial portions of the Software.
# 
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#  DEALINGS IN THE SOFTWARE.

import sys
import json
import tinypacks

if len(sys.argv) < 3:
    print("\nUsage: %s <JSON file> <TinyPacks file>\n" % sys.argv[0])
    sys.exit()

source_file = open(sys.argv[1], "r")
destination_file = open(sys.argv[2], "w")

parsed_json = json.load(source_file)
packed_tinypacks = tinypacks.pack(parsed_json)
destination_file.write(packed_tinypacks)

source_file.close()
destination_file.close()

