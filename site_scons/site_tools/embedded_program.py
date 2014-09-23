#
# Copyright (c) Michael Tharp <gxti@partiallystapled.com>
#
# This file is distributed under the terms of the MIT License.
# See the LICENSE file at the top of this tree, or if it is missing a copy can
# be found at http://opensource.org/licenses/MIT
#

import os
from SCons.Builder import Builder
from SCons.Util import is_String


def SetARMFlags(env, mcpu):
    assert mcpu == 'cortex-m3'
    env['MCPU'] = mcpu
    env['MCFLAGS'] = '-mcpu=$MCPU -mthumb'
    if env.get('DEBUG'):
        env['CFLAGS_OPT'] = '-O0'
    else:
        env['CFLAGS_OPT'] = '-Os -fomit-frame-pointer'
    env['CFLAGS_GC'] = '-ffunction-sections -fdata-sections -fno-common -falign-functions=16'
    env['CFLAGS_DBG'] = '-ggdb3'
    env['CFLAGS_WARN'] = '-Wall -Wextra -Wstrict-prototypes'
    env['CFLAGS_WARN'] += ' -Wno-unused-parameter -Wno-main -Wno-address'
    env['CFLAGS_USER'] = ''
    env['LDFLAGS_GC'] = '-Wl,--gc-sections'

    env['CFLAGS'] = '$MCFLAGS $CFLAGS_OPT $CFLAGS_GC $CFLAGS_DBG $CFLAGS_WARN $CFLAGS_USER'
    env['ASFLAGS'] = '$MCFLAGS'
    env['LINKFLAGS'] = '$MCFLAGS $LDFLAGS_GC'


def EmbeddedProgram(env, target, sources, script, with_hex=True, **kwargs):
    target = _node(env, target)
    script = _node(env, script)
    mapfile = target.target_from_source('', '.map')
    flags = kwargs.get('LINKFLAGS', '$LINKFLAGS')
    flags += ' -nostartfiles -T%s -Wl,-Map=%s,--cref' % (script, mapfile.path)
    kwargs['LINKFLAGS'] = flags
    elf = env.Program(target, sources, **kwargs)
    env.Depends(elf, script)
    env.Clean(elf, mapfile)
    if with_hex:
        hexfile = target.target_from_source('', '.hex')
        ihex = CopyObject(env, hexfile, target, format='ihex',
                strip_sections=['.boot_stub'])
    else:
        ihex = []
    return elf + ihex


def CopyObject(env, target, source, format=None, strip_sections=None):
    flags = ''
    if format:
        flags += ' -O %s' % format
    if strip_sections:
        for section in strip_sections:
            flags += ' -R %s' % section
    return env.Objcopy(target, source, COPYFLAGS=flags)


def GDBInstall(env, source):
    if env.get('BMP'):
        cmd = env.Command('$BMP', source,
            '$GDB --batch'
                ' -ex "tar ext $TARGET"'
                ' -ex "mon connect_srst enable"'
                ' -ex "mon swdp_scan"'
                ' -ex "att 1"'
                ' -ex load'
                ' -ex kill'
                ' $SOURCE'
                )
    else:
        cmd = env.Command('/tmp/dummy', source,
            'echo Set BMP variable to device path of blackmagic probe && exit 1')
    env.AlwaysBuild(cmd)
    env.Precious(cmd)
    return cmd



def generate(env):
    env['CC'] = env.WhereIs('arm-none-eabi-gcc', os.environ['PATH'])
    env['AS'] = env.WhereIs('arm-none-eabi-as', os.environ['PATH'])
    env['OBJCOPY'] = env.WhereIs('arm-none-eabi-objcopy', os.environ['PATH'])
    env['GDB'] = env.WhereIs('arm-none-eabi-gdb', os.environ['PATH'])

    env.AddMethod(SetARMFlags)
    env.AddMethod(EmbeddedProgram)
    env.AddMethod(CopyObject)
    env.AddMethod(GDBInstall)
    env['BUILDERS']['Objcopy'] = Builder(action='$OBJCOPY $COPYFLAGS $SOURCE $TARGET')


def exists(env):
    return 1


def _node(env, path):
    if is_String(path):
        return env.File(path)
    else:
        return path
