#ifndef __NWLOCALE_H__
/*----------------------------------------------------------------------------*
 *                                                                            *
 *      This include file defines the constants, types, structures,           *
 *      and prototypes for the Novell Enabling APIs.                          *
 *                                                                            *
 *      (c) Copyright. 1989-1991 Novell, Inc.  All rights reserved.           *
 *                                                                            *
 *----------------------------------------------------------------------------*/

#ifndef NWAPI
# define NWAPI far pascal
#endif

#if defined(MEDIUM_MODEL)
# undef NWFAR
# define NWFAR
#endif

#ifndef NWFAR
# define NWFAR far
#endif

#ifndef NWCCODE
#define NWCCODE           unsigned int
#endif

#define NUMBER_TYPE long double

#if !defined (_SIZE_T_DEFINED) && !defined (_SIZE_T) && !defined (_SIZE_T_DEFINED_)
typedef unsigned int size_t;
# define _SIZE_T
#endif

/* (in imitation of stdlib.h) */

#define L_MB_LEN_MAX       2   /* (in imitation of limits.h) */
#define MERIDLEN           5
#define NWSINGLE_BYTE      1
#define NWDOUBLE_BYTE      2

#define _UCHAR             (unsigned char)

#define LC_ALL             0
#define LC_COLLATE         1
#define LC_CTYPE           2
#define LC_MONETARY        3
#define LC_NUMERIC         4
#define LC_TIME            5


/*====================================================================
   country definitions
====================================================================*/
#define ARABIC            785
#define WBAHRAIN          973
#define WCYPRUS           357 /* ??? */
#define WEGYPT             20
#define WETHIOPIA         251
#define WIRAN              98
#define WIRAQ             964
#define WJORDAN           962
#define WKUWAIT           965         
#define WLIBYA            218
#define WMALTA            356 /* ??? */
#define WMOROCCO          212 /* SHOULD THIS BE FRENCH?? */
#define WPAKISTAN          92
#define WQATAR            974 /* ??? */
#define WSAUDI            966
#define WTANZANIA         255 /* ??? */
#define WTUNISIA          216 /* ??? */
#define WTURKEY            90 /* ??? */
#define WUAE              971
#define WYEMEN            967 /* ??? */
#define AUSTRALIA          61
#define BELGIUM            32
/*#define CANADA_ENG          1*/
#define CANADA_FR           2
#define CANADA              2
#define DENMARK            45
#define FINLAND           358
#define FRANCE             33
#define GERMANY            49
#define GERMANYE           37
#define HEBREW            972
#define ITALY              39
#define LATIN_AMERICA       3
#define WARGENTINA         54
#define WBOLIVIA          591
#define WCHILE             56
#define WCOLOMBIA          57
#define WCOSTARICA        506
#define WECUADOR          593
#define WELSALVADOR       503
#define WGUATEMALA        502
#define WHONDURAS         504
#define WMEXICO            52
#define WNICARAGUA        505
#define WPANAMA           507
#define WPARAGUAY         595
#define WPERU              51
#define WURUGUAY          598
#define WVENEZUELA         58
#define NETHERLANDS        31
#define NORWAY             47
#define PORTUGAL          351
#define SPAIN              34
#define SWEDEN             46
#define SWITZERLAND        41
#define UK                 44
#define USA                 1
#define JAPAN              81
#define KOREA              82
#define PRC                86
#define TAIWAN             886	/* This one for DOS */
#define WTAIWAN				886	/* This one for Windows */
#define ASIAN_ENGLISH      99

/*====================================================================
   typedef Llconv
====================================================================*/
typedef struct Llconv
   {
   char decimal_point[4];     /* non-monetary decimal point */
   char thousands_sep[4];     /* non-monetary separator for digits
                                   left of the decimal-point */
   char grouping[4];          /* String indicating size of groups
                                   of digits*/
   /*
    The international currency symbol applicable to
    the current locale.  The first three characters
    contain the alphabetic international currency
    symbol in accordance with those specified in ISO
    4217 "codes for the representation of currency
    and funds." The fourth character is the character
    used to separate the international currency
    symbol from the monetary quantity.
   */
   char int_curr_symbol[8];
   char currency_symbol[4];   /* Currency symbol for current locale */
   char mon_decimal_point[4]; /* monetary decimal point */
   char mon_thousands_sep[4]; /* monetary separator for digits left
                                   of the decimal-point */
   char mon_grouping[8];      /* String indicating size of
                                   groups of digits */
   char positive_sign[4];     /* String indicating positive
                                   monetary value */
   char negative_sign[4];     /* String indicating negative
                                   monetary value */
   char int_frac_digits;      /* Num of fractional digits in
                                   monetary display */
   char frac_digits;          /* Num of fractional digits in
                                   non-monetary display*/
   char p_cs_precedes;        /* 1=precede or 0=succeeds
                                   (pos currency symbol)*/
   char p_sep_by_space;       /* 1=space separator or
                                 0=no space separator
                                   (currency symbol) */
   char n_cs_precedes;        /* location of currency_symbol
                                   for neg monetary quantity */
   char n_sep_by_space;       /* separation of currency symbol
                                   in neg monetary quantity */
   char p_sign_posn;          /* value indicating position of
                                   positive_sign for positive
                                   monetary quantity */
   char n_sign_posn;          /* value indicating position of
                                   negative_sign for negative
                                   monetary quantity.*/

   /* Novell Additions to the ANSI definition:*/
   int  code_page;
   int  country_id;
   char data_list_separator[2];
   char date_separator[2];
   char time_separator[2];
   char time_format;
   int date_format;
	char am[MERIDLEN];
	char pm[MERIDLEN];
   char reserved[40];
} LCONV;

