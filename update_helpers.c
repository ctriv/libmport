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
 * $MidnightBSD: src/lib/libmport/delete_primative.c,v 1.2 2008/04/26 17:59:26 ctriv Exp $
 */



#include "mport.h"
#include "mport_private.h"

/* very low level code.  This function is called by mport_install_primative() when
 * the package to be installed is already installed.  By the point that this function
 * is called, the bundle has already been opened, and the metafiles extracted to a 
 * tmp directory.  
 *
 * This means there is a high level of interdependence between this function
 * and mport_install_primative.  The author acknowledges that this is less
 * than diserable, but given that a bundle may have some packaes that need
 * to be installed and have others that need to updated; such tight coupling
 * is unavoidable.
 */

/* mport_update_protoprimative(mportInstance *, mportBundleRead *, mportPackageMeta *, const char *)
 * 
 * handle updating a package, once the newer version has already been
 * prepped by the install primative.
 */

int mport_update_primative(mportInstance *mport, mportBundleRead *bundle, mportPackageMeta *pkg, const char *filename) 
{
  mportAssetList *alist;
  
  if (mport_check_update_preconditions(mport, pkg) != MPORT_OK)
    RETURN_CURRENT_ERROR;
   
  if (mport_get_assets_from_master(mport, pkg, &alist) != MPORT_OK)
    RETURN_CURRENT_ERROR;
  
  if (create_backup_package(mport, pkg, alist) != MPORT_OK)
    RETURN_CURRENT_ERROR;
  
  if (mport_delete_primative(mport, pkg) != MPORT_OK) {
    install_backup_package(mport, pkg, alist);
    RETURN_CURRENT_ERROR;
  }
  
  /* at this point we almost want to hand control back to install.... */
  
    
     
  
    
     
  
} 
      
  
