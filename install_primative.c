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
 * $MidnightBSD: src/lib/libmport/install_primative.c,v 1.2 2008/04/26 17:59:26 ctriv Exp $
 */



#include "mport.h"
#include "mport_private.h"
#include <sys/cdefs.h>
#include <sys/stat.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <archive.h>
#include <archive_entry.h>

static int do_pre_install(mportInstance *, mportPackageMeta *, const char *);
static int do_actual_install(mportInstance *, mportBundleRead *, mportPackageMeta *, const char *);
static int do_post_install(mportInstance *, mportPackageMeta *, const char *);
static int run_pkg_install(mportInstance *, const char *, mportPackageMeta *, const char *);
static int run_mtree(mportInstance *, const char *, mportPackageMeta *);
static int display_pkg_msg(mportInstance *, mportPackageMeta *, const char *);
static int clean_up(mportInstance *, const char *);
static int fail(mportInstance *, mportPackageMeta *, const char *);
static int rollback(void);


static int fail(mportInstance *mport, mportPackageMeta *pkg, const char *depend)
{
  RETURN_ERRORX(MPORT_ERR_MISSING_DEPEND, "%s depends on %s, which is not installed.", pkg->name, depend);
}


MPORT_PUBLIC_API mport_install_primative(mportInstance *mport, const char *filename, const char *prefix) 
{
  return mport_install_handler(mport, &fail, filename, prefix);
}


/* this the private API that the library uses, with a flexible callback for resolving depends */
int mport_install_handler(mportInstance *mport, mport_depend_resolver resolver, const char *filename, const char *prefix)
{
  /* 
   * The general strategy here is to extract the meta-files into a tempdir, but
   * extract the real files inplace.  There's huge IO overhead with having a stagging
   * area. 
   */
  mportBundleRead *bundle;
  char *tmpdir;
  sqlite3 *db = mport->db;
  mportPackageMeta **packs;
  mportPackageMeta *pack;
  int i, is_update;
  
  if ((bundle = mport_bundle_read_new()) == NULL)
    return MPORT_ERR_NO_MEM;
    
  if (mport_bundle_read_init(bundle, filename) != MPORT_OK)
    RETURN_CURRENT_ERROR;
  
  /* extract the meta-files into the a temp dir */  
  if (mport_bundle_read_extract_metafiles(bundle, &tmpdir) != MPORT_OK)
    RETURN_CURRENT_ERROR;  

  /* Attach the stub db */
  if (mport_attach_stub_db(db, tmpdir) != MPORT_OK) 
    RETURN_CURRENT_ERROR;

  /* get the meta objects from the stub database */  
  if (mport_get_meta_from_stub(db, &packs) != MPORT_OK)
    RETURN_CURRENT_ERROR;

  for (i=0; *(packs + i) != NULL; i++) {
    pack  = *(packs + i);    

    /* overwrite the prefix with the one we were given */
    if (prefix != NULL) {
      free(pack->prefix);
      pack->prefix = strdup(prefix);
    }

    /* if the package has been installed, we update instead! */
    if (mport_older_pkg_is_installed(mport, pack)) {
      is_update = 1;
      if (mport_update_prepare(mport, pack) != MPORT_OK) {
        mport_call_msg_cb(mport, "Unable to prepare %s-%s for update: %s", pack->name, pack->version, mport_err_string());
        mport_set_err(0, NULL);
        break;
      }
    } else {
      is_update = 0;
    }	
    
    /* check if this is installed already, depends, and conflicts */
    if ((mport_check_install_preconditions(mport, pack, resolver) != MPORT_OK)
              ||
        (do_pre_install(mport, pack, tmpdir) != MPORT_OK)
              ||
        (do_actual_install(mport, bundle, pack, tmpdir) != MPORT_OK)
              ||
        (do_post_install(mport, pack, tmpdir) != MPORT_OK))
    {
      mport_call_msg_cb(mport, "Unable to install %s-%s: %s", pack->name, pack->version, mport_err_string());
      mport_set_err(0, NULL);
      
      if (is_update) {
        if (mport_update_restore_old(mport, pack) != MPORT_OK)
          RETURN_CURRENT_ERROR; /* fatal error, cannot continue */
      }
      
      break;
    }
  } 
  
  mport_packagemeta_vec_free(packs);
  mport_bundle_read_finish(bundle);
    
  if (clean_up(mport, tmpdir) != MPORT_OK)
    RETURN_CURRENT_ERROR;
  
  return MPORT_OK;
}

