import glob
import os
import re

linkflags = []
cppflags = []

if ARGUMENTS.get('debug', 0):
    cppflags = ['-g', '-Wall']
else:
    cppflags = ['-O2']

env = Environment(LINKFLAGS = linkflags,
                  tools = ['default','textfile'])

env['NAME'] = 'gxgraph'
env['NAME_CAP'] = 'GxGraph'
env['VERSION'] = "0.1.6"

def is_centos():
    return re.search('CentOS', open("/etc/redhat-release").read())

# This probably exists in scons already
def file2c(env, target, source):
    out = open(str(target[0]), "wb")
    inp = open(str(source[0]), "rb")

    for line in inp.readlines():
        line = line.rstrip()
        line = re.sub("\\\\", "\\\\", line)
        line = re.sub("\\\"", "\\\"", line)
        line = '"'+line+'\\n"\n'
        out.write(line)
        
    out.close()
    inp.close()

env.Substfile("version.h",
              "version.h.in",
              SUBST_DICT = {'@VERSION@':env['VERSION']})
    
if ARGUMENTS.get('mingw', 0):
    env['CC']='i686-w64-mingw32-gcc'
    env['CXX']='i686-w64-mingw32-g++'
    env['AR']='i686-w64-mingw32-ar'
    env['RANLIB']='i686-w64-mingw32-ranlib'
#    env['PKGCONFIG'] = "env PKG_CONFIG_PATH=/usr/local/mingw32/lib/pkgconfig:/usr/i686-w64-mingw32/sys-root/mingw/lib/pkgconfig pkg-config"
    env['PKGCONFIG'] = "env PKG_CONFIG_PATH=/usr/local/mingw32/lib/pkgconfig:/usr/i686-w64-mingw32/sys-root/mingw/lib/pkgconfig pkg-config"
    env['OBJSUFFIX']=".obj"
    env['PROGSUFFIX'] = ".exe"
    env['SHOBJSUFFIX']=".obj"
    env['SHLIBSUFFIX'] = ".dll"
    env['SHLIBPREFIX'] = ""
    env['CROOT'] = "z:\\home\\dov\\.wine\\drive_c"
    env['PREFIX'] = "z:\\usr\\local\\mingw32"
    env['AGG'] = "aggx"
    env['ROOT'] = ""

    cppflags += ['-Wno-deprecated-declarations']
    subst_dict = dict([ ('@%s@'%k, env[k]) for k in ['ROOT',
                                                     'NAME',
                                                     'NAME_CAP',
                                                     'VERSION'] ])
    env.Substfile("${NAME}.nsi.in",
                  SUBST_DICT = subst_dict)
    env.Substfile("${NAME}.rc.in",
                  SUBST_DICT = subst_dict
                  )
    env.Command("COPYING.dos",
                "COPYING",
                ["unix2dos < COPYING > COPYING.dos"])
    
    env.Command("Install${NAME_CAP}-" + env['VERSION'] + ".exe",
                ["${NAME}.exe",
                 "${NAME}.nsi",
                 "SConstruct"
                 ],
                ["makensis ${NAME}.nsi"]
                )
    res = env.Command("${NAME}.res.o",
                      ["${NAME}.rc",
                      ],
                      ["i686-w64-mingw32-windres ${NAME}.rc ${NAME}.res.o"])
    env['PLOT'] = "gtkextra-3.0"
    env.Append(LINKFLAGS=["-mwindows"
                          ],
               CPPPATH=["/usr/local/mingw32/include/opencv"])
else:
    pkgconfig = "env PKG_CONFIG_PATH=/usr/local/lib/pkgconfig pkg-config"
    if is_centos():
        pkgconfig = "env PKG_CONFIG_PATH=/usr/local/gtk/lib/pkgconfig:/usr/local/lib/pkgconfig:/usr/lib/pkgconfig /usr/local/gtk/bin/pkg-config"

    env.Append(PKGCONFIG = pkgconfig,
               CPPPATH = ["/usr/local/include"],
               LIBS = ['m']
               )

env.ParseConfig('${PKGCONFIG} --cflags --libs gtk+-2.0')

src = ['gxgraph.c',
       'gtk_painter.c',
       'ps_painter.c',
       'svg_painter.c',
       'moving_ants.c',
       'gxgraph_hardcopy.c',
       'gxgraph_about.c',
       'parser.c' ]

if env['PLATFORM'] == "cygwin":
    linkflags += ['-mms-bitfields',
                  '-mno-cygwin',
                  '-mwindows'
                  ]
    cppflags += ['-mms-bitfields',
                 '-mno-cygwin']
#    env.Append(CPPPATH=['/usr/local/include'])
    src += ['${NAME}-rc.o']
    env.Command("${NAME}-rc.o",
                "${NAME}.rc",
                "windres -o ${NAME}-rc.o ${NAME}.rc")
elif ARGUMENTS.get('mingw',0):
    src += res
    
env.Append(LINKFLAGS=linkflags,
           CPPFLAGS=cppflags)

bin = env.Program('${NAME}',
                  src,
                  env['LIBS'])

env.Alias("install",
          [env.Install('/usr/local/bin',
                       bin),
           ])

env.Alias("dist",
          env.Command("dist",
                      glob.glob("*.c")
                      +glob.glob("*.h")
                      +glob.glob("*.i")
                      + ["COPYING",
                         "README.md",
                         "INSTALL",
                         "SConstruct",
                         "examples"],
                      ["tar -zcf ${NAME}-${VERSION}.tar.gz $SOURCES"]))

