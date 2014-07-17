#
# Copyright (c) Michael Tharp <gxti@partiallystapled.com>
#
# This file is distributed under the terms of the MIT License.
# See the LICENSE file at the top of this tree, or if it is missing a copy can
# be found at http://opensource.org/licenses/MIT
#

import struct
import sys


msg_cls = int(sys.argv[1], 16)
msg_id = int(sys.argv[2], 16)
data = [int(x, 16) for x in sys.argv[3:]]
len_words = struct.unpack('BB', struct.pack('<H', len(data)))
words = [msg_cls, msg_id] + list(len_words) + data

ck1 = ck2 = 0
for word in words:
    ck1 = (ck1 + word) & 0xFF
    ck2 = (ck2 + ck1) & 0xFF

words = [0xB5, 0x62] + words + [ck1, ck2]
while words:
    chunk, words = words[:8], words[8:]
    print ' '.join('0x%02X,' % x for x in chunk)
