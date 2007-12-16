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
 * $MidnightBSD: src/lib/libmport/inst_init.c,v 1.1 2007/11/22 08:00:32 ctriv Exp $
 */



#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <sqlite3.h>
#include <md5.h>
#include "mport.h"

__MBSDID("$MidnightBSD: src/lib/libmport/inst_init.c,v 1.1 2007/11/22 08:00:32 ctriv Exp $");



static int run_pkg_deinstall(mportInstance *, mportPackageMeta *, const char *);
static int check_for_upwards_depends(mportInstance *, mportPackageMeta *);

int mport_delete_name_primative(mportInstance *mport, const char *name, int force)
{
  mportPackageMeta *pkg;
  int ret;
  
  if (mport_get_meta_from_master(mport->db, &pkg, name) != MPORT_OK)
    RETURN_CURRENT_ERROR;
  
  if (pkg == NULL) {
    mport_free_packagemeta(pkg);
    RETURN_ERRORX(MPORT_ERR_NO_SUCH_PKG, "Package %s is not installed.", name);
  }
  
  ret = mport_delete_primative(mport, pkg, force);
  
  mport_free_packagemeta(pkg);
  
  return ret;
}
 

int mport_delete_primative(mportInstance *mport, mportPackageMeta *pack, int force) 
{
  sqlite3_stmt *stmt;
  int ret;
  mportPlistEntryType type;
  char *data, *checksum, *cwd;
  char md5[33], file[FILENAME_MAX];
  
  if (force != 0) {
    if (check_for_upwards_depends(mport, pack) != MPORT_OK)
      RETURN_CURRENT_ERROR;
  }
  
  if (mport_db_prepare(mport->db, &stmt, "SELECT type,data,checksum FROM assets WHERE pkg=?", pack->name) != MPORT_OK)
    RETURN_CURRENT_ERROR;  
  
  cwd = pack->prefix;
    
  while (1) {
    ret = sqlite3_step(stmt);
    
    if (ret == SQLITE_DONE)
      break;
      
    if (ret != SQLITE_ROW) {
      /* some error occured */
      SET_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(mport->db));
      sqlite3_finalize(stmt);
      RETURN_CURRENT_ERROR;
    }
    
    type     = (mportPlistEntryType)sqlite3_column_int(stmt, 0);
    data     = (char *)sqlite3_column_text(stmt, 1);
    checksum = (char *)sqlite3_column_text(stmt, 2);
    
    switch (type) {
      case PLIST_CWD:
        cwd = data == NULL ? pack->prefix : data;
        break;
      case PLIST_FILE:
        if (MD5File(file, md5) == NULL) {
          sqlite3_finalize(stmt);
          RETURN_ERRORX(MPORT_ERR_FILE_NOT_FOUND, "File not found: %s", file);
        }
        
        if (strcmp(md5, checksum) != 0) {
          sqlite3_finalize(stmt);
          RETURN_ERRORX(MPORT_ERR_CHECKSUM_MISMATCH, "Checksum mismatch: %s", file);
        }
        
        if (unlink(file) != 0) {
          sqlite3_finalize(stmt);
          RETURN_ERROR(MPORT_ERR_SYSCALL_FAILED, strerror(errno));
        }
        
        break;
      case PLIST_UNEXEC:
        if (mport_run_plist_exec(mport, data, cwd, file) != MPORT_OK) {
          sqlite3_finalize(stmt);
          RETURN_CURRENT_ERROR;
        }
        break;
      case PLIST_DIRRM:
      case PLIST_DIRRMTRY:
        (void)snprintf(file, FILENAME_MAX, "%s%s/%s", mport->root, cwd, data);
        
        if (mport_rmdir(file, type == PLIST_DIRRMTRY ? 1 : 0) != MPORT_OK) {
          sqlite3_finalize(stmt);
          RETURN_CURRENT_ERROR;
        }
        
        break;
      default:
        /* do nothing */
        break;
    }
  }
  
  sqlite3_finalize(stmt);
  return run_pkg_deinstall(mport, pack, "POST-DEINSTALL");        
} 
  

static int run_pkg_deinstall(mportInstance *mport, mportPackageMeta *pack, const char *mode)
{
  char file[FILENAME_MAX];
  int ret;
  
  (void)snprintf(file, FILENAME_MAX, "%s/%s-%s/%s", MPORT_INST_INFRA_DIR, pack->name, pack->version, MPORT_INSTALL_FILE);    

  if (mport_file_exists(file)) {
    if ((ret = mport_xsystem(mport, "PKG_PREFIX=%s %s %s %s", pack->prefix, MPORT_SH_BIN, file, mode)) != 0)
      RETURN_ERRORX(MPORT_ERR_SYSCALL_FAILED, "%s %s returned non-zero: %i", MPORT_INSTALL_FILE, mode, ret);
  }
  
  return MPORT_OK;
}


static int check_for_upwards_depends(mportInstance *mport, mportPackageMeta *pack)
{
  /* XXX - write me */
  return MPORT_OK;
}
