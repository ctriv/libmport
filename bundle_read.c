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
 * $MidnightBSD: src/lib/libmport/bundle.c,v 1.2 2008/04/26 17:59:26 ctriv Exp $
 */


#include "mport.h"
#include "mport_private.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <archive_entry.h>

/*
 * mport_bundle_read_new()
 *
 * allocate a new read bundle struct.  Returns null if no memory could
 * be had.
 */
mportBundleRead * mport_bundle_read_new()
{
  return (mportBundleRead *)malloc(sizeof(mportBundleRead));
}


/*
 * mport_bundle_read_init(bundle, filename)
 *
 * connect the bundle struct to the file at filename.
 */
int mport_bundle_read_init(mportBundleRead *bundle, const char *filename)
{
  if ((bundle->filename = strdup(filename)) == NULL) 
    RETURN_ERROR(MPORT_ERR_NO_MEM, "Couldn't dup filename");
    
  if ((bundle->archive = archive_read_new()) == NULL)
    RETURN_ERROR(MPORT_ERR_NO_MEM, "Couldn't dup filename");
    
  
  bundle->firstreal = NULL;
  
  if (archive_read_open_filename(bundle->archive, bundle->filename, 10240) != ARCHIVE_OK) {
    RETURN_ERROR(MPORT_ERR_ARCHIVE, archive_error_string(bundle->archive));
  }
  
  return MPORT_OK;    
}


/*  
 * mport_bundle_read_finish(bundle)
 *
 * close the file connected to the bundle, and free any memory allocated.
 */
int mport_bundle_read_finish(mportBundleRead *bundle)
{
  int ret = MPORT_OK;
    
  if (archive_read_finish(bundle->archive) != ARCHIVE_OK)
    ret = SET_ERROR(MPORT_ERR_ARCHIVE, archive_error_string(bundle->archive));
          
  free(bundle->filename);
  free(bundle);
                  
  return ret;
}  
                    

/* 
 * mport_bundle_read_extract_metafiles(bundle, &dirnamep)
 *
 * creates a temporary directory containing all the meta files.  It is
 * expected that this will be called before next_entry() or next_file(),
 * terrible things might happen if you don't do this!
 *
 * The calling code should free the memory that dirnamep points to.
 */
int mport_bundle_read_extract_metafiles(mportBundleRead *bundle, char **dirnamep)
{
  /* extract the meta-files into the a temp dir */
  char filepath[FILENAME_MAX];
  const char *file;
  char dirtmpl[] = "/tmp/mport.XXXXXXXX";
  char *tmpdir = mkdtemp(dirtmpl);
  struct archive_entry *entry;
     
  if (tmpdir == NULL)
    RETURN_ERROR(MPORT_ERR_FILEIO, strerror(errno));
  
  if ((*dirnamep = strdup(tmpdir)) == NULL) 
    return MPORT_ERR_NO_MEM;
  
  while (1) {
    if (mport_bundle_read_next_entry(bundle, &entry) != MPORT_OK)
      RETURN_CURRENT_ERROR;     
 
    file = archive_entry_pathname(entry);
       
    if (*file == '+') {
      (void)snprintf(filepath, FILENAME_MAX, "%s/%s", tmpdir, file);
      archive_entry_set_pathname(entry, filepath);
      
      if (mport_bundle_read_extract_next_file(bundle, entry) != MPORT_OK)
        RETURN_CURRENT_ERROR;
        
    } else {
      /* entry points to the first real file in the bundle, so we 
       * want to hold on to that until next_entry() is called
       */
      bundle->firstreal = entry;
      break;
    }
  }
 
  return MPORT_OK;                 
} 


/*
 * mport_bundle_read_next_entry(bundle, &entry)
 *
 * sets entry to the next file entry in the bundle.
 */
int mport_bundle_read_next_entry(mportBundleRead *bundle, struct archive_entry **entryp)
{
  int ret;
  
  while (1) {
    ret = archive_read_next_header(bundle->archive, entryp);
    
    if (ret == ARCHIVE_RETRY) continue;

    if (ret == ARCHIVE_FATAL) 
      RETURN_ERROR(MPORT_ERR_ARCHIVE, archive_error_string(bundle->archive));

    /* ret was warn or OK, we're done */
    break;
  }
  
  return MPORT_OK;
}  


/*
 * mport_bundle_read_extract_next_file(bundle, entry)
 *
 * extract the next file int he bundle, based on the settings in entry.  
 * If you need to change things like perms or paths, you can do so by 
 * modifing the entry struct before you pass it to this function
 */
 
 /* XXX - should this be implemented as a macro? inline? */
int mport_bundle_read_extract_next_file(mportBundleRead *bundle, struct archive_entry *entry)
{
  if (archive_read_extract(bundle->archive, entry, 0) != ARCHIVE_OK) 
    RETURN_ERROR(MPORT_ERR_ARCHIVE, archive_error_string(bundle->archive));
  
  return MPORT_OK;
}