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
#include <stdlib.h>

static int fetch(mportInstance *, const char *, const char *);

int mport_fetch_index(mportInstance *mport)
{
  char **mirrors;
  char *url;
  char *dest;
  int i;
  
  if (mport_index_is_recentish(mport))
    return MPORT_OK;
  
  if (mport_index_get_mirror_list(mport, &mirrors) != MPORT_OK)
    RETURN_CURRENT_ERROR;
  
  /* XXX mirrors needs freeing */
  
  asprintf(&dest, "%s/%s.bz2", MPORT_FETCH_STAGING_DIR, MPORT_INDEX_FILE);
  
  if (dest == NULL)
    return MPORT_ERR_NO_MEM;
    
  if (mirrors == NULL) {
    if (fetch(mport, MPORT_BOOTSTRAP_INDEX_URL, MPORT_INDEX_FILE) != MPORT_OK) {
      RETURN_CURRENT_ERROR;
    }
    
    return MPORT_OK;
  } else {
    while (mirrors[i] != NULL) {
      asprintf(&url, "%s/%s", mirrors[i], MPORT_INDEX_URL_PATH);

      if (url == NULL) {
        free(dest);
        return MPORT_ERR_NO_MEM;
      }

      if (fetch(mport, url, MPORT_INDEX_FILE) == MPORT_OK) {
        free(url);
        free(dest);
        return MPORT_OK;
      } 
      
      free(url);
    }
  }
    
  free(dest);
  RETURN_ERRORX(MPORT_ERR_FETCH, "Unable to fetch index file: %s", mport_err_string());
}


int mport_fetch_pkg(mportInstance *mport, const char *filename)
{
  char **mirrors;
  char *url;
  char *dest;
  int i;
  
  if (mport_index_get_mirror_list(mport, &mirrors) != MPORT_OK)
    RETURN_CURRENT_ERROR;
    
  if (mirrors == NULL) 
    RETURN_ERROR(MPORT_ERR_INTERNAL, "Attempt to fetch a file without an index.");
    
  asprintf(&dest, "%s/%s", MPORT_FETCH_STAGING_DIR, filename);
  
  while (mirrors[i] != NULL) {
    asprintf(&url, "%s/%s/%s", mirrors[i], MPORT_URL_PATH, filename);

    if (fetch(mport, url, dest) == MPORT_OK) {
      free(url);
      free(dest);
      return MPORT_OK;
    } 
    
    free(url);
  }
  
  free(dest);
  
  RETURN_ERRORX(MPORT_ERR_FETCH, "Unable to fetch %s: %s", filename, mport_err_string());
}

