# -*- python -*-
# @file SConscript
# @brief build info
#
# $Id: SConscript,v 1.93 2016/03/05 19:35:24 mdwood Exp $
# Authors: T. Burnett <tburnett@u.washington.edu>
# Version: astro-04-00-02
Import('baseEnv')
Import('listFiles')
libEnv = baseEnv.Clone()
progEnv = baseEnv.Clone()

if baseEnv['PLATFORM'] == "win32":
    libEnv.Append(CCFLAGS = "/wd4554") #  warning C4554: '<<' : check operator precedence

if 'makeStatic' in baseEnv:
    libEnv.Tool('addLinkDeps', package="astro", toBuild="static")
    # EAC, switch to using wcslib as an external
    astroLib = libEnv.StaticLibrary('astro', listFiles(
        ['src/*.cxx', 
         'src/jplephem/*.cxx',
         'src/igrf_sub/*.cxx']))

else:
    libEnv.Tool('addLinkDeps', package="astro", toBuild="shared")
    # EAC, switch to using wcslib as an external
    astroLib = libEnv.SharedLibrary('astro', listFiles(
        ['src/*.cxx', 
         'src/jplephem/*.cxx',
         'src/igrf_sub/*.cxx']))

progEnv.Tool('astroLib')

test_astro = progEnv.Program('test_astro', listFiles(['src/test/*.cxx']))

if 'makeStatic' in baseEnv:
    progEnv.Tool('registerTargets', 
                 package = 'astro', 
                 staticLibraryCxts = [[astroLib, libEnv]], 
                 testAppCxts = [[test_astro, progEnv]],
                 data = listFiles(['data/*']),
                 includes = listFiles(['astro/*.h']))
else:
    progEnv.Tool('registerTargets', 
                 package = 'astro', 
                 libraryCxts = [[astroLib, libEnv]], 
                 testAppCxts = [[test_astro, progEnv]],
                 data = listFiles(['data/*']),
                 includes = listFiles(['astro/*.h']))




