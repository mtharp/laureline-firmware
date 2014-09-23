#
# Copyright (c) Michael Tharp <gxti@partiallystapled.com>
#
# This file is distributed under the terms of the MIT License.
# See the LICENSE file at the top of this tree, or if it is missing a copy can
# be found at http://opensource.org/licenses/MIT
#

vars = Variables(None, ARGUMENTS)
vars.Add(BoolVariable('DEBUG', 'Disable optimizations', 0))
vars.Add('HSE_FREQ', '', '0')
vars.Add('HW_VERSION', '', '0')
vars.Add('BMP', 'Path to blackmagic probe device', '')

env = Environment(variables=vars, tools=['default', 'embedded_program'])
Help(vars.GenerateHelpText(env))

env.SetARMFlags('cortex-m3')
env['CFLAGS_USER'] = '-std=gnu99 -Werror'
Export('env')
result = SConscript('SConscript', variant_dir='build')
Clean(result, 'build')
