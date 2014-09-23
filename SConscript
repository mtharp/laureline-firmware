#
# Copyright (c) Michael Tharp <gxti@partiallystapled.com>
#
# This file is distributed under the terms of the MIT License.
# See the LICENSE file at the top of this tree, or if it is missing a copy can
# be found at http://opensource.org/licenses/MIT
#


Import('env')
env = env.Clone()

# Include path
env.PrependUnique(CPPPATH="""
src/conf
src
ports
lib
lib/lwip
lwip/src/include
lwip/src/include/ipv4
lwip/src/include/ipv6
FreeRTOS/Source/include
FreeRTOS/Source/portable/GCC/ARM_CM3
""".split())

# Regular sources
srcs = env.Globs("""
src/*.c
src/gps/*.c
src/net/*.c
lib/cmdline/core.c
lib/cmdline/settings.c
lib/freertos_plat.c
lib/info_table.c
lib/lwip/arch/sys_arch.c
lib/stm32/eth_mac.c
lib/stm32/i2c.c
lib/stm32/iwdg.c
lib/stm32/serial.c
lib/uptime.c
lib/util/parse.c
ports/core_cm3.c
ports/crt0.c
ports/stubs.c
ports/vectors.c
""")

# Build third-party code with relaxed warnings
libs = env.Object(env.Globs("""
lib/crypto/md5_dgst.c
lib/crypto/sha1dgst.c
lib/crypto/sha1_thumb.s
lwip/src/api/*.c
lwip/src/core/api/*.c
lwip/src/core/*.c
lwip/src/core/ipv4/*.c
lwip/src/core/ipv6/*.c
lwip/src/core/snmp/*.c
lwip/src/netif/etharp.c
FreeRTOS/Source/event_groups.c
FreeRTOS/Source/list.c
FreeRTOS/Source/portable/GCC/ARM_CM3/port.c
FreeRTOS/Source/portable/MemMang/heap_3.c
FreeRTOS/Source/queue.c
FreeRTOS/Source/tasks.c
FreeRTOS/Source/timers.c
"""), CFLAGS_USER='$CFLAGS_USER -Wno-unused-value -Wno-error=aggressive-loop-optimizations')

default = env.EmbeddedProgram('laureline.elf', srcs + libs,
    script='ports/STM32F107xB.ld',
    LIBS=['m', 'nosys'],
    )

# Generated version.h with git revision and bootloader settings
def version_h(source, target, env):
    import os
    p = os.popen(str(File('#util/git_version.sh')))
    version = p.read().strip()
    if p.close():
        print 'Failed to detect git version'
        Exit(1)
    with open(str(target[0]), 'w') as f:
        print >> f, '#define VERSION "%s"' % version
        print >> f, '#define HW_VERSION %s' % env['HW_VERSION']
        print >> f, '#define HSE_FREQ %s' % env['HSE_FREQ']
env['BUILDERS']['VersionH'] = Builder(action=version_h)
AlwaysBuild(env.VersionH('src/version.h', []))

Alias('install', env.GDBInstall(default))
Return('default')
