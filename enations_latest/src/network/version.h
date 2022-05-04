

#define         VER_MAJOR       1
#define         VER_MINOR       0
#define         VER_RELEASE     5

#define         VER_STRING                              "1.00.005"
#define         RES_VER_STRING                          "1.00.005\0"

#ifdef _DEBUG
	#define         VER_FLAGS         VS_FF_DEBUG|VS_FF_PRIVATEBUILD|VS_FF_PRERELEASE
#else
	#define         VER_FLAGS         VS_FF_PRERELEASE // final version
#endif

