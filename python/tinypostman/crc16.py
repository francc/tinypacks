'''Packages: maths;miscellaneous'''

# crc.py
# Various CRC routines.

# Exports:
# crc16str(string)  Return the CRC value of a string.


def crc16str(s):
    crc = 0L
    for index1 in range(len(s)):
        crc = crc ^ (ord(s[index1]) << 8)
        for index2 in range(1, 9):
            if crc & 0x8000 != 0:
                crc = ((crc << 1) ^ 0x1021)
            else:
                crc = crc << 1
    return crc & 0xFFFF


if __name__ == '__main__':

    print '%X' % crc16str('\002\001\000\000')

