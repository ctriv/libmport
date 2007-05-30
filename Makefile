# $FreeBSD: src/usr.sbin/pkg_install/lib/Makefile,v 1.18 2004/10/24 15:33:07 ru Exp $

LIB=		mport
SRCS=		plist.c	create_pkg.c db_schema.c util.c error.c

WARNS?=	3
WFORMAT?=	1
SHLIB_MAJOR=	1

DPADD=	${LIBSQLITE3} ${LIBPTHREAD} ${LIBMD}
LDADD=	-lsqlite3 -lpthread -lmd

.include <bsd.lib.mk>
