#ifndef RBUILD_H
#define RBUILD_H

#include "sptypes.h"

int CountLeafDirs( const char *pBaseDir );
bool RecursiveBuild( TSpriteType type, const char *pBaseDir, BOOL bShareware = false );

#endif
