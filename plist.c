/*-
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
 * $MidnightBSD$
 */



#include <sys/cdefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mport.h"

__MBSDID("$MidnightBSD: src/usr.sbin/pkg_install/lib/plist.c,v 1.50.2.1 2006/01/10 22:15:06 krion Exp $");


/* Do everything needed to set up a new plist.  Always use this to create a plist,
 * don't go off and do it yourself.
 */
Plist* new_plist() 
{
  Plist *list = (Plist*)malloc(sizeof(Plist));
  STAILQ_INIT(list);
  return list;
}


/* free all the entryes in the list, and then the list itself. */
void free_plist(Plist *list) 
{
  PlistEntry *n;

  while (!STAILQ_EMPTY(list)) {
     n = STAILQ_FIRST(list);
     STAILQ_REMOVE_HEAD(list, next);
     free(n->data);
     free(n);
  }
  
  free(list);
}


/* Parsers the contenst of the file and returns a plist data structure.
 *
 * Returns NULL on failure.
 */
Plist* parse_plist_file(FILE *fp)
{
  size_t length;
  char *line;
  
  while ((line = fgetln(fp, &length)) != NULL) {
    if (feof(fp)) {
      /* File didn't end in \n, get an exta byte so that the next bit doesn't
         wack the last char in the string. */
      length++;
      if ((line = realloc(line, length)) == NULL) {
        return NULL;
      }
    }
    
    
    /* change the last \n to \0 */
    *(line + length - 1) = 0;
    
    
    if (*line == '@') {
      line++;
      char *cmnd = strsep(&line, " \t");
      
      if (cmnd == NULL) {
        // warn: malformed plist
        return NULL;
      }
      
      char *data = (char *)malloc(strlen(line) + 1);
      
      if (cmnd == NULL) {
        // warn: out of mem!
        return NULL;
      }
      
      strlcpy(data, line, (strlen(line) + 1));
      
      printf("%s: %s\n", cmnd, line);
    } else {
      printf("file: %s\n", line);
    }
  }
  
  return new_plist();
}
    
    
  

