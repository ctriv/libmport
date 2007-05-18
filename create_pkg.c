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
 * $MidnightBSD$
 */



#include <sys/cdefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sqlite3.h>
#include "mport.h"

__MBSDID("$MidnightBSD: src/usr.sbin/pkg_install/lib/plist.c,v 1.50.2.1 2006/01/10 22:15:06 krion Exp $");

#define PACKAGE_DB_FILENAME "+CONTENTS.db"

static int create_package_db(sqlite3 **);
static int create_plist(sqlite3 *, Plist *);
static int create_meta(sqlite3 *, PackageMeta *);
static int tar_files(Plist *, PackageMeta *);
static int clean_up(const char *);

int create_pkg(Plist *plist, PackageMeta *pack)
{
  /* create a temp dir to hold our meta files. */
  char dirtmpl[] = "/tmp/mport.XXXXXXXX"; 
  char *tmpdir   = mkdtemp(dirtmpl);
  
  int ret = 0;
  sqlite3 *db;
  
  if (tmpdir == NULL) 
    return MPORT_ERR_FILEIO;
    
  if (chdir(tmpdir) != 0) 
    return MPORT_ERR_FILEIO;

  /* tmp */
  printf("Tmpdir: %s\n", tmpdir);
    
  ret += create_package_db(&db);
  ret += create_plist(db, plist);
  ret += create_meta(db, pack);
  
  /* done with the db */
  sqlite3_close(db);
  
  ret += tar_files(plist, pack);  
  ret += clean_up(tmpdir);
  
  return ret;    
}


static int create_package_db(sqlite3 **db) 
{
  if (sqlite3_open(PACKAGE_DB_FILENAME, db) != 0) {
    sqlite3_close(*db);
    return MPORT_ERR_SQLITE;
  }
  
  /* create tables */
  generate_plist_schema(*db);
  generate_package_schema(*db);
  
  return 0;
}

static int create_plist(sqlite3 *db, Plist *plist)
{
  PlistEntry *e;
  sqlite3_stmt *stmnt;
  const char *rest  = 0;
  int ret;
  char sql[]  = "INSERT INTO assets (pkg, type, data) VALUES (?,?,?)";
  
  if (sqlite3_prepare_v2(db, sql, -1, &stmnt, &rest) != SQLITE_OK) {
    fprintf(stderr, "SQL ERROR: %s\n", sqlite3_errmsg(db));
    exit(1);
  }
  
  
  STAILQ_FOREACH(e, plist, next) {
    if (sqlite3_bind_text(stmnt, 1, "not figured", -1, SQLITE_STATIC) != SQLITE_OK) {
      fprintf(stderr, "SQL ERROR: %s\n", sqlite3_errmsg(db));
      exit(1);
    }
    if (sqlite3_bind_int(stmnt, 2, e->type) != SQLITE_OK) {
      fprintf(stderr, "SQL ERROR: %s\n", sqlite3_errmsg(db));
      exit(1);
    }
    if (sqlite3_bind_text(stmnt, 3, e->data, -1, SQLITE_STATIC) != SQLITE_OK) {
      fprintf(stderr, "SQL ERROR: %s\n", sqlite3_errmsg(db));
      exit(1);
    }
    if ((ret = sqlite3_step(stmnt)) != SQLITE_DONE) {
      fprintf(stderr, "SQL ERROR: (%i) %s\n", sqlite3_errcode(db), sqlite3_errmsg(db));
      exit(1);
    }
        
    sqlite3_reset(stmnt);
  } 
}     

static int create_meta(sqlite3 *db, PackageMeta *pack)
{
}

static int tar_files(Plist *plist, PackageMeta *pack)
{
}

static int clean_up (const char *tmpdir)
{
}