/* This does everything that has to happen before we start installing files.
 * We run mtree, pkg-install PRE-INSTALL, etc... 
 */
static int do_pre_install(mportInstance *mport, mportPackageMeta *pack, const char *tmpdir)
{
  int ret = MPORT_OK;
  
  /* run mtree */
  if ((ret = run_mtree(mport, tmpdir, pack)) != MPORT_OK)
    return ret;
  
  /* run pkg-install PRE-INSTALL */
  if ((ret = run_pkg_install(mport, tmpdir, pack, "PRE-INSTALL")) != MPORT_OK)
    return ret;

  return ret;    
}


static int do_actual_install(
      mportInstance *mport, 
      mportBundleRead *bundle,
      mportPackageMeta *pack, 
      const char *tmpdir
    )
{
  int file_total, ret;
  int file_count = 0;
  mportAssetListEntryType type;
  struct archive_entry *entry;
  char *data, *checksum, *orig_cwd; 
  char file[FILENAME_MAX], cwd[FILENAME_MAX], dir[FILENAME_MAX];
  sqlite3_stmt *assets, *count, *insert;
  sqlite3 *db;
  
  db = mport->db;

  /* sadly, we can't just use abs pathnames, because it will break hardlinks */
  orig_cwd = getcwd(NULL, 0);

  /* get the file count for the progress meter */
  if (mport_db_prepare(db, &count, "SELECT COUNT(*) FROM stub.assets WHERE type=%i AND pkg=%Q", ASSET_FILE, pack->name) != MPORT_OK)
    RETURN_CURRENT_ERROR;

  switch (sqlite3_step(count)) {
    case SQLITE_ROW:
      file_total = sqlite3_column_int(count, 0);
      sqlite3_finalize(count);
      break;
    default:
      SET_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(db));
      sqlite3_finalize(count);
      RETURN_CURRENT_ERROR;
  }
  
  (mport->progress_init_cb)();
  

  /* Insert the package meta row into the packages table (We use pack here because things might have been twiddled) */
  /* Note that this will be marked as dirty by default */  
  if (mport_db_do(db, "INSERT INTO packages (pkg, version, origin, prefix, lang, options) VALUES (%Q,%Q,%Q,%Q,%Q,%Q)", pack->name, pack->version, pack->origin, pack->prefix, pack->lang, pack->options) != MPORT_OK)
    goto ERROR;

  /* Insert the assets into the master table (We do this one by one because we want to insert file 
   * assets as absolute paths. */
  if (mport_db_prepare(db, &insert, "INSERT INTO assets (pkg, type, data, checksum) values (%Q,?,?,?)", pack->name) != MPORT_OK)
    goto ERROR;  
  /* Insert the depends into the master table */
  if (mport_db_do(db, "INSERT INTO depends (pkg, depend_pkgname, depend_pkgversion, depend_port) SELECT pkg,depend_pkgname,depend_pkgversion,depend_port FROM stub.depends WHERE pkg=%Q", pack->name) != MPORT_OK) 
    goto ERROR;
  
  if (mport_db_prepare(db, &assets, "SELECT type,data,checksum FROM stub.assets WHERE pkg=%Q", pack->name) != MPORT_OK) 
    goto ERROR;

  (void)strlcpy(cwd, pack->prefix, sizeof(cwd));
  
  if (mport_chdir(mport, cwd) != MPORT_OK)
    goto ERROR;

  while (1) {
    ret = sqlite3_step(assets);
    
    if (ret == SQLITE_DONE) 
      break;
    
    if (ret != SQLITE_ROW) {
      SET_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(db));
      goto ERROR;
    }
    
    type     = (mportAssetListEntryType)sqlite3_column_int(assets, 0);
    data     = (char *)sqlite3_column_text(assets, 1);
    checksum = (char *)sqlite3_column_text(assets, 2);  
    
    switch (type) {
      case ASSET_CWD:      
        (void)strlcpy(cwd, data == NULL ? pack->prefix : data, sizeof(cwd));
        if (mport_chdir(mport, cwd) != MPORT_OK)
          goto ERROR;
          
        break;
      case ASSET_EXEC:
        if (mport_run_asset_exec(mport, data, cwd, file) != MPORT_OK)
          goto ERROR;
        break;
      case ASSET_FILE:
        if (mport_bundle_read_next_entry(bundle, &entry) != MPORT_OK)
          goto ERROR;
        
        (void)snprintf(file, FILENAME_MAX, "%s%s/%s", mport->root, cwd, data);

        archive_entry_set_pathname(entry, file);

        if (mport_bundle_read_extract_next_file(bundle, entry) != MPORT_OK) 
          goto ERROR;
        
        (mport->progress_step_cb)(++file_count, file_total, file);
        
        break;
      default:
        /* do nothing */
        break;
    }
    
    /* insert this assest into the master database */
    if (sqlite3_bind_int(insert, 1, (int)type) != SQLITE_OK) {
      SET_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(db));    
      goto ERROR;
    }
    if (type == ASSET_FILE) {
      /* don't put the root in the database! */
      if (sqlite3_bind_text(insert, 2, file + strlen(mport->root), -1, SQLITE_STATIC) != SQLITE_OK) {
        SET_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(db));
        goto ERROR;
      }
      if (sqlite3_bind_text(insert, 3, checksum, -1, SQLITE_STATIC) != SQLITE_OK) {
        SET_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(db));
        goto ERROR;
      }
    } else if (type == ASSET_DIRRM || type == ASSET_DIRRMTRY) {
      (void)snprintf(dir, FILENAME_MAX, "%s/%s", cwd, data);
      if (sqlite3_bind_text(insert, 2, dir, -1, SQLITE_STATIC) != SQLITE_OK) {
        SET_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(db));
        goto ERROR;
      }
    } else {  
      if (sqlite3_bind_text(insert, 2, data, -1, SQLITE_STATIC) != SQLITE_OK) {
        SET_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(db));
        goto ERROR;
      }
      
      if (sqlite3_bind_null(insert, 3) != SQLITE_OK) {
        SET_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(db));
        goto ERROR;
      }
    }
     
    if (sqlite3_step(insert) != SQLITE_DONE) {
      SET_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(db));
      goto ERROR;
    }
                        
    sqlite3_reset(insert);
  }

  sqlite3_finalize(assets); 
  sqlite3_finalize(insert);
  
  if (mport_db_do(db, "UPDATE packages SET status='clean' WHERE pkg=%Q", pack->name) != MPORT_OK) 
    goto ERROR;
    
  (mport->progress_free_cb)();
  (void)mport_chdir(NULL, orig_cwd);
  free(orig_cwd);
  return MPORT_OK;
  
  ERROR:
    (mport->progress_free_cb)();
    free(orig_cwd);
    rollback();
    RETURN_CURRENT_ERROR;
}           

