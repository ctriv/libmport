#include <archive.h>
#include <archive_entry.h>
#include "mport.h"



int mport_merge_primative(char **filenames)
{
  db = build_stub_db(filenames);
  tree = build_depends_tree(db);  
  
  archive_db(depends_tree, tmpdir);
  archive_package_files(depends_tree, filenames);
}

