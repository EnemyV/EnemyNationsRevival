

#define         VER_MAJOR       1
#define         VER_MINOR       0
#define         VER_RELEASE     40

#define         VER_STRING                              "1.00.040"
#define         RES_VER_STRING                          "1.00.040\0"

#ifdef _DEBUG
	#define         VER_FLAGS         VS_FF_DEBUG | VS_FF_PRIVATEBUILD | VS_FF_PRERELEASE
#else
  #ifndef _FINAL
	  #define       VER_FLAGS         VS_FF_PRERELEASE // still testing
	#else
	  #define       VER_FLAGS         0
  #endif
#endif


