import subprocess
import sys

elf = sys.argv[1]
if len(sys.argv) > 2:
    base = 0x08000000
    prefix = '08'
    desc = 'ROM'
else:
    base = 0x20000000
    prefix = '20'
    desc = 'RAM'
p = subprocess.Popen(['readelf', '-s', elf], stdout=subprocess.PIPE)
symbols = {}
byaddr = {}
for line in p.communicate()[0].splitlines():
    if ': ' not in line or 'Num:' in line:
        continue
    line = line.split(':', 1)[1].split()
    addr = line[0]
    size = line[1]
    if len(line) >= 7:
        name = line[6]
    else:
        name = '<none>'
    if not addr.startswith(prefix):
        continue
    byaddr[addr] = (name, size)
    size = int(size)
    if not size:
        continue
    symbols[name] = symbols.get(name, 0) + size
print 'addr     symbol'
lastaddr = 0
for addr, (symbol, size) in sorted(byaddr.items()):
    size = int(size)
    if size:
        lastaddr = int(addr, 16) + size
    print addr, symbol
print
print 'size     symbol'
for symbol, size in sorted(symbols.items(), key=lambda x: x[1]):
    print '%8d %s' % (size, symbol)
print
print 'total %s:' % desc, lastaddr - base
