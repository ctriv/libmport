/* $MidnightBSD$
 *
 * Copyright (c) 2007 Chris Reinhardt
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef _MPORT_H_
#define _MPORT_H_


#include <sys/cdefs.h>
__MBSDID("$MidnightBSD: src/usr.sbin/pkg_install/lib/plist.c,v 1.50.2.1 2006/01/10 22:15:06 krion Exp $");


/* Headerish stuff */

#include <sys/queue.h>

/* Incomplete */
enum _PlistEntryType {
  PLIST_FILE, PLIST_EXEC, PLIST_UNEXEC
};

typedef enum _PlistEntryType PlistEntryType;

struct _PlistEntry {
  PlistEntryType type;
  char *data;
  STAILQ_ENTRY(_PlistEntry) next;
};

STAILQ_HEAD(_Plist, _PlistEntry);

typedef struct _Plist Plist;
typedef struct _PlistEntry PlistEntry;

Plist* new_plist(void);
void free_plist(Plist *);
Plist* parse_plist_file(FILE *);

#endif
