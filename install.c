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

static int install_bundle_file(mportInstance *, const char *);
static int resolve_depends(mportInstance *, mportPackageMeta *);

MPORT_PUBLIC_API int mport_install(mportInstance *mport, const char *pkgname)
{
  mportIndexEntry **e;
  char *filename;
  int ret = MPORT_OK, i;

  MPORT_CHECK_FOR_INDEX(mport, "mport_install()");
  
  if (mport_file_exists(pkgname)) 
    return install_bundle_file(pkgname);
  
  if (mport_index_lookup_pkgname(mport, pkgname, &e) != MPORT_OK)
    RETURN_CURRENT_ERROR;
  
  for (i = 0; e[i] != NULL; i++) {   
    if (mport_fetch_bundle(mport, bundlefile) != MPORT_OK)
      RETURN_CURRENT_ERROR;
    
    (void)asprintf(&filename, "%s/%s", MPORT_FETCH_STAGING_DIR, bundlefile);
    
    if (filename == NULL) {
      ret = MPORT_ERR_NO_MEM; 
      break;
    }
    
    ret = install_bundle_file(mport, filename);
  
    free(bundlefile);
  
    if (ret != MPORT_OK)
      break;
  }
  
  mport_free_index_entry_vec(e);
  
  return ret;
}


static int install_bundle_file(mportInstnace *mport, const char *filename)
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

    if ((mport_check_install_preconditions(mport, pkg) != MPORT_OK) 
                      || 
        (mport_bundle_read_install_pkg(mport, bundle, pkg) != MPORT_OK)) 
    {
      mport_call_msg_cb(mport, "Unable to install %s-%s: %s", pkg->name, pkg->version, mport_err_string());
      mport_set_err(MPORT_OK, NULL);
    }
  }
}