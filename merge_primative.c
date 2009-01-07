/*-
 * Copyright (c) 2008,2009 Chris Reinhardt
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "mport.h"
#include "mport_private.h"

/* build a hashtable with pkgname keys, values are a struct with fields for
 * filename, and boolean is_in_db */
struct table_entry {
  char *file;
  short is_in_db;
  char *name;
  struct table_entry *next;
};

#define TABLE_SIZE 128

static int build_stub_db(sqlite3 **, const char *, const char *, const char **, struct table_entry **); 
static int archive_package_files(mportBundle *, sqlite3 *, struct table_entry **);

static int extract_stub_db(const char *, const char *);
static int insert_into_table(struct table_entry **, char *, const char *);
static uint32_t SuperFastHash(const char *);


#include <err.h>

int mport_merge_primative(const char **filenames, const char *outfile)
{
  sqlite3 *db;
  mportBundle *bundle;
  struct table_entry **table;
  char tmpdir[] = "/tmp/mport.XXXXXXXX";
  char *dbfile;
  
  if ((table = (struct table_entry **)calloc(TABLE_SIZE, sizeof(struct table_entry *))) == NULL)
    RETURN_ERROR(MPORT_ERR_NO_MEM, "Couldn't allocate hash table.");
  
  warnx("mport_merge_primative(%p, %s)", filenames, outfile);
  
  if (mkdtemp(tmpdir) == NULL)
    RETURN_ERROR(MPORT_ERR_FILEIO, "Couldn't make temp directory.");
  if (asprintf(&dbfile, "%s/%s", tmpdir, "merged.db") == -1)
    RETURN_ERROR(MPORT_ERR_NO_MEM, "Couldn't build merge database name.");
  
  warnx("Building stub");
      
  if (build_stub_db(&db, tmpdir, dbfile, filenames, table) != MPORT_OK)
    RETURN_CURRENT_ERROR;
  
  warnx("Stub complete");
  
  
  /* set up the bundle, and add our new stub database to it. */
  if ((bundle = mport_bundle_new()) == NULL)
    RETURN_ERROR(MPORT_ERR_NO_MEM, "Couldn't alloca bundle struct.");
  if (mport_bundle_init(bundle, outfile) != MPORT_OK)
    RETURN_CURRENT_ERROR;
  if (mport_bundle_add_file(bundle, dbfile, MPORT_STUB_DB_FILE) != MPORT_OK)
    RETURN_CURRENT_ERROR;
  
  /* add all the other files */     
  archive_package_files(bundle, db, table);  
 
 /*if (mport_rmtree(tmpdir) != MPORT_OK)
   RETURN_CURRENT_ERROR; */
 
 return MPORT_OK;
}


