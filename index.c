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

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

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
  struct stat st;
  struct timespec now;
  
  
  if (stat(MPORT_INDEX_FILE, &st) != 0) 
    return 0;
   
  if (clock_gettime(CLOCK_REALTIME, &now) != 0) 
    RETURN_ERROR(MPORT_ERR_SYSCALL_FAILED, strerror(errno));
      
  if ((stat.st_birthtime + MPORT_INDEX_MAX_AGE) < now.tv_sec) 
    return 0;
    
  return 1;
}  

int mport_index_get_mirror_list(mportInstance *mport, char ***list_p)
{
  char **list;
  int len, ret, i;
  sqlite3_stmt *stmt;
  
  /* XXX the country is hard coded until a configuration system is created */    
  if (mport_db_prepare(mport->db, "SELECT COUNT(*) FROM index.mirrors WHERE country='us'") != MPORT_OK)
    RETURN_CURRENT_ERROR;

  switch (sqlite3_step(stmt)) {
    case SQLITE_ROW:
      len = sqlite3_column_int(stmt, 0);
      sqlite3_finalize(stmt);
      break;
    case SQLITE_DONE:
      SET_ERROR(MPORT_ERR_INTERNAL, "A 'SELECT COUNT(*)...' statement returned no rows.");
      sqlite3_finalize(stmt);
      RETURN_CURRENT_ERROR;
      break;
    default:
      SET_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(mport->db));
      sqlite3_finalize(stmt);
      RETURN_CURRENT_ERROR;
  }
  
  list = calloc(len + 1, sizeof(char *));
  *list_p = list;  
  i = 0;
    
  if (mport_db_prepare(mport->db, "SELECT mirror FROM index.mirrors WHERE country='us'") != MPORT_OK)
    RETURN_CURRENT_ERROR;
    
  while (1) {
    ret = sqlite3_step(stmt);
    
    if (ret == SQLITE_ROW) {
      list[i] = strdup(sqlite3_column_text(stmt, 0));
      
      if (list[i] == NULL) {
        sqlite3_finalize(stmt);
        return MPORT_ERR_NO_MEM;
      }
      
      i++;
    } else if (ret == SQLITE_DONE) {
      list[i] = NULL;
      break;
    } else {
      sqlite3_finalize(stmt);
      RETURN_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(mport->db));
    }
  }
  
  sqlite3_finalize(stmt);
  return MPORT_OK;
}

        
MPORT_PUBLIC_API int mport_index_lookup_pkgname(mportInstance *mport, const char *pkgname, mportIndexEntry **e)
{

}
