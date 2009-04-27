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
#include <string.h>

static int install_bundle_file(mportInstance *, const char *, const char *);
static int resolve_depends(mportInstance *, mportPackageMeta *);

MPORT_PUBLIC_API int mport_install(mportInstance *mport, const char *pkgname, const char *prefix)
{
  mportIndexEntry **e;
  char *filename;
  int ret = MPORT_OK;

  MPORT_CHECK_FOR_INDEX(mport, "mport_install()");
  
  if (mport_file_exists(pkgname)) 
    return install_bundle_file(mport, pkgname, prefix);
  
  if (mport_index_lookup_pkgname(mport, pkgname, &e) != MPORT_OK)
    RETURN_CURRENT_ERROR;

  /* we don't support installing more than one top-level package at a time.
   * Consider a situation like this:
   *
   * mport_install(mport, "p5-Class-DBI*");
   *
   * Say this matches p5-Class-DBI and p5-Class-DBI-AbstractSearch
   * and say the order from the index puts p5-Class-DBI-AbstractSearch 
   * first.
   * 
   * p5-Class-DBI-AbstractSearch is installed, and its depends installed.  
   * However, p5-Class-DBI is a depend of p5-Class-DBI-AbstractSearch, so 
   * when it comes time to install p5-Class-DBI, we can't - because it is
   * already installed.
   *
   * If a user facing application wants this functionality, it would be
   * easy to piece together with mport_index_lookup_pkgname(), a
   * check for already installed packages, and mport_install().
   */
  
  if (e[1] != NULL)
    RETURN_ERRORX(MPORT_ERR_AMBIGUOUS_ID, "Could not resolve '%s' to a single package.", pkgname);
  
  if (mport_fetch_bundle(mport, e[0]->bundlefile) != MPORT_OK)
      RETURN_CURRENT_ERROR;
    
  (void)asprintf(&filename, "%s/%s", MPORT_FETCH_STAGING_DIR, e[0]->bundlefile);
    
  if (filename == NULL) 
    return MPORT_ERR_NO_MEM; 
    
  ret = install_bundle_file(mport, filename, prefix);
  
  free(filename);
  
  mport_index_entry_free_vec(e);
  
  return ret;
}


static int install_bundle_file(mportInstance *mport, const char *filename, const char *prefix)
{
  mportBundleRead *bundle;
  mportPackageMeta **pkgs, *pkg;
  int i;
  
  if ((bundle = mport_bundle_read_new()) == NULL)
    return MPORT_ERR_NO_MEM;
  
  if (mport_bundle_read_init(bundle, filename) != MPORT_OK)
    RETURN_CURRENT_ERROR;

  if (mport_bundle_read_prep_for_install(mport, bundle) != MPORT_OK)
    RETURN_CURRENT_ERROR;
  
  if (mport_pkgmeta_read_stub(mport, &pkgs) != MPORT_OK)
    RETURN_CURRENT_ERROR;
  
  for (i=0; *(pkgs + i) != NULL; i++) {
    pkg = pkgs[i];
    
    if (prefix != NULL) {
      /* override the default prefix with the given prefix */
      free(pkg->prefix);
      if ((pkg->prefix = strdup(prefix)) == NULL) /* all hope is lost! bail */
        return MPORT_ERR_NO_MEM;
    }

    if (resolve_depends(mport, pkg) != MPORT_OK)
      RETURN_CURRENT_ERROR;

    if ((mport_check_preconditions(mport, pkg, MPORT_PRECHECK_INSTALLED|MPORT_PRECHECK_CONFLICTS) != MPORT_OK) 
                      || 
        (mport_bundle_read_install_pkg(mport, bundle, pkg) != MPORT_OK)) 
    {
      RETURN_CURRENT_ERROR;
    }
  }
  
  return MPORT_OK;
}
