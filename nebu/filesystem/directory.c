#include "base/nebu_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

nebu_List* readDirectoryContents(const char* dirname, const char* prefix) {
  struct dirent* entry;
  nebu_List *l, *p;

  l = (nebu_List*)malloc(sizeof(nebu_List));
  p = l;
  p->next = nullptr;

  DIR* const dir = opendir(dirname);
  if (dir == nullptr) {
    fprintf(stderr, "warning: cannot open directory '%s'\n", dirname);
    return l;
  }
  while ((entry = readdir(dir)) != nullptr) {
    if (prefix == nullptr || strstr(entry->d_name, prefix) == entry->d_name) {
      if (entry->d_name[0] != '.') {
        char* const name = malloc(strlen(entry->d_name) + 1);
        memcpy(name, entry->d_name, strlen(entry->d_name) + 1);
        p->data = name;
        p->next = (nebu_List*)malloc(sizeof(nebu_List));
        p = p->next;
        p->next = nullptr;
      }
    }
  }
  closedir(dir);
  return l;
}

void makeDirectory(const char* name) {
  int result;
  if (access(name, R_OK)) {
#ifndef WIN32
    result = mkdir(name, 0x1ff);
#else
    result = mkdir(name);
#endif
    if (result) printf("cannot create dir '%s': %s\n", name, strerror(errno));
  }
}
