#include "filesystem/path.h"
#include "Nebu_scripting.h"

#include "base/nebu_debug_memory.h"

void runScript(int ePath, const char* name) {
  char* const s = getPath((ePathLocation)ePath, name);
  scripting_RunFile(s);
  free(s);
}