static int build_stub_db(sqlite3 **db,  const char *tmpdir,  const char *dbfile,  const char **filenames, struct table_entry **table) 
{
  char *tmpdbfile, *name;
  const char *file     = NULL;
  int made_table = 0, ret;
  sqlite3_stmt *stmt;
  
  if (asprintf(&tmpdbfile, "%s/%s", tmpdir, "pkg.db") == -1)
    RETURN_ERROR(MPORT_ERR_FILEIO, "Couldn't make stub db tempfile.");
  
  
  if (sqlite3_open(dbfile, db) != SQLITE_OK)
    RETURN_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(*db));
  
  if (mport_generate_stub_schema(*db) != MPORT_OK)
    RETURN_CURRENT_ERROR;
    
  for (file = *filenames; file != NULL; file = *(++filenames)) {
    warnx("Visiting %s", file);
    if (extract_stub_db(file, tmpdbfile) != MPORT_OK)
      RETURN_CURRENT_ERROR;

    /* XXX we should do a transaction per loop, for nothing else it's faster */    
    if (mport_db_do(*db, "ATTACH %Q AS subbundle", tmpdbfile) != MPORT_OK)
      RETURN_CURRENT_ERROR;
      
    if (made_table == 0) { 
      made_table++;
      if (mport_db_do(*db, "CREATE TABLE unsorted AS SELECT * FROM subbundle.packages") != MPORT_OK)
        RETURN_CURRENT_ERROR;
    } else {
      if (mport_db_do(*db, "INSERT INTO unsorted SELECT * FROM subbundle.packages") != MPORT_OK)
        RETURN_CURRENT_ERROR;
    }
    
    if (mport_db_do(*db, "INSERT INTO assets SELECT * FROM subbundle.assets") != MPORT_OK) 
      RETURN_CURRENT_ERROR;
    if (mport_db_do(*db, "INSERT INTO conflicts SELECT * FROM subbundle.conflicts") != MPORT_OK) 
      RETURN_CURRENT_ERROR;
    if (mport_db_do(*db, "INSERT INTO depends SELECT * FROM subbundle.depends") != MPORT_OK) 
      RETURN_CURRENT_ERROR;
    if (mport_db_do(*db, "DETACH subbundle") != MPORT_OK)
      RETURN_CURRENT_ERROR;    

    /* build our hashtable (pkgname => metadata) up */      
    if (mport_db_prepare(*db, &stmt, "SELECT pkg FROM subbundle.packages") != MPORT_OK)
      RETURN_CURRENT_ERROR;
    
    while (1) {
      ret = sqlite3_step(stmt);
      
      if (ret == SQLITE_ROW) {
        name = (char *)sqlite3_column_text(stmt, 0);
        if (insert_into_table(table, name, file) != MPORT_OK) {
          sqlite3_finalize(stmt);
          RETURN_CURRENT_ERROR;
        }
      } else if (ret == SQLITE_DONE) {
        break;
      } else {
        SET_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(*db));
        sqlite3_finalize(stmt);
        RETURN_CURRENT_ERROR;
      }
    }
    
    sqlite3_finalize(stmt);
  }

  /* just have to sort the packages (going from unsorted to packages), no big deal... ;) */
  while (1) {
    if (mport_db_do(*db, "INSERT INTO packages SELECT * FROM unsorted WHERE NOT EXISTS (SELECT 1 FROM packages WHERE packages.pkg=unsorted.pkg) AND (NOT EXISTS (SELECT 1 FROM depends WHERE depends.pkg=unsorted.pkg) OR NOT EXISTS (SELECT 1 FROM depends LEFT JOIN packages ON depends.depend=packages.pkg WHERE depends.pkg=unsorted.pkg AND packages.pkg ISNULL))") != MPORT_OK)
      RETURN_CURRENT_ERROR;
    if (sqlite3_changes(*db) == 0) /* if there is nothing left to insert, we're done */
      break;
  }
      
  /* Check that unsorted and packages have the same number of rows. */     
  if (mport_db_prepare(*db, &stmt, "SELECT COUNT(DISTINCT pkg) FROM packages")
    RETURN_CURRENT_ERROR;  
  if (sqlite3_step(stmt)) != SQLITE_ROW) {
    SET_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(*db));
    sqlite3_finalize(stmt);
    RETURN_CURRENT_ERROR;
  }
  
  int pkgs = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);
  
  if (mport_db_prepare(*db, &stmt, "SELECT COUNT(DISTINCT pkg) FROM unsorted")
    RETURN_CURRENT_ERROR;  
  if (sqlite3_step(stmt)) != SQLITE_ROW) {
    SET_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(*db));
    sqlite3_finalize(stmt);
    RETURN_CURRENT_ERROR;
  }
  
  int unsort = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);
  
  if (pkgs != unsort) 
    RETURN_ERRORX(MPORT_ERR_INTERNAL, "Sorted (%i) and unsorted (%i) counts do no match.", pkgs, unsort);
    
      
  /* Close the stub database handle, and reopen as read only to insure that we don't
   * try to change it after this point 
   */    
  if (sqlite3_close(*db) != SQLITE_OK)
    RETURN_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(*db));
  if (sqlite3_open_v2(dbfile, db, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK)
    RETURN_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(*db));
  
  return MPORT_OK;
}


