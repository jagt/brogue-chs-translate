#!/usr/bin/env python
#
#  package-dylibs.py
#
#  Created by Matt Kimball.
#  Copyright 2013. All rights reserved.
#  
#  This file is part of Brogue.
#
#  Brogue is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  Brogue is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with Brogue.  If not, see <http://www.gnu.org/licenses/>.
#

import argparse
import os
import re

"""
When building for OS X, we need to copy all of the GNOME libraries
which we are dependent upon into the application bundle, but we also
need to change their 'install name' from the absolute paths they are
built with to paths which are relative to the executable, so that we
can then redistribute the executable.

package-dylibs.py will both copy in the necessary libraries, by
following the chain of dylib dependencies, and change the linkage such
that they are then relative to the main executable.
"""

class SubcommandError (Exception):
    "Raised when one of our subcommands has an unexpected return code."

    pass

def get_dependent_libs(executable):
    """
    Given an executable or library, return a list of dylibs which it
    links against.
    """

    otool_out = os.popen('otool -L %s' % executable).read()
    
    return re.findall('\s+(.*) \(compatibility ', otool_out)

def path_match(libpath, lib):
    "Return true if the library is in the patch we are matching against."

    return os.path.dirname(lib) == libpath

def lib_already_present(executable, lib):
    "Return true if the library is already present with the executable."

    return os.path.exists(
        os.path.dirname(executable) + '/' + os.path.basename(lib))

def populate_lib(executable, lib):
    "Copy the library such that it is in the same directory as the executable."

    libpath = os.path.dirname(executable) + '/' + os.path.basename(lib)
    cmd = 'cp %s %s' % (lib, libpath)
    print cmd

    ret = os.system(cmd)
    if ret:
        raise SubcommandError(ret)

    return libpath

def replace_install_name(executable, lib):
    """
    Given an executable (or dylib), change the library path 'lib' in
    the executable to assume the library is in the same directory as
    the executable.
    """

    cmd = 'install_name_tool -change %s %s %s' % \
        (lib, '@executable_path/' + os.path.basename(lib), executable)
    print cmd

    ret = os.system(cmd)
    if ret:
        raise SubcommandError(ret)

def package(executable, libpath):
    """
    Given an executable (or dylib), recursively find all dependent
    libraries which match the libpath and copy them such that they are
    present with the executable and change the linkage such that it
    expects them to be there.
    """

    for lib in get_dependent_libs(executable):
        if not path_match(libpath, lib):
            continue

        if not lib_already_present(executable, lib):
            newlib = populate_lib(executable, lib)
            package(newlib, libpath)

        replace_install_name(executable, lib)

def main():
    "Parse the commandline arguments and then do the library relocation."

    parser = argparse.ArgumentParser(description='Package depended dylibs')
    parser.add_argument('--libpath', required=True, help='Source library path')
    parser.add_argument('executable', help='Target executable')

    args = parser.parse_args()

    package(args.executable, args.libpath)

if __name__ == '__main__':
    main()
