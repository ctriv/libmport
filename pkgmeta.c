/*-
 * Copyright (c) 2007-2009 Chris Reinhardt
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
 * $MidnightBSD: src/lib/libmport/util.c,v 1.10 2008/04/26 17:59:26 ctriv Exp $
 */


#include <stdlib.h>
#include <string.h>
#include "mport.h"
#include "mport_private.h"

static int populate_meta_from_stmt(mportPackageMeta *, sqlite3 *, sqlite3_stmt *);
static int populate_vec_from_stmt(mportPackageMeta ***, int, sqlite3 *, sqlite3_stmt *);


/* Package meta-data creation and destruction */
MPORT_PUBLIC_API mportPackageMeta* mport_pkgmeta_new() 
{
  /* we use calloc so any pointers that aren't set are NULL.
     (calloc zero's out the memory region. */
  return (mportPackageMeta *)calloc(1, sizeof(mportPackageMeta));
}

MPORT_PUBLIC_API void mport_pkgmeta_free(mportPackageMeta *pack)
{
  int i;
  
  free(pack->pkg_filename);
  free(pack->name);
  free(pack->version);
  free(pack->lang);
  free(pack->comment);
  free(pack->sourcedir);
  free(pack->desc);
  free(pack->prefix);
  free(pack->mtree);
  free(pack->origin);
  free(pack->conflicts);
  free(pack->pkginstall);
  free(pack->pkgdeinstall);
  free(pack->pkgmessage);
  
  i = 0;
  if (pack->conflicts != NULL)  {
    while (pack->conflicts[i] != NULL)
      free(pack->conflicts[i++]);
  }

  free(pack->conflicts);
  
  i = 0;
  if (pack->depends != NULL) {
    while (pack->depends[i] != NULL) {
      free(pack->depends[i++]);
    }
  }
  
  free(pack->depends);

  free(pack);
}

/* free a vector of mportPackageMeta pointers */
MPORT_PUBLIC_API void mport_pkgmeta_vec_free(mportPackageMeta **vec)
{
  int i;
  for (i=0; *(vec + i) != NULL; i++) {
    mport_pkgmeta_free(*(vec + i));
  }
  
  free(vec);
}


/* mport_pkgmeta_read_stub(mportInstance *mport, mportPackageMeta ***pack)
 *
 * Allocates and populates a vector of mportPackageMeta structs from the stub database
 * connected to db. These structs represent all the packages in the stub database.
 * This does not populate the conflicts and depends fields.
 */
int mport_pkgmeta_read_stub(mportInstance *mport, mportPackageMeta ***ref)
{
  sqlite3_stmt *stmt;
  sqlite3 *db = mport->db;
  int len, ret;
  
  if (mport_db_prepare(db, &stmt, "SELECT COUNT(*) FROM stub.packages") != MPORT_OK)
    RETURN_CURRENT_ERROR;

  if (sqlite3_step(stmt) != SQLITE_ROW) {
    sqlite3_finalize(stmt);
    RETURN_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(db));
  }
  
  len = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);

  if (len == 0) {
    /* a stub should have packages! */
    RETURN_ERROR(MPORT_ERR_INTERNAL, "stub database contains no packages.");
  }
    
  if (mport_db_prepare(db, &stmt, "SELECT pkg, version, origin, lang, prefix, comment FROM stub.packages") != MPORT_OK)
    RETURN_CURRENT_ERROR;
  
  ret = populate_vec_from_stmt(ref, len, db, stmt);
  
  sqlite3_finalize(stmt);
  
  return ret;
}


 
 


/* mport_pkgmeta_search_master(mportInstance *mport, mportPacakgeMeta ***pack, const char *where, ...)
 *
 * Allocate and populate the package meta for the given package from the
 * master database.
 * 
 * 'where' and the vargs are used to be build a where clause.  For example to search by
 * name:
 * 
 * mport_pkgmeta_search_master(mport, &packvec, "pkg=%Q", name);
 *
 * or by origin
 *
 * mport_pkgmeta_search_master(mport, &packvec, "origin=%Q", origin);
 *
 * pack is set to NULL and MPORT_OK is returned if no packages where found.
 */
