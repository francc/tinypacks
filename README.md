TinyPacks
=========

TinyPacks is data serialization format for constrained environments like 8-bit
and 16-bit microcontrollers. It was designed to achieve these goals:

 * Easy to traverse and to skip entire nested elements.
 * Small serializated data footprint.
 * Small encoder/decoder memory footprint.
 * In-place parsing using static memory allocation.
 * Easy translation to and from JSON
 
For more information see the [README-TinyPacks.md](README-TinyPacks.md) file.


TinyPostman
===========

TinyPostman is a REST-like protocol for reading and writing data resources
from microcontrollers via a serial port. It is based on the TinyPacks data
serialization format and supports the REST methods GET, PUT, POST and DELETE.

For more information see the [README-TinyPostman.md](README-TinyPostman.md) file.
