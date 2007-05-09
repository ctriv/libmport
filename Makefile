# $FreeBSD: src/usr.sbin/pkg_install/lib/Makefile,v 1.18 2004/10/24 15:33:07 ru Exp $

LIB=		mport
SRCS=		plist.c	

WARNS?=	3
WFORMAT?=	1
SHLIB_MAJOR=	1
SHLIB_MINOR=	0

.include <bsd.lib.mk>
