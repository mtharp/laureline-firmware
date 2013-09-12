poly = 0xea

table = []
for n in range(256):
    for k in range(8):
        use_term = n & 1
        n >>= 1
        if use_term:
            n ^= poly
    table.append(n)


def print_table():
    out = 'static const uint16_t tbl_crc8[256] = {\n'
    for a in range(0, 256, 8):
        for b in range(a, a + 4):
            out += '0x%02x, ' % table[b]
        out += ' '
        for b in range(a + 4, a + 8):
            out += '0x%02x, ' % table[b]
        out = out[:-1] + '\n'
    out = out[:-2] + '};'
    print out


def crc8(data):
    c = 0xff
    for d in data:
        c = table[c ^ ord(d)]
        print '%02x %d' % (ord(d), c)
    return c


if __name__ == '__main__':
    import sys
    if len(sys.argv) > 1:
        #msg = sys.argv[1]
        msg = "\254\030\000\b\254\030\000\001\377\377\377\000\000\000\0"
        #print 'msg:', msg
        crc = crc8(msg)
        print 'crc: %02x' % crc
        msg += chr(crc)
        crc = crc8(msg)
        print 'crc: %02x' % crc
    else:
        print_table()
