import struct
import sys


def make_table(poly, bits):
    table = []
    for n in range(256):
        for k in range(8):
            use_term = n & 1
            n >>= 1
            if use_term:
                n ^= poly
        table.append(n)
    return table


def print_table(poly, bits):
    table = make_table(poly, bits)
    if bits <= 8:
        width = 2
        ctype = 'uint8_t'
    elif bits <= 16:
        width = 4
        ctype = 'uint16_t'
    elif bits <= 32:
        width = 8
        ctype = 'uint32_t'
    elif bits <= 64:
        width = 16
        ctype = 'uint64_t'
    else:
        sys.exit('the bits are too big')
    print 'static const %s crc%d_%x[256] = {' % (ctype, bits, poly)
    for a in range(0, 256, 8):
        out = ''
        for b in range(a, a + 4):
            out += '0x%0*x, ' % (width, table[b])
        out += ' '
        for b in range(a + 4, a + 8):
            out += '0x%0*x, ' % (width, table[b])
        out = out[:-1]
        print out
    print '};'


def crc(poly, bits, data):
    table = make_table(poly, bits)
    c = (1<<bits)-1
    for d in data:
        if bits > 8:
            c = table[(c ^ ord(d)) & 0xff] ^ (c >> 8)
        else:
            c = table[c ^ ord(d)]
    return c


if __name__ == '__main__':
    if len(sys.argv) != 3:
        print 'usage: %s bits poly'
        print 'ex: %s 8 0xEA'
        sys.exit(1)
    bits, poly = sys.argv[1:]
    bits = int(bits)
    if poly.lower().startswith('0x'):
        poly = poly[2:]
    poly = int(poly, 16)
    print_table(poly, bits)

    msg = "beefcake"
    n = crc(poly, bits, msg)
    print hex(n)
    if bits <= 8:
        msg += chr(n)
    elif bits <= 16:
        msg += struct.pack('<H', n)
    elif bits <= 32:
        msg += struct.pack('<I', n)
    else:
        msg += struct.pack('<Q', n)
    n = crc(poly, bits, msg)
    print hex(n)
