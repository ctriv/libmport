# $MidnightBSD: src/lib/libmport/Makefile,v 1.6 2008/01/05 22:18:20 ctriv Exp $

LIB=		mport
SRCS=		bundle.c plist.c create_primative.c db.c util.c error.c \
		install_primative.c instance.c version_cmp.c \
		check_preconditions.c delete_primative.c default_cbs.c 
#		merge_primative.c
		
INCS=		mport.h


CFLAGS+=	-I${.CURDIR}
WARNS?=	3
WFORMAT?=	1
SHLIB_MAJOR=	1

DPADD=	${LIBSQLITE3} ${LIBMD}
LDADD=	-lsqlite3 -lmd

.include <bsd.lib.mk>
