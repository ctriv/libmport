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
 * $MidnightBSD: src/lib/libmport/inst_init.c,v 1.3 2007/12/05 17:02:15 ctriv Exp $
 */


#include <stdio.h>
#include "mport.h"

__MBSDID("$MidnightBSD: src/lib/libmport/inst_init.c,v 1.3 2007/12/05 17:02:15 ctriv Exp $");


void mport_default_msg_cb(const char *msg) 
{
  (void)puts(msg);
}

int mport_default_confirm_cb(const char *msg, const char *yes, const char *no, int def)
{
  size_t len;
  char *ans;
  
  (void)fprintf(stderr, "%s (Y/N) [%s]: ", msg, def == 1 ? yes : no);
  
  while (1) {
    /* get answer, if just \n, then default. */
    ans = fgetln(stdin, &len);
  
    if (len == 1) { 
      /* user just hit return */
      (void)fprintf(stderr, "%s\n", def == 1 ? yes : no);
      return def;
    }
    
    (void)fprintf(stderr, "\n");
    
    if (*ans == 'Y' || *ans == 'y') 
      return MPORT_OK;
    if (*ans == 'N' || *ans == 'n')
      return -1;
    
    (void)fprintf(stderr, "Please enter yes or no.\n");   
  }
  
  /* Not reached */
  return MPORT_OK;
}

