/*-
 * Copyright (c) 2009 Chris Reinhardt
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
 * $MidnightBSD: src/lib/libmport/error.c,v 1.7 2008/04/26 17:59:26 ctriv Exp $
 */


#include "mport.h"
#include "mport_private.h"

static mport_is_recentish(mportInstance *);

MPORT_PUBLIC_API int mport_index_load(mportInstance *mport)
{
  if (mport_file_exists(MPORT_INDEX_FILE)) {
    if (mport_db_do(mport->db, "ATTACH %Q AS index", MPORT_INDEX_FILE) != MPORT_OK)
        RETURN_CURRENT_ERROR;
        
    mport->flags |= MPORT_INST_HAVE_INDEX;
  
    if (!index_is_recentish(mport)) {
      if (mport_fetch_index(mport) != MPORT_OK)
        RETURN_CURRENT_ERROR;
        
      if (mport_db_do(mport->db, "DETACH index") != MPORT_OK)
        RETURN_CURRENT_ERROR;
        
      mport->flags &= ~MPORT_INST_HAVE_INDEX;
        
      if (mport_db_do(mport->db, "ATTACH %Q AS index", MPORT_INDEX_FILE) != MPORT_OK)
        RETURN_CURRENT_ERROR;
        
      mport->flags |= MPORT_INST_HAVE_INDEX;
    }
  } else {
    if (mport_fetch_bootstrap_index(mport)) != MPORT_OK)
      RETURN_CURRENT_ERROR;
    
    if (mport_db_do(mport->db, "ATTACH %Q AS index", MPORT_INDEX_FILE) != MPORT_OK)
      RETURN_CURRENT_ERROR;
      
    mport->flags |= MPORT_INST_HAVE_INDEX;
  }
  

  return MPORT_OK;
}


static int index_is_recentish(mportInstance *mport) 
{
    
}  

int mport_index_get_mirror_list(mportInstance *mport, char ***list_p)
{
    
  
}


int mport_index_lookup_pkgname(mportInstance *mport, const char *pkgname, mportIndexEntry **e)
{
}