MPORT_PUBLIC_API int mport_pkgmeta_search_master(mportInstance *mport, mportPackageMeta ***ref, const char *fmt, ...)
{
  va_list args;
  sqlite3_stmt *stmt;
  int ret, len;
  char *where;
  sqlite3 *db = mport->db;
  
  va_start(args, fmt);
  where = sqlite3_vmprintf(fmt, args);
  va_end(args);
    
  if (where == NULL) 
    RETURN_ERROR(MPORT_ERR_NO_MEM, "Could not build where clause");
  
  
  if (mport_db_prepare(db, &stmt, "SELECT count(*) FROM packages WHERE %s", where) != MPORT_OK)
    RETURN_CURRENT_ERROR;

  if (sqlite3_step(stmt) != SQLITE_ROW) {
    sqlite3_finalize(stmt);
    RETURN_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(db));
  }

    
  len = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);

  if (len == 0) {
    sqlite3_free(where);
    *ref = NULL;
    return MPORT_OK;
  }

  if (mport_db_prepare(db, &stmt, "SELECT pkg, version, origin, lang, prefix, comment FROM packages WHERE %s", where) != MPORT_OK)
    RETURN_CURRENT_ERROR;
    
    
  ret = populate_vec_from_stmt(ref, len, db, stmt);

  sqlite3_free(where);  
  sqlite3_finalize(stmt);
  
  return ret;
}






/* mport_pkgmeta_get_downdepends(mportInstance *mport, mportPackageMeta *pkg)
 * 
 * Populate the depends of a pkg using the data in the master database.  
 */
MPORT_PUBLIC_API int mport_pkgmeta_get_downdepends(mportInstance *mport, mportPackageMeta *pkg)
{
  int count = 0;
  int ret;
  int i = 0;
  sqlite3_stmt *stmt;
  
  /* if the depends are set, there's nothing for us to do */
  
  if (pkg->depends != NULL) 
    return MPORT_OK;
    
  if (mport_db_prepare(mport->db, &stmt, "SELECT COUNT(*) FROM depends WHERE pkg=%Q", pkg->name) != MPORT_OK)
    RETURN_CURRENT_ERROR;
    
  if (sqlite3_step(stmt) != SQLITE_ROW) {
    sqlite3_finalize(stmt);
    RETURN_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(mport->db));
  }
  
  count = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);
  
  if (count == 0) 
    return MPORT_OK;    /* XXX is there some way to optimize repeated lookups away? */
  
  pkg->depends = (char **)calloc((1+count), sizeof(char **));
  
  if (mport_db_prepare(mport->db, &stmt, "SELECT depend_pkgname FROM depends WHERE pkg=%Q", pkg->name) != MPORT_OK) 
    RETURN_CURRENT_ERROR;

  while (1) {
    ret = sqlite3_step(stmt);
    
    if (ret == SQLITE_DONE) {
      break;
    } else if (ret == SQLITE_ROW) {
      pkg->depends[i] = strdup(sqlite3_column_text(stmt, 0));
      i++;
    } else {
      sqlite3_finalize(stmt);
      RETURN_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(mport->db));
    } 
  } 
  
  /* pad the last cell in the array with null */
  pkg->depends[i] = NULL;
  
  sqlite3_finalize(stmt);
  return MPORT_OK; 
}  




