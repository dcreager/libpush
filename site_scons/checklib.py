# -*- coding: utf-8 -*-

"""
Provides a function similar to CheckLib, but allows you to specify the
directory where we expect to find the library.  Also defines a
command-line option that lets the user override this directory.
"""

from SCons.Script import *

def CheckLibInPath(context, libname, library, call, header=""):
    vars = Variables('.scons.vars.%s' % libname, ARGUMENTS)

    vars.AddVariables(
        PathVariable("with_%s" % libname,
                     "Location of %s library" % libname,
                     "/usr"),
        )

    vars.Update(context.env)
    vars.Save('.scons.vars.%s' % libname, context.env)

    context.Message("Checking for %s..." % libname)

    try:
        lastLIBS = context.env['LIBS']
    except KeyError:
        lastLIBS = []

    try:
        lastLIBPATH = context.env['LIBPATH']
    except KeyError:
        lastLIBPATH = []

    try:
        lastCPPPATH = context.env['CPPPATH']
    except KeyError:
        lastCPPPATH = []

    libpath = "${with_%s}/lib" % libname
    cpppath = "${with_%s}/include" % libname

    context.env.Append(LIBS=[library],
                       LIBPATH=[libpath],
                       CPPPATH=[cpppath])

    ret = context.TryLink("""
%s

int
main(int argc, char **argv) {
    %s;
    return 0;
}
""" % (header, call), ".c")

    context.env.Replace(LIBS=lastLIBS,
                        LIBPATH=lastLIBPATH,
                        CPPPATH=lastCPPPATH)

    if ret:
        context.env['%s_LIB' % libname] = library
        context.env['%s_LIBPATH' % libname] = libpath
        context.env['%s_CPPPATH' % libname] = cpppath

    context.Result(ret)
    return ret