static int do_post_install(mportInstance *mport, mportPackageMeta *pack, const char *tmpdir)
{
  char to[FILENAME_MAX], from[FILENAME_MAX];
  (void)snprintf(from, FILENAME_MAX, "%s/%s/%s-%s/%s", tmpdir, MPORT_STUB_INFRA_DIR, pack->name, pack->version, MPORT_DEINSTALL_FILE);
  
  if (mport_file_exists(from)) {
    (void)snprintf(to, FILENAME_MAX, "%s%s/%s-%s/%s", mport->root, MPORT_INST_INFRA_DIR, pack->name, pack->version, MPORT_DEINSTALL_FILE);
    
    if (mport_mkdir(dirname(to)) != MPORT_OK)
      RETURN_CURRENT_ERROR;
  
    if (mport_copy_file(from, to) != MPORT_OK)
      RETURN_CURRENT_ERROR;
  }

  if (display_pkg_msg(mport, pack, tmpdir) != MPORT_OK)
    RETURN_CURRENT_ERROR;
  
  return run_pkg_install(mport, tmpdir, pack, "POST-INSTALL");
}


      
static int run_mtree(mportInstance *mport, const char *tmpdir, mportPackageMeta *pack)
{
  char file[FILENAME_MAX];
  int ret;
  
  (void)snprintf(file, FILENAME_MAX, "%s/%s/%s-%s/%s", tmpdir, MPORT_STUB_INFRA_DIR, pack->name, pack->version, MPORT_MTREE_FILE);
  
  if (mport_file_exists(file)) {
    if ((ret = mport_xsystem(mport, "%s -U -f %s -d -e -p %s >/dev/null", MPORT_MTREE_BIN, file, pack->prefix)) != 0) 
      RETURN_ERRORX(MPORT_ERR_SYSCALL_FAILED, "%s returned non-zero: %i", MPORT_MTREE_BIN, ret);
  }
  
  return MPORT_OK;
}