int mport_pkgmeta_get_assetlist(mportInstance *mport, mportPackageMeta *pkg, mportAssetList **alist_p)
{
  mportAssetList *alist;
  sqlite3_stmt *stmt;
  int ret;
  mportAssetListEntry *e;
  
  if ((alist = mport_assetlist_new()) == NULL)
    return MPORT_ERR_NO_MEM;

  *alist_p = alist;
  
  if (mport_db_prepare(mport->db, &stmt, "SELECT type, data FROM assets WHERE pkg=%Q", pkg->name) != MPORT_OK)
    RETURN_CURRENT_ERROR;
    
  while (1) {
    ret = sqlite3_step(stmt);
    
    if (ret == SQLITE_DONE)
      break;
      
    if (ret != SQLITE_ROW) {
      sqlite3_finalize(stmt);
      RETURN_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(mport->db));
    }
    
    e = (mportAssetListEntry *)malloc(sizeof(mportAssetListEntry));
    
    if (e == NULL) {
      sqlite3_finalize(stmt);
      return MPORT_ERR_NO_MEM;
    }
    
    e->type = sqlite3_column_int(stmt, 0);
    e->data = strdup(sqlite3_column_text(stmt, 1));
    
    if (e->data == NULL) {
      sqlite3_finalize(stmt);
      return MPORT_ERR_NO_MEM;
    }
  }
  
  sqlite3_finalize(stmt);
  return MPORT_OK;
}


static int populate_vec_from_stmt(mportPackageMeta ***ref, int len, sqlite3 *db, sqlite3_stmt *stmt)
{ 
  mportPackageMeta **vec;
  int done = 0;
  vec  = (mportPackageMeta**)malloc((1+len) * sizeof(mportPackageMeta *));
  *ref = vec;

  while (!done) { 
    switch (sqlite3_step(stmt)) {
      case SQLITE_ROW:
        *vec = mport_pkgmeta_new();
        if (*vec == NULL)
          RETURN_ERROR(MPORT_ERR_NO_MEM, "Couldn't allocate meta."); 
        if (populate_meta_from_stmt(*vec, db, stmt) != MPORT_OK)
          RETURN_CURRENT_ERROR;
        vec++;
        break;
      case SQLITE_DONE:
        /* set the last cell in the array to null */
        *vec = NULL;
        done++;
        break;
      default:
        RETURN_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(db));
        break; /* not reached */
    }
  }
  
  /* not reached */
  return MPORT_OK;
}



static int populate_meta_from_stmt(mportPackageMeta *pack, sqlite3 *db, sqlite3_stmt *stmt) 
{  
  const char *tmp = 0;

  /* Copy pkg to pack->name */
  if ((tmp = sqlite3_column_text(stmt, 0)) == NULL) 
    RETURN_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(db));

  if ((pack->name = strdup(tmp)) == NULL)
    return MPORT_ERR_NO_MEM;

  /* Copy version to pack->version */
  if ((tmp = sqlite3_column_text(stmt, 1)) == NULL) 
    RETURN_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(db));
  
  if ((pack->version = strdup(tmp)) == NULL)
    return MPORT_ERR_NO_MEM;
  
  /* Copy origin to pack->origin */
  if ((tmp = sqlite3_column_text(stmt, 2)) == NULL) 
    RETURN_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(db));
  
  if ((pack->origin = strdup(tmp)) == NULL)
    return MPORT_ERR_NO_MEM;

  /* Copy lang to pack->lang */
  if ((tmp = sqlite3_column_text(stmt, 3)) == NULL) 
    RETURN_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(db));
  
  if ((pack->lang = strdup(tmp)) == NULL)
    return MPORT_ERR_NO_MEM;

  /* Copy prefix to pack->prefix */
  if ((tmp = sqlite3_column_text(stmt, 4)) == NULL) 
    RETURN_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(db));
  
  if ((pack->prefix = strdup(tmp)) == NULL)
    return MPORT_ERR_NO_MEM;


  /* Copy comment to pack->comment */
  if ((tmp = sqlite3_column_text(stmt, 5)) == NULL) 
    RETURN_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(db));
  
  if ((pack->comment = strdup(tmp)) == NULL)
    return MPORT_ERR_NO_MEM;

  
  return MPORT_OK;
}


