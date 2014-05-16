
.include "src.opts.mk"

.if defined(.PARSEDIR)
.if ${.MAKE.MODE:Mmeta*} != ""
.if !empty(SUBDIR) && !defined(LIB) && !defined(PROG) && ${.MAKE.MAKEFILES:M*bsd.prog.mk} == ""
.if ${.MAKE.MODE:Mleaf*} != ""
# we only want leaf dirs to build in meta mode... and we are not one
.MAKE.MODE = normal
.endif
.endif
.endif
.endif

.if ${MK_SYSROOT} == "yes" && !empty(SYSROOT)
CFLAGS_LAST+= --sysroot=${SYSROOT}
CXXFLAGS_LAST+= --sysroot=${SYSROOT}
LDADD+= --sysroot=${SYSROOT}
.if ${MACHINE} == "host"
# we cheat?
LDADD+= -B/usr/lib
CFLAGS_LAST+= -I/usr/include
CXXFLAGS_LAST+= -I/usr/include
.endif
.endif

.if ${MACHINE} == "host"
.if ${.MAKE.DEPENDFILE:E} != "host"
UPDATE_DEPENDFILE?= no
.endif
HOST_CC?= /usr/bin/cc
HOST_CFLAGS+= -DHOSTPROG
CC= ${HOST_CC}
CFLAGS+= ${HOST_CFLAGS}
.endif