static int archive_package_files(mportBundle *bundle, sqlite3 *db, struct table_entry **table)
{
  sqlite3_stmt *stmt;
  int ret;
  table_entry *cur;
  char *name, *file;
  struct archive *a;
  struct archive_entry *entry;
  
  if (mport_db_prepare(db, &stmt, "SELECT pkg FROM packages") != MPORT_OK)
    RETURN_CURRENT_ERROR;
  
  while (1) {
    ret = sqlite3_step(stmt);
    
    if (ret == SQLITE_DONE)
      break;
    
    if (ret != SQLITE_ROW) {
      SET_ERROR(MPORT_ERR_SQLITE, sqlite3_errmsg(*db));
      sqlite3_finalize(stmt);
      RETURN_CURRENT_ERROR;
    }
    
    name = sqlite3_column_text(stmt, 0);
    cur  = find_in_table(table, name);
    
    if (cur == null) {
      sqlite3_finalize(stmt);
      RETURN_ERROX(MPORT_ERR_INTERNAL, "Couldn't find package '%s' in bundle hash table", name);
    }
    
    file = cur->file;
        
    /* open the tar file, copy the files into the current bundle */
    a = archive_read_new();
    
    if (a == NULL)
      RETURN_ERROR(MPOER_ERR_ARCHIVE, "Couldn't allocate read archive struct");
      
    archive_read_support_compression_bzip2(a);
    archive_read_support_format_tar(a);

    if (archive_read_open_filename(a, filename, 10240) != MPORT_OK)
      RETURN_ERROR(MPORT_ERR_ARCHIVE, archive_error_string(a));
  
    
    
    
}



static int extract_stub_db(const char *filename, const char *destfile)
{
  struct archive *a = archive_read_new();
  struct archive_entry *entry;
  
  if (a == NULL)
    RETURN_ERROR(MPORT_ERR_ARCHIVE, "Couldn't allocate read archive struct");

  archive_read_support_compression_bzip2(a);
  archive_read_support_format_tar(a);
    
  if (archive_read_next_header(a, &entry) != ARCHIVE_OK)
    RETURN_ERROR(MPORT_ERR_ARCHIVE, archive_error_string(a));
  
  if (strcmp(archive_entry_pathname(entry), MPORT_STUB_DB_FILE) != 0)
    RETURN_ERROR(MPORT_ERR_MALFORMED_BUNDLE, "Invalid bundle file: stub database is not the first file");
    
  archive_entry_set_pathname(entry, destfile);
  
  if (archive_read_extract(a, entry, 0) != ARCHIVE_OK)
    RETURN_ERROR(MPORT_ERR_ARCHIVE, archive_error_string(a));
  
  if (archive_read_finish(a) != ARCHIVE_OK)
    RETURN_ERROR(MPORT_ERR_ARCHIVE, archive_error_string(a));
    
  return MPORT_OK;
}


/* insert into a name => file pair into the given hash table. */
static int insert_into_table(struct table_entry **table, char *name, const char *file)
{
  struct table_entry *node, *cur;
  int hash = SuperFastHash(name) % TABLE_SIZE;
  
  if ((node = (struct table_entry *)malloc(sizeof(struct table_entry))) == NULL)
    RETURN_ERROR(MPORT_ERR_NO_MEM, "Couldn't allocate table entry");

  node->name     = strdup(name);
  node->file     = strdup(file);
  node->is_in_db = 0;
  node->next     = NULL;
   
  if (table[hash] == NULL) {     
    table[hash] = node;   
  } else {
    cur = table[hash];
    while (cur->next != NULL) 
      cur = cur->next;
  
    cur->next = node;
  }
  
  return MPORT_OK;
}


static struct table_entry * find_in_table(struct table_entry **table, const char *name)
{
  int hash = SuperFastHash(name) % TABLE_SIZE;
  struct table_entry *e;
  
  e = table[hash];  
  while (e != null) {
    if (strcmp(e->name, name) == 0)
      return e;
      
    e = e->next;
  }
  
  return e;
}
      
      
    
      
/* Paul Hsieh's fast hash function, from http://www.azillionmonkeys.com/qed/hash.html */
/* This function has been modified to only work with C strings */
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
                       +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

static uint32_t SuperFastHash(const char * data) 
{
    int len = strlen(data);
    uint32_t hash = len, tmp;
    int rem;

    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
        case 3: hash += get16bits (data);
                hash ^= hash << 16;
                hash ^= data[sizeof (uint16_t)] << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += *data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}