/*====================================================================
   *function prototypes*
====================================================================*/
#if defined(__cplusplus)
extern "C" {
#endif

LCONV NWFAR * NWAPI NWLlocaleconv(
         LCONV NWFAR *lconvPtr);

int NWAPI NWLmblen(
         char NWFAR *string,
         size_t maxBytes);

char  NWFAR * NWAPI NWLsetlocale(
         int category,
         const char NWFAR *locale);

char  NWFAR * NWAPI NWLstrchr(
         char NWFAR *string,
         int find);

/* NWLstrcoll  (see below) */

size_t NWAPI NWLstrcspn(
         const char NWFAR *string1,
         const char NWFAR *string2);

#ifndef NWL_EXCLUDE_TIME
size_t NWAPI NWLstrftime(
         char NWFAR *string,
         size_t maxSize,
         char NWFAR *format,
         struct tm NWFAR *timePtr);
#endif

char  NWFAR * NWAPI NWLstrpbrk(
         char NWFAR *string1,
         const char NWFAR *string2);

char  NWFAR * NWAPI NWLstrrchr(
         char NWFAR *string,
         int find);

char  NWFAR * NWAPI NWLstrrev(
         char NWFAR *string1,
         char NWFAR *string2);

size_t NWAPI NWLstrspn(
         const char NWFAR *string1,
         const char NWFAR *string2);

char  NWFAR * NWAPI NWLstrstr(
         char NWFAR *string,
         char NWFAR *searchString);

/* NWLstrupr ( see below )*/

char  NWFAR * NWAPI NWIncrement(
         char NWFAR *string,
         size_t numChars);

char  NWFAR * NWAPI NWstrImoney(
         char NWFAR *buffer,
         NUMBER_TYPE Value);

char  NWFAR * NWAPI NWstrmoney(
         char NWFAR *buffer,
         NUMBER_TYPE Value);

int   NWAPI NWstrncoll(
         char NWFAR *string1,
         char NWFAR *string2,
         size_t maxBytes);

char   NWFAR * NWAPI NWstrncpy(
         char NWFAR *target_string,
         char NWFAR *source_string,
         int numChars);

char   NWFAR * NWAPI NWLstrbcpy(
         char NWFAR *target_string,
         char NWFAR *source_string,
         int numBytes);

char  NWFAR * NWAPI NWstrnum(
         char NWFAR *buffer,
         NUMBER_TYPE Value);

int far cdecl NWprintf(
         char NWFAR *format,
         ...);

#ifndef NWL_EXCLUDE_FILE
int far cdecl NWfprintf(
         FILE NWFAR *stream,
         char NWFAR *format,
         ...);
#endif

int far cdecl NWsprintf(
         char NWFAR *buffer,
         char NWFAR *format,
         ...);

int NWAPI NWvprintf(
         char NWFAR *format,
         char NWFAR *arglist);

#ifndef NWL_EXCLUDE_FILE
int NWAPI NWvfprintf(
         FILE NWFAR *stream,
         char NWFAR *format,
         char NWFAR *arglist);
#endif

int NWAPI NWvsprintf(
         char NWFAR *buffer,
         char NWFAR *format,
         char NWFAR *arglist);

int far cdecl NWwsprintf(
         char NWFAR *buffer,
         char NWFAR *format,
         ...);

int NWAPI NWatoi(
         char NWFAR *string);

int NWAPI NWisalpha(
         unsigned int ch);

int NWAPI NWisalnum(
         unsigned int ch);

int NWAPI NWisdigit(
         unsigned int ch);

void NWAPI NWGetNWLOCALEVersion(unsigned char NWFAR *majorVersion,
                                unsigned char NWFAR *minorVersion,
                                unsigned char NWFAR *revisionLevel,
                                unsigned char NWFAR *betaReleaseLevel);

NWCCODE NWAPI NWGetShortMachineName(char NWFAR *shortMachineName);

/* This call is not needed for Windows */
int NWAPI NWGetCollateTable ( char NWFAR *retCollateTable,
                               size_t maxLen );


/*====================================================================
   macros
====================================================================*/

#if defined (NWWIN)
#define NWNextChar(s)         AnsiNext(s)
#define NWPrevChar(t, s)      AnsiPrev(t, s)
#define NWLstrupr(s)          AnsiUpper(s)
#define NWLstrcoll(s1, s2)    lstrcmp(s1, s2)
#define NWLstrxfrm(s1, s2, t) strxfrm(s1, s2, t)

/* AnsiUpper handles double byte characters. */

#define NWCharUpr(c) \
   (int) ( LOWORD ( (DWORD) AnsiUpper ( (LPSTR) (DWORD) c ) ) )

#else

size_t NWAPI NWLstrxfrm(
         char NWFAR *string1,
         char NWFAR *string2,
         size_t numBytes);

char  NWFAR * NWAPI NWNextChar(
         char NWFAR *string);

char  NWFAR * NWAPI NWPrevChar(
         const char NWFAR *string,
         char NWFAR *position);

char  NWFAR * NWAPI NWLstrupr ( char NWFAR *string );
int  NWAPI NWCharUpr ( int chr );
int  NWAPI  NWLstrcoll( char NWFAR *string1, char NWFAR *string2);
#endif

int  NWAPI NWCharType(unsigned char ch);
int  NWAPI NWCharVal(char NWFAR *);

#if defined(__cplusplus)
}
#endif

#define __NWLOCALE_H__
#endif
