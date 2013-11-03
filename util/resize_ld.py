#!/usr/bin/env python2
#
# Copyright (c) Michael Tharp <gxti@partiallystapled.com>
#
# This file is distributed under the terms of the MIT License.
# See the LICENSE file at the top of this tree, or if it is missing a copy can
# be found at http://opensource.org/licenses/MIT
#


import os
import re
import subprocess
import sys
import traceback

ALIGN = 16
PAGE_SIZE = 2048
replace_re = r'(flash\s*\(rx\)\s*:\s*org = 0x[0-9a-fA-F]+) \+ \S+ - \S+, len = \S+'


def main():
    flash_size = sys.argv[1]
    args = sys.argv[2:]
    new_args = []
    script = None
    while args:
        arg = args.pop(0)
        if arg.startswith('-T'):
            if len(arg) > 2:
                script = arg[2:]
            else:
                script = args.pop(0)
            continue
        elif arg.startswith('-o'):
            if len(arg) > 2:
                output = arg[2:]
            else:
                output = args.pop(0)
            continue
        new_args.append(arg)

    # First pass: use the default sizes
    tmp_file = output + '.tmp'
    if os.path.exists(tmp_file):
        os.unlink(tmp_file)
    link(new_args + ['-o', tmp_file, '-T', script])
    if not os.path.exists(tmp_file):
        sys.exit("%s was not created" % tmp_file)

    # Figure out how much flash was used
    used = 0
    for size, name in get_sections(tmp_file):
        if 'startup' in name:
            continue
        # Assume alignment of 16
        size = (size + ALIGN - 1) // ALIGN * ALIGN
        print size, name
        used += size
    os.unlink(tmp_file)

    # Determine precise load address
    used = (used + PAGE_SIZE - 1) // PAGE_SIZE * PAGE_SIZE
    if flash_size.lower().endswith('k'):
        flash_size = int(flash_size[:-1]) * 1024
    else:
        flash_size = int(flash_size)
    start = flash_size - used

    # Second pass with precise sizes
    new_script = output + '.ld'
    fin = open(script)
    fout = open(new_script, 'w')
    found = False
    for line in fin:
        m = re.search(replace_re, line)
        if m:
            found = True
            base, = m.groups()
            line = re.sub(
                    replace_re, base + ' + %s, len = %s' % (start, used),
                    line)
        fout.write(line)
    fout.close()
    fin.close()
    if not found:
        sys.exit("Unable to find memory region to replace")

    if os.path.exists(output):
        os.unlink(output)
    link(new_args + ['-o', output, '-T', new_script])


def link(args):
    executable = sys.argv[2]
    if not os.path.sep in executable:
        for search in os.environ.get('PATH', '').split(os.pathsep):
            path = os.path.join(search, executable)
            if os.path.exists(path):
                executable = path
                break
        else:
            sys.exit("couldn't find %s" % executable)

    print 'Running:', ' '.join(args)
    pid = os.fork()
    if pid:
        status = os.waitpid(pid, 0)[1]
        if status:
            print >> sys.stderr, "link exited with status", status
            sys.exit(status)
    else:
        try:
            os.execv(executable, args)
        except:
            traceback.print_exc()
        finally:
            os._exit(70)


def get_sections(infile):
    p = subprocess.Popen(['readelf', '-SW', infile], stdout=subprocess.PIPE)
    sections = []
    for line in p.communicate()[0].splitlines():
        if ']' not in line:
            continue
        line = line.split(']')[1].split()
        if line[1] == 'Type':
            continue
        name = line[0]
        prog = line[1]
        if prog != 'PROGBITS':
            continue
        size = int(line[4], 16)
        flags = line[6]
        if 'A' not in flags:
            continue
        sections.append((size, '%s(%s)' % (infile, name)))
    return sections


main()
