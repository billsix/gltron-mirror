#include "filesystem/path.h"
#include "filesystem/dirsetup.h"

#include "Nebu_filesystem.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "base/nebu_debug_memory.h"

#ifndef PATH_MAX
// #warning PATH_MAX "is not defined in limits.h!"
#define PATH_MAX 255
#endif

static char preferences_dir[PATH_MAX];
static char snapshots_dir[PATH_MAX];
static char data_dir[PATH_MAX];
static char art_dir[PATH_MAX];
static char music_dir[PATH_MAX];
static char scripts_dir[PATH_MAX];
static char level_dir[PATH_MAX];

void initDirectories(void) {
  // PREF_DIR/SNAP_DIR are compile-time macros; the leading '~' means "under the
  // home directory", so &X[1] is the intentional skip of that '~'.
  if (PREF_DIR[0] != '~')
    snprintf(preferences_dir, sizeof preferences_dir, "%s", PREF_DIR);
  else
    snprintf(preferences_dir, sizeof preferences_dir, "%s%s", getHome(),
             &PREF_DIR[1]);

  if (SNAP_DIR[0] != '~')
    snprintf(snapshots_dir, sizeof snapshots_dir, "%s", SNAP_DIR);
  else
    snprintf(snapshots_dir, sizeof snapshots_dir, "%s%s", getHome(),
             &SNAP_DIR[1]);

  snprintf(data_dir, sizeof data_dir, "%s%c%s", DATA_DIR, SEPARATOR, "data");
  snprintf(art_dir, sizeof art_dir, "%s%c%s", DATA_DIR, SEPARATOR,
           "art/default");
  snprintf(scripts_dir, sizeof scripts_dir, "%s%c%s", DATA_DIR, SEPARATOR,
           "scripts");
  snprintf(music_dir, sizeof music_dir, "%s%c%s", DATA_DIR, SEPARATOR, "music");
  snprintf(level_dir, sizeof level_dir, "%s%c%s", DATA_DIR, SEPARATOR,
           "levels");

  /*
printf("directories:\n"
   "\tprefs: %s\n"
   "\tsnaps: %s\n"
   "\tdata:  %s\n"
   "\tart:   %s\n"
   "\tscripts:   %s\n"
   "\tmusic: %s\n",
   preferences_dir, snapshots_dir,
   data_dir, art_dir, scripts_dir,
   music_dir);
  */

  makeDirectory(preferences_dir);
  makeDirectory(snapshots_dir);
}

char* getPath(ePathLocation eLocation, const char* filename) {
  char* const path = getPossiblePath(eLocation, filename);
  if (path == nullptr) return nullptr;
  if (nebu_FS_Test(path)) return path;

  fprintf(stderr, "*** failed to locate file '%s' at '%s' (type %d)\n",
          filename, path, eLocation);
  assert(0);

  free(path);
  return nullptr;
}

// Builds a candidate path into a freshly allocated PATH_MAX buffer.
// Caller frees. Returns nullptr on allocation failure.
[[nodiscard]] char* getPossiblePath(ePathLocation eLocation,
                                    const char* filename) {
  char* const path = malloc(PATH_MAX);
  if (path == nullptr) return nullptr;
  snprintf(path, PATH_MAX, "%s%c%s", getDirectory(eLocation), SEPARATOR,
           filename);
  return path;
}

const char* getDirectory(ePathLocation eLocation) {
  switch (eLocation) {
    case PATH_PREFERENCES:
      return preferences_dir;
      break;
    case PATH_SNAPSHOTS:
      return snapshots_dir;
      break;
    case PATH_DATA:
      return data_dir;
      break;
    case PATH_SCRIPTS:
      return scripts_dir;
      break;
    case PATH_MUSIC:
      return music_dir;
      break;
    case PATH_ART:
      return art_dir;
      break;
    case PATH_LEVEL:
      return level_dir;
      break;
    default:
      fprintf(stderr, "invalid path type\n");
      assert(0);
  }
  return nullptr;
}
