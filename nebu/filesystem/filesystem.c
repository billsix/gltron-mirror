#include "filesystem/nebu_filesystem.h"
#include "base/nebu_util.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "base/nebu_debug_memory.h"

int nebu_FS_Test(const char* path) {
  FILE* const f = fopen(path, "r");
  if (f) {
    fclose(f);
    return 1;
  }
  return 0;
}

nebu_List* path_list = nullptr;

void nebu_FS_ClearAllPaths(void) {
  nebu_List* p;
  if (!path_list) return;

  for (p = path_list; p->next; p = p->next) {
    free(p->data);
  }
  nebu_List_Free(path_list);
}

void nebu_FS_SetupPath(int tag, int nDirs, const char** directories) {
  nebu_List* p;

  if (path_list == nullptr) {
    path_list = (nebu_List*)malloc(sizeof(nebu_List));
    path_list->next = nullptr;
  }

  for (p = path_list; p->next != nullptr; p = p->next) {
    nebu_FS_PathConfig* const pConfig = (nebu_FS_PathConfig*)p->data;
    if (pConfig->tag == tag) {
      // replace directories
      // TODO: consider memory management
      pConfig->nDirs = nDirs;
      pConfig->directories = directories;
      return;
    }
  }
  p->next = malloc(sizeof(nebu_List));
  p->next->next = nullptr;
  p->data = malloc(sizeof(nebu_FS_PathConfig));
  // append directories
  {
    nebu_FS_PathConfig* const pConfig = (nebu_FS_PathConfig*)p->data;
    pConfig->tag = tag;
    pConfig->nDirs = nDirs;
    pConfig->directories = directories;
  }
}

char* nebu_FS_GetPath(int tag, const char* filename) {
  nebu_List* p;
  for (p = path_list; p->next != nullptr; p = p->next) {
    const nebu_FS_PathConfig* const pConfig = (nebu_FS_PathConfig*)p->data;
    if (pConfig->tag == tag) {
      int i;
      for (i = 0; i < pConfig->nDirs; i++) {
        int length = (int)(strlen(pConfig->directories[i]) + 1 /* seperator */ +
                           strlen(filename) + 1 /* terminator */);
        char* const path = (char*)malloc((size_t)length);
        sprintf(path, "%s%c%s", pConfig->directories[i], PATH_SEPARATOR,
                filename);
        printf("trying %s\n", path);
        if (nebu_FS_Test(path))
          return path;
        else
          free(path);
      }
    }
  }
  return nullptr;
}
