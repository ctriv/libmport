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



#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "mport.h"

__MBSDID("$MidnightBSD: src/lib/libmport/inst_init.c,v 1.3 2007/12/05 17:02:15 ctriv Exp $");


/* set up the master database, and related instance infrastructure. */
mportInstance * mport_new_instance() 
{
 return (mportInstance *)malloc(sizeof(mportInstance)); 
}
 
int mport_init_instance(mportInstance *mport, const char *root)
{
  char dir[FILENAME_MAX];
  
  if (root != NULL) {
    mport->root = strdup(root);
    (void)snprintf(dir, FILENAME_MAX, "%s/%s", root, MPORT_INST_DIR);
  } else {
    mport->root = NULL;
    (void)strlcpy(dir, MPORT_INST_DIR, FILENAME_MAX);
  }
  
  if (mport_mkdir(dir) != MPORT_OK)
    RETURN_CURRENT_ERROR;
  
  if (root != NULL) {
    (void)snprintf(dir, FILENAME_MAX, "%s/%s", root, MPORT_INST_INFRA_DIR);
  } else {
    (void)strlcpy(dir, MPORT_INST_INFRA_DIR, FILENAME_MAX);
  }
  
  if (mport_mkdir(dir) != MPORT_OK)
    RETURN_CURRENT_ERROR;
  
  if (mport_db_open_master(&(mport->db)) != MPORT_OK)
    RETURN_CURRENT_ERROR;  

  /* create tables */
  return mport_generate_master_schema(mport->db);
}
