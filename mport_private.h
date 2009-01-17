/* $MidnightBSD: src/lib/libmport/mport.h,v 1.10 2008/04/26 17:59:26 ctriv Exp $
 *
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
 */

#ifndef _MPORT_PRIV_H_
#define _MPORT_PRIV_H_

/* callback syntaxtic sugar */
void mport_call_msg_cb(mportInstance *, const char *, ...);

/* precondition checking */
int mport_check_update_preconditions(mportInstance *, mportPackageMeta *);
int mport_check_install_preconditions(mportInstance *, mportPackageMeta *);

/* schema */
int mport_generate_master_schema(sqlite3 *);
int mport_generate_stub_schema(sqlite3 *);

/* Various database convience functions */
int mport_attach_stub_db(sqlite3 *, const char *);
int mport_detach_stub_db(sqlite3 *);
int mport_get_meta_from_stub(sqlite3 *, mportPackageMeta ***);
int mport_db_do(sqlite3 *, const char *, ...);
int mport_db_prepare(sqlite3 *, sqlite3_stmt **, const char *, ...);


/* Utils */
int mport_copy_file(const char *, const char *);
int mport_rmtree(const char *);
int mport_mkdir(const char *);
int mport_rmdir(const char *, int);
int mport_chdir(mportInstance *, const char *);
int mport_file_exists(const char *);
int mport_xsystem(mportInstance *mport, const char *, ...);
int mport_run_plist_exec(mportInstance *mport, const char *, const char *, const char *);


/* sqlite version compare function */
void mport_version_cmp_sqlite(sqlite3_context *, int, sqlite3_value **);


#define RETURN_CURRENT_ERROR return mport_err_code()
#define RETURN_ERROR(code, msg) return mport_set_errx((code), "Error at %s:(%d): %s", __FILE__, __LINE__, (msg))
#define SET_ERROR(code, msg) mport_set_errx((code), "Error at %s:(%d): %s", __FILE__, __LINE__, (msg))
#define RETURN_ERRORX(code, fmt, ...) return mport_set_errx((code), "Error at %s:(%d): " fmt, __FILE__, __LINE__, __VA_ARGS__)
#define SET_ERRORX(code, fmt, ...) mport_set_errx((code), "Error at %s:(%d): " fmt, __FILE__, __LINE__, __VA_ARGS__)
int mport_set_err(int, const char *);
int mport_set_errx(int , const char *, ...);


/* Infrastructure files */
#define MPORT_STUB_DB_FILE 	"+CONTENTS.db"
#define MPORT_STUB_INFRA_DIR	"+INFRASTRUCTURE"
#define MPORT_MTREE_FILE   	"mtree"
#define MPORT_INSTALL_FILE 	"pkg-install"
#define MPORT_DEINSTALL_FILE	"pkg-deinstall"
#define MPORT_MESSAGE_FILE	"pkg-message"


/* Instance files */
#define MPORT_INST_DIR 		"/var/db/mport"
#define MPORT_MASTER_DB_FILE	"/var/db/mport/master.db"
#define MPORT_INST_INFRA_DIR	"/var/db/mport/infrastructure"

/* Binaries we use */
#define MPORT_MTREE_BIN		"/usr/sbin/mtree"
#define MPORT_SH_BIN		"/bin/sh"
#define MPORT_CHROOT_BIN	"/usr/sbin/chroot"


#endif /* _MPORT_PRIV_H_ */