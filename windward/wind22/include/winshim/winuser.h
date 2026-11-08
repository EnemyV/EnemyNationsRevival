// Linux-only wrapper: redirect a Windows SDK umbrella header to the shim.
// This dir is on the include path for the Linux build only (see root CMakeLists).
#include "win32_compat.h"
