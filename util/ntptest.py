#!/usr/bin/python
import select
import socket
import time
import sys
import struct
from math import sqrt

host = sys.argv[1]
group = int(sys.argv[2])
assert group % 2 == 1
s = socket.socket(socket.SOCK_DGRAM, socket.AF_INET)
s.connect((host, 123))
poll = select.poll()
poll.register(s, select.POLLIN)

def ping(n=[0]):
    transmit = time.time() + 2208988800
    transmit_ntp = struct.pack('>Q', long(transmit * 2.0**32))
    d = '\xe3\x00\x03\xfa\x00\x01' '\x00\x00\x00\x01\x00\x00\x00\x00' '\x00\x00'
    d += struct.pack('>QQQ',
            0, # reference
            0, # origin
            0, # receive
            ) + transmit_ntp # transmit
    s.send(d)

    while True:
        if not poll.poll(100):
            return None, None
        result = s.recv(16384)
        if result[24:32] == transmit_ntp:
            break
    receive = time.time() + 2208988800
    delay = receive - transmit
    measured = struct.unpack('>Q', result[40:48])[0]
    measured /= 2.0 ** 32
    error = receive - measured - delay
    return error, delay


def pings():
    results = []
    for n in range(group):
        while True:
            error, delay = ping()
            if error is not None:
                break
        results.append((error, delay))
    results.sort()
    return results[len(results)/2]


def main():
    errors = []
    delays = []
    start = time.time()
    #print 'delay error'
    for n in range(150):
        error, delay = pings()
        delay *= 1e6
        error *= 1e6
        #print delay, error
        sys.stdout.flush()
        errors.append(error)
        delays.append(delay)
        #time.sleep(0.09)

    print
    print 'DELAY'
    delays.sort()
    mean = sum(delays) / len(delays)
    stdev = sqrt(sum((x - mean)**2 for x in delays)/len(delays))
    print 'min', delays[0]
    print '1qt', delays[len(delays)/4]
    print '2qt', delays[len(delays)*2/4]
    print '3qt', delays[len(delays)*3/4]
    print 'max', max(delays)
    print 'avg', mean
    print 'dev', stdev

    print
    print 'ERROR'
    errors.sort()
    mean = sum(errors) / len(errors)
    stdev = sqrt(sum((x - mean)**2 for x in errors)/len(errors))
    print 'min', errors[0]
    print '1qt', errors[len(errors)/4]
    print '2qt', errors[len(errors)*2/4]
    print '3qt', errors[len(errors)*3/4]
    print 'max', max(errors)
    print 'avg', mean
    print 'dev', stdev


main()
