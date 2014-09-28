#
# Copyright (c) Michael Tharp <gxti@partiallystapled.com>
#
# This file is distributed under the terms of the MIT License.
# See the LICENSE file at the top of this tree, or if it is missing a copy can
# be found at http://opensource.org/licenses/MIT
#

vars = Variables(None, ARGUMENTS)
vars.Add(BoolVariable('DEBUG', 'Disable optimizations', 0))
vars.Add(BoolVariable('WERROR', 'Make warnings into errors', 0))
vars.Add('HSE_FREQ', "Default oscillator frequency for the application's boot stub", '25000000')
vars.Add('HW_VERSION', "Hardware version for the application's boot stub", '0x0700')
vars.Add('BMP', 'Path to blackmagic probe device for installation', '')

env = Environment(variables=vars, tools=['default', 'embedded_program', 'version_h'])
Help(vars.GenerateHelpText(env))

env.SetARMFlags('cortex-m3')
env['CFLAGS_USER'] = '-std=gnu99'
if env.get('WERROR'):
    env['CFLAGS_USER'] += ' -Werror'

Export('env')
all = []

# scons default - main application
default = SConscript('SConscript', variant_dir='build')
all += default
main_elf = [x for x in default if x.name.endswith('.elf')]
main_hex = [x for x in default if x.name.endswith('.hex')]
Alias('default', default)
Default(default)

# scons bootloader - bootloaders for all targets
VariantDir('build/bootloader', '.')
loader = SConscript('build/bootloader/bootloader/SConscript')
all += loader
Alias('bootloader', loader)

# scons dist
dist = []
dist += env.Command('dist/laureline-${VERSION}.elf', main_elf, Copy('$TARGET', '$SOURCE'))
dist += env.Command('dist/laureline-${VERSION}.hex', main_hex, Copy('$TARGET', '$SOURCE'))
for bl in loader:
    name = bl.name.replace('bootloader-', 'bootloader-${VERSION}-')
    dist += env.Command('dist/' + name, bl, Copy('$TARGET', '$SOURCE'))
NoClean(dist)
Alias('dist', dist)

Clean(all, 'build')
Alias('all', all)
