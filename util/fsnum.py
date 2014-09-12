#!/usr/bin/python
import struct
import sys

hwver = 0x0701
serial = int(sys.argv[1])

print 'fsnum', (struct.pack('<H', hwver) + struct.pack('>Q', serial)[2:]).encode('hex')
print 'defaults'
