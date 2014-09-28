#
# Copyright (c) Michael Tharp <gxti@partiallystapled.com>
#
# This file is distributed under the terms of the MIT License.
# See the LICENSE file at the top of this tree, or if it is missing a copy can
# be found at http://opensource.org/licenses/MIT
#

import os
import sys
from SCons.Builder import Builder


def version_h(source, target, env):
    with open(str(target[0]), 'w') as f:
        f.write(env['VERSION_H_CONTENTS'])


def VersionH(env, target, **extra_vars):
    lines = ['#define VERSION "%s"\n' % env['VERSION']]
    for name, value in sorted(extra_vars.items()):
        resolved = env.subst(value)
        if not resolved:
            sys.exit('Failed to expand macro %s (value %r)' % (name, value))
        lines.append('#define %s %s\n' % (name, resolved))
    return env.AlwaysBuild(env.WriteVersionH(target, [],
        VERSION_H_CONTENTS=''.join(lines)))


def generate(env):
    p = os.popen(str(env.File('#util/git_version.sh')))
    env['VERSION'] = p.read().strip()
    if p.close():
        sys.exit('Failed to detect git version')
    env.AddMethod(VersionH)
    env['BUILDERS']['WriteVersionH'] = Builder(action=version_h)


def exists(env):
    return 1
