# $MidnightBSD: src/lib/libmport/Makefile,v 1.5 2007/12/01 06:21:37 ctriv Exp $

LIB=		mport
SRCS=		archive.c plist.c create_primative.c db.c util.c error.c \
		install_primative.c instance.c version_cmp.c \
		check_preconditions.c

INCS=		mport.h


CFLAGS+=	-I${.CURDIR}
WARNS?=	3
WFORMAT?=	1
SHLIB_MAJOR=	1

DPADD=	${LIBSQLITE3} ${LIBMD}
LDADD=	-lsqlite3 -lmd

.include <bsd.lib.mk>
