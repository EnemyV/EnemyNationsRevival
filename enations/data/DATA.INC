//#define SHAREWARE 1           // shareware limits
#define NET_VER 1                     // small version
//#define SMALL_FULL 1          // small DAT file with no shareware limits
//#define SECOND_DISK 1         // second disk in retail SKU

#ifdef NET_VER
#define SHAREWARE 1
#endif

#ifdef SMALL_FULL
#define ALL_ZOOMS 1
#define TWO_VOICES 1
#define HAS_INTRO 1
#else


#ifdef SHAREWARE
#else
#define ALL_ZOOMS 1             // Set to 0 for .SPR files with only 3 zooms
#define INC24 1                 // include 24-bit art
#define INC_WAV 1               // include digital audio & 22K vfx/sfx
#define HAS_INTRO 1             // has the intro sequences
#define TWO_VOICES 1    // set to 0 if only 1 voice
#endif

#endif