static int run_pkg_install(mportInstance *mport, const char *tmpdir, mportPackageMeta *pack, const char *mode)
{
  char file[FILENAME_MAX];
  int ret;
  
  (void)snprintf(file, FILENAME_MAX, "%s/%s/%s-%s/%s", tmpdir, MPORT_STUB_INFRA_DIR, pack->name, pack->version, MPORT_INSTALL_FILE);    
 
  if (mport_file_exists(file)) {
    if (chmod(file, 755) != 0)
      RETURN_ERRORX(MPORT_ERR_SYSCALL_FAILED, "chmod(%s, 755): %s", file, strerror(errno));
      
    if ((ret = mport_xsystem(mport, "PKG_PREFIX=%s %s %s %s", pack->prefix, file, pack->name, mode)) != 0)
      RETURN_ERRORX(MPORT_ERR_SYSCALL_FAILED, "%s %s returned non-zero: %i", MPORT_INSTALL_FILE, mode, ret);
  }
  
 return MPORT_OK;
}
 


static int display_pkg_msg(mportInstance *mport, mportPackageMeta *pack, const char *tmpdir)
{
  char filename[FILENAME_MAX];
  char *buf;
  struct stat st;
  FILE *file;
  
  (void)snprintf(filename, FILENAME_MAX, "%s/%s/%s-%s/%s", tmpdir, MPORT_STUB_INFRA_DIR, pack->name, pack->version, MPORT_MESSAGE_FILE);
  
  if (stat(filename, &st) == -1) 
    /* if we couldn't stat the file, we assume there isn't a pkg-msg */
    return MPORT_OK;
    
  if ((file = fopen(filename, "r")) == NULL) 
    RETURN_ERRORX(MPORT_ERR_FILEIO, "Couldn't open %s: %s", filename, strerror(errno));
  
  if ((buf = (char *)malloc((st.st_size + 1) * sizeof(char))) == NULL)
    return MPORT_ERR_NO_MEM;

  
  if (fread(buf, 1, st.st_size, file) != st.st_size) {
    free(buf);
    RETURN_ERRORX(MPORT_ERR_FILEIO, "Read error: %s", strerror(errno));
  }
  
  buf[st.st_size] = 0;
  
  mport_call_msg_cb(mport, buf);
  
  free(buf);
  
  return MPORT_OK;
}
 
 

static int clean_up(mportInstance *mport, const char *tmpdir) 
{
  if (mport_detach_stub_db(mport->db) != MPORT_OK)
    RETURN_CURRENT_ERROR;
    
#ifdef DEBUG
  return MPORT_OK;
#else
  return mport_rmtree(tmpdir);
#endif
}


static int rollback()
{
  fprintf(stderr, "Rollback called!\n");
  return 0;
}

