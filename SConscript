#
# Copyright (c) Michael Tharp <gxti@partiallystapled.com>
#
# This file is distributed under the terms of the MIT License.
# See the LICENSE file at the top of this tree, or if it is missing a copy can
# be found at http://opensource.org/licenses/MIT
#


Import('env')
env = env.Clone()
env.PrependUnique(CPPPATH=[
    'src/conf',
    'src',
    'ports',
    'lib',
    'FreeRTOS/Source/include',
    'FreeRTOS/Source/portable/GCC/ARM_CM3',
    ])

default = env.EmbeddedProgram('laureline.elf', [
    'src/board.c',
    'src/init.c',
    'src/main.c',
    'ports/core_cm3.c',
    'ports/crt0.c',
    'ports/stubs.c',
    'ports/vectors.c',
    'lib/info_table.c',
    'lib/freertos_plat.c',
    'FreeRTOS/Source/event_groups.c',
    'FreeRTOS/Source/list.c',
    'FreeRTOS/Source/queue.c',
    'FreeRTOS/Source/tasks.c',
    'FreeRTOS/Source/timers.c',
    'FreeRTOS/Source/portable/GCC/ARM_CM3/port.c',
    'FreeRTOS/Source/portable/MemMang/heap_3.c',
    ],
    script='ports/STM32F107xB.ld',
    LIBS=['m', 'nosys'],
    )

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
