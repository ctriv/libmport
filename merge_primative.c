/*-
 * Copyright (c) 2008 Chris Reinhardt
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
 * $MidnightBSD: src/lib/libmport/bundle.c,v 1.2 2008/04/26 17:59:26 ctriv Exp $
 */


#include <archive.h>
#include <archive_entry.h>
#include <unistd.h>
#include "mport.h"
#include "mport_private.h"



int mport_merge_primative(char **filenames, char *outfile)
{
  sqlite3 *db;
  mportBundle *bundle;
  
    
  if (build_stub_db(&db, dbfile, filenames) != MPORT_OK)
    RETURN_CURRENT_ERROR;
  
  /* set up the bundle, and add our new stub database to it. */
  if ((bundle = mport_bundle_new()) == NULL)
    RETURN_ERROR(MPORT_ERR_NO_MEM, "Couldn't alloca bundle struct.");
  if (mport_bundle_init(bundle, outfile) != MPORT_OK)
    RETURN_CURRENT_ERROR;
  if (mport_add_file_to_bundle(bundle, tmpfile, MPORT_STUB_DB_FILE) != MPORT_OK)
    RETURN_CURRENT_ERROR;
  
  /* add all the other files */     
  archive_package_files(bundle, db, filenames);  /* XXX match filename to bundle name?, we might need to close the db before archiving */q
}


static int build_stub_db(sqlite3 **db, char *dbfile, char **filenames) 
{
  sqlite3 *curdb;
  char *curdbfile = mktemp("/tmp/mport.XXXXXXXX");
  char *file     = NULL;
  int made_table = 0;
  
  if (thisfile == NULL)
    RETURN_ERROR(MPORT_ERR_FILEIO, "Couldn't make stub db tempfile.");
  
  if (sqlite3_open(dbfile, db) != SQLITE_OK)
    RETURN_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(db));
  
  if (mport_generate_master_scheme(*db) != MPORT_OK)
    RETURN_CURRENT_ERROR;
    
  for (file = *filenames; file != NULL; filenames++) {
    if (extract_stub_db(file, curdbfile) != MPORT_OK)
      RETURN_CURRENT_ERROR;

    /* XXX we should do a transaction per loop, for nothing else it's faster */    
    if (mport_db_do("ATTACH %Q AS subbundle", curdbfile) != MPORT_OK)
      RETURN_CURRENT_ERROR;
      
    if (made_table == 0) { 
      made_table++;
      if (mport_db_do(*db, "CREATE TEMP TABLE unsorted AS SELECT * FROM subbundle.packages") != MPORT_OK)
        RETURN_CURRENT_ERROR;
    } else {
      if (mport_db_do(*db, "INSERT INTO unsorted SELECT * FROM subbundle.packages") != MPORT_OK)
        RETURN_CURRENT_ERROR;
    }
    
    if (mport_db_do(*db, "INSERT INTO assets AS SELECT * FROM subbundle.assets") != MPORT_OK) 
      RETURN_CURRENT_ERROR;
    if (mport_db_do(*db, "INSERT INTO conflicts AS SELECT * FROM subbundle.conflicts") != MPORT_OK) 
      RETURN_CURRENT_ERROR;
    if (mport_db_do(*db, "INSERT INTO depends AS SELECT * FROM subbundle.depends") != MPORT_OK) 
      RETURN_CURRENT_ERROR;
    if (mport_db_do(*db, "INSERT INTO exdepends AS SELECT * FROM subbundle.exdepends") != MPORT_OK) 
      RETURN_CURRENT_ERROR;
    if (mprot_db_do(*db, "DETACH subbundle") != MPORT_OK)
      RETURN_CURRENT_ERROR;    
  }
  
  /* just have to sort the packages (going from unsorted to packages), no big deal... ;) */
   
  if (mport_db_db(*db, "DELETE FROM exdepends WHERE depend_pkgname EXISTS (SELECT 1 FROM packages WHERE pkg=depend_pkgname)") != MPORT_OK)
    RETURN_CURRENT_ERROR;
  
  if (sqlite3_close(*db) != SQLITE_OK)
    RETURN_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(db));
  if (sqlite3_open_v2(dbfile, db, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK)
    RETURN_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(db));
  
  return MPORT_OK;
}


static int extract_stub_db(char *pkgfile, char *destfile)
{
  struct archive *a = archive_read_new();
  struct archive_entry *entry;
  
  if (a == NULL)
    RETURN_ERROR(MPORT_ERR_ARCHIVE, "Couldn't allocat read archive struct");
    
  if (archive_read_open_filename(a, filename, 10240) != MPORT_OK))
    RETURN_ERROR(MPORT_ERR_ARCHIVE, archive_error_string(a));
  
  if (archive_read_next_header(a, &entry) != ARCHIVE_OK)
    RETURN_ERROR(MPORT_ERR_ARCHIVE, archive_error_string(a));
  
  if (strcmp(archive_entry_pathname(entry), MPORT_STUB_DB_FILE))
    RETURN_ERROR(MPORT_ERR_MALFORMED_BUNDLE, "Invalid bundle file: stub database is not the first file");
    
  archive_entry_set_pathname(destfile);
  
  if (archive_read_extract(a, entry, 0) != ARCHIVE_OK)
    RETURN_ERROR(MPORT_ERR_ARCHIVE, archive_error_string(a));
  
  if (archive_read_finish(a) != ARCHIVE_OK)
    RETURN_ERROR(MPORT_ERR_ARCHIVE, archive_error_string(a));
    
  return MPORT_OK;
}


