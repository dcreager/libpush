# -*- coding: utf-8 -*-

from checklib import *

vars = Variables('.scons.vars', ARGUMENTS)

vars.AddVariables(
    PathVariable("prefix", "Installation prefix", "/usr",
                 PathVariable.PathAccept),
    ('AR', "The static archiver to use"),
    ('RANLIB', "The archive indexer to use"),
    ('CC', "The C compiler to use"),
    ('LINK', "The linker to use"),
    ('SHLINK', "The shared library linker to use"),
    ('CCFLAGS', "Any additional options to pass in to the C compiler"),
    ('CPPFLAGS', "Any additional options to pass in to the C preprocessor"),
    ('LINKFLAGS', "Any additional options to pass in to the linker"),
    ('SHLINKFLAGS', "Any additional options to pass in to the shared library linker"),
    )


root_env = Environment(tools=['default', 'packaging'],
                       package="libpush",
                       pkg_version="1.0-dev",
                       CCFLAGS="-O2",
                       BINDIR = "$prefix/bin",
                       DOCDIR = "$prefix/share/doc/$package",
                       LIBDIR = "$prefix/lib",
                       INCLUDEDIR = "$prefix/include")

vars.Update(root_env)
vars.Save(".scons.vars", root_env)

root_env.MergeFlags('-g -Wall -Werror')

# An action that can clean up the scons temporary files.

if 'sdist' not in COMMAND_LINE_TARGETS:
    root_env.Clean("distclean",
                   [
                    ".sconf_temp",
                    ".sconsign.dblite",
                    ".scons.vars",
                    ".scons.vars.check",
                    "config.log",
                   ])

# Only run the configuration steps if we're actually going to build
# something.

if not GetOption('clean') and not GetOption('help'):
    conf = root_env.Configure(custom_tests=
                              {
                               'CheckLibInPath': CheckLibInPath,
                              })

    if not conf.CheckCC():
        print "!! Your compiler and/or environment is not properly configured."
        Exit(0)


    if not conf.CheckLibInPath("check",
                               library="check",
                               call="",
                               header="#include <check.h>"):
        print "!! Cannot find the check library."
        Exit(0)


    if not conf.CheckLibInPath("libhwm",
                               library="hwm",
                               call="hwm_buffer_new()",
                               header="#include <hwm-buffer.h>"):
        print "!! Cannot find the libhwm library."
        Exit(0)


    root_env = conf.Finish()

# Set up a list of source files for the packaging target later on.
# Each SConscript file is responsible for updating this list.

SOURCE_FILES = []
Export('SOURCE_FILES')

# Add the root scons files to the SOURCES_LIST.

build_files = map(File, \
    [
     "SConstruct",
     "site_scons/checklib.py",
    ])

SOURCE_FILES.extend(build_files)

# Include the subdirectory SConscripts

Export('root_env')
SConscript([
            'ccan/SConscript',
            'doc/SConscript',
            'include/SConscript',
            'src/SConscript',
            'tests/SConscript',
           ])

# Install documentation files

doc_files = map(File, \
    [
     "LICENSE.txt",
    ])

SOURCE_FILES.extend(doc_files)

root_env.Alias("install", root_env.Install("$DOCDIR", doc_files))

# Define packaging targets

license = 'BSD'
summary = 'A push parsing framework'
source_url = 'http://github.com/dcreager/libpush.git'

src = root_env.Package(NAME="$package",
                       VERSION="${pkg_version}",
                       PACKAGETYPE='src_targz',
                       source=SOURCE_FILES)
root_env.Alias("sdist", src)

if GetOption('clean'):
    root_env.Default("sdist")
    root_env.Clean("sdist", "$package-${pkg_version}")
