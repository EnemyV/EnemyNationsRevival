
// $Header:   T:\vcs\nwadmin\inc\nwsnapin.h_v   1.18   17 Aug 1994 15:51:42   VINOD  $
#ifndef __NWSNAPIN_H
#define __NWSNAPIN_H

#if !defined( NTYPES_H )
#define N_PLAT_WIN16
#include <ntypes.h>
#endif

#include <nwdsdefs.h>

#ifdef __cplusplus
extern "C"
{
#endif


N_TYPEDEF_CALLBACK_PAS( NWRCODE, NWASnapinObjectProc )
(
  pnstr8 objName, 
  nuint16 message, 
  nparam param1, 
  nparam param2
);

N_TYPEDEF_CALLBACK_PAS( void, NWASnapinMenuActionProc )
(void);

N_TYPEDEF_CALLBACK_PAS( void, NWASnapinMenuValidProc )
(pnuint16);

N_TYPEDEF_CALLBACK_PAS( nuint32, NWAFlatBrowserProc )
(
  nuint32 userParam,
  nuint16 uMsg,
  nparam  param1,
  nparam  param2 
);

N_TYPEDEF_CALLBACK_PAS( nuint32, NWAMVEDProc )
(
  nuint32 userParam,
  nuint16 uMsg,
  nparam  param1,
  nparam  param2
);

/*------------------------------------------------------------*/
/* SNAPIN Version related defines                             */
/*------------------------------------------------------------*/
#define    NWA_SNAPIN_VER_MAJOR     1
#define    NWA_SNAPIN_VER_MINOR     0
#define    NWA_SNAPIN_VER_REVISION  0
#define    NWA_SNAPIN_VERSION       NMake32( NMake16(NWA_SNAPIN_VER_REVISION, 0 )  , NMake16(NWA_SNAPIN_VER_MINOR, NWA_SNAPIN_VER_MAJOR ) )

/*------------------------------------------------------------*/
/* View Names                                                 */
/*------------------------------------------------------------*/
#define    NWA_VIEW_CONSOLE        "ListMenu"
#define    NWA_VIEW_BROWSER        "BROWSER"

/*--------------------------------------------*/
/*   messages from nwadmin to object snapins  */
/*--------------------------------------------*/
#define    NWA_MSG_INITSNAPIN              0x0100
#define    NWA_MSG_CLOSESNAPIN             0x0101
#define    NWA_MSG_CREATEOBJECT            0x0102
#define    NWA_MSG_GETPAGECOUNT            0x0103
#define    NWA_MSG_REGISTERPAGE            0x0104
#define    NWA_MSG_RENAME                  0x0105
#define    NWA_MSG_MODIFY                  0x0106
#define    NWA_MSG_QUERYDELETE             0x0107
#define    NWA_MSG_QUERYCOPY               0x0108
#define    NWA_MSG_QUERYMOVE               0x0109
#define    NWA_MSG_GETVALIDOPERATIONS      0x0110
#define    NWA_MSG_MPEWCLOSE               0x0111

/*----------------------------------------------*/
/*    message from snapin page to its parent    */
/*----------------------------------------------*/
#define    NWA_WM_SETPAGEMODIFY     WM_USER+0x120

#define    NWA_WM_CANCLOSE          WM_USER+0x121

/*--------------------------------------------------------------*/
/* F1 help message to all snapin dialogs                        */
/* MPEW also sends the same message when Help button is pressed */
/*--------------------------------------------------------------*/
#define  NWA_WM_F1HELP              (WM_USER + 0x139)

#define  NWA_MENU_BASE               0x1000

/*--------------------------------------------------------------*/
/* Messages passed to Flat Browser Proc                         */
/*--------------------------------------------------------------*/
#define  NWA_MSG_FBOBJECT_COUNT  0x0200
#define  NWA_MSG_FBOBJECT_VALUE  0x0201
#define  NWA_MSG_FBFILTER_COUNT  0x0202
#define  NWA_MSG_FBFILTER_VALUE  0x0203

/*----------------------------------------------------------------*/
/* Flags or-ed to specify options for Flat Browser                */
/*----------------------------------------------------------------*/
#define NWA_FB_BROWSE_PUBLIC     0x00000001L
#define NWA_FB_BROWSE_ROOT       0x00000002L
#define NWA_FB_SINGLE_SELECT     0x00000004L
#define NWA_FB_MULTIPLE_SELECT   0x00000008L
#define NWA_FB_NONDS_VOLUMES     0x00000010L
#define NWA_FB_EXPAND_VOLUMES    0x00000020L


/*----------------------------------------------------------------*/
/* edit Types for MVED Control                                    */
/*----------------------------------------------------------------*/
#define NWA_MVED_VALUE_READONLY    0x0001
#define NWA_MVED_VALUE_STRING      0x0001
#define NWA_MVED_VALUE_DISTNAME    0x0002
#define NWA_MVED_VALUE_DIGITSONLY  0x0004
#define NWA_MVED_VALUE_PRINTABLE   0x0005

/*----------------------------------------------------------------*/
/* Message passed to MVED callback proc                           */
/*----------------------------------------------------------------*/
#define NWA_MSG_MVED_INITCOUNT   0x300
#define NWA_MSG_MVED_INITVALUE   0x301




/*----------------------------------------------------------*/
/* return values from snapins to admin and admin to snapin  */
/*----------------------------------------------------------*/
#define    NWA_RET_SUCCESS                     0
#define    NWA_RET_DODEFAULT                   1
#define    NWA_RET_SHOWDETAILS                 2
#define    NWA_RET_CREATEANOTHER               3

#define    NWA_ERR_ERROR                                               -1
#define    NWA_ERR_NO_MEMORY                                         -901
#define    NWA_ERR_DUPLICATE_ENTRY                                   -902
#define    NWA_ERR_UNKNOWN_ERROR                                     -903
#define    NWA_ERR_INVALID_STRING_PARAMETER                          -904
#define    NWA_ERR_INVALID_BITMAP_PARAMETER                          -905
#define    NWA_ERR_INVALID_BITMAP_POINTER_PARAMETER                  -906
#define    NWA_ERR_INSUFFICIENT_STRING_BUFFER                        -907
#define    NWA_ERR_CLASSNAME_TABLE_NOT_INITIALIZED                   -908
#define    NWA_ERR_ATTRNAME_TABLE_NOT_INITIALIZED                    -909
#define    NWA_ERR_BITMAP_TABLE_NOT_INITIALIZED                      -910
#define    NWA_ERR_ALIAS_BITMAP_TABLE_NOT_INITIALIZED                -911
#define    NWA_ERR_CLASSNAME_NOT_FOUND                               -912
#define    NWA_ERR_ATTRNAME_NOT_FOUND                                -913
#define    NWA_ERR_BITMAP_NOT_FOUND                                  -914
#define    NWA_ERR_IS_BAD_STRING_POINTER                             -915
#define    NWA_ERR_INVALID_PARAMETER                                 -916
#define    NWA_ERR_LAUNCH_FBROWSER                                   -917
#define    NWA_ERR_FUNCTION_FAILED                                   -918
#define    NWA_ERR_INVALID_CALLBACK_PROC                             -919
#define    NWA_ERR_INVALID_MODULE_HANDLE                             -920
#define    NWA_ERR_INVALID_SNAPIN_VERSION                            -921


/*------------------------------------------------------------*/
/* snapins should '|' operations they support and             */
/* return in response to NWAD_GETVALIDOPERATIONS              */
/*------------------------------------------------------------*/
#define NWA_OP_DSTYPE          0x00000001L
#define NWA_OP_FSTYPE          0x00000002L
#define NWA_OP_CREATE          0x00000004L
#define NWA_OP_DETAILS         0x00000008L
#define NWA_OP_MOVE            0x00000010L
#define NWA_OP_COPY            0x00000020L
#define NWA_OP_RENAME          0x00000040L
#define NWA_OP_DELETE          0x00000080L
#define NWA_OP_USERTEMPLATE    0x00000100L
#define NWA_OP_SEARCH          0x00000200L
#define NWA_OP_CONTAINER       0x00000400L

typedef struct tagNWAPageStruct
{
   DLGPROC    dlgProc;     /* dialog procedure */
   HINSTANCE  hDLL;        /* resource DLL handle */
   LPSTR      resName;     /* resource name */
   LPSTR      pageTitle;   /* title of the page in the listbox */
   LPARAM     initParam;   /* initialization parameters */
} NWAPageStruct;

#define NWA_MAX_TRANSLATED_NAME  256
#define NWA_MAX_CLASS_NAME       MAX_SCHEMA_NAME_BYTES
#define NWA_MAX_CLASS_TYPE       MAX_SCHEMA_NAME_BYTES
#define NWA_MAX_OBJECT_NAME      MAX_DN_BYTES
#define NWA_DS_OBJECT_TYPE       "DS"
#define NWA_FS_OBJECT_TYPE       "FS"

typedef struct tagNWASelObject
{
   char    objType[NWA_MAX_CLASS_TYPE];   /* "DS", "FS" or some user defined*/
   char    objClass[NWA_MAX_CLASS_NAME];  /* object class name */
   char    objName[NWA_MAX_OBJECT_NAME];  /* complete name or path */
} NWASelObject, N_FAR *PNWASelObject;

/* prototypes */

int  FAR PASCAL InitSnapin( void );
void FAR PASCAL ShutDown( void );

N_EXTERN_LIBRARY( NWRCODE )
NWAGetSelObjectCount
(
   pnint32 lpCount
);

N_EXTERN_LIBRARY( NWRCODE )
NWAGetSelObject
(
  nint32 index,
  PNWASelObject selObject
);

N_EXTERN_LIBRARY( NWRCODE )
NWARegisterObjectProc

(
   pnstr8              classType,
   pnstr8              className,
   pnstr8              developerInfo,
   HINSTANCE           hDLL,
   NWASnapinObjectProc proc,
   nuint32             version
);

N_EXTERN_LIBRARY( NWRCODE )
NWARegisterMenu
(
   pnstr8                  viewName,
   nuint16                 menuParentId,
   pnstr8                  menuParent,
   nuint16                 menuOption,
   pnuint16                pmenuID,
   pnstr8                  menuString,
   pnstr8                  menuHint,
   NWASnapinMenuActionProc menuActionProc, 
   NWASnapinMenuValidProc  menuValidProc,
   nuint32                 version
);

/*------------------------------------------------------------------*/
/*             translation and bitmap table functions               */
/*------------------------------------------------------------------*/
N_EXTERN_LIBRARY( NWRCODE )
NWAAddClassData
(
   LPCSTR  pszClassName,
   LPCSTR  pszTranslation,
   HBITMAP hClassBitmap,
   HBITMAP hClassAliasBitmap,
   HBITMAP hClassROBitmap
);

N_EXTERN_LIBRARY( NWRCODE )
NWARemoveClassData
(
   LPCSTR   pszClassName,
   HBITMAP *phClassBitmap,
   HBITMAP *phClassAliasBitmap,
   HBITMAP *phClassROBitmap
);

N_EXTERN_LIBRARY( NWRCODE )
NWAGetTranslatedClassName
(
   LPCSTR  pszClassName,
   pnstr8  pszTranslation,
   nint16  nMaxLen
);

N_EXTERN_LIBRARY( NWRCODE )
NWAGetClassBitmap
(
   LPCSTR   pszClassName,
   HBITMAP *phBitmap
);

N_EXTERN_LIBRARY( NWRCODE )
NWAGetClassAliasBitmap
(
   LPCSTR   pszClassName,
   HBITMAP *phBitmap
);

N_EXTERN_LIBRARY( NWRCODE )
NWAGetClassReadOnlyBitmap
(
   LPCSTR   pszClassName,
   HBITMAP *phBitmap
);

N_EXTERN_LIBRARY( NWRCODE )
NWAAddPropertyNameTranslation
(
   LPCSTR pszPropertyName,
   LPCSTR pszClassName,
   LPCSTR pszTranslation
);

N_EXTERN_LIBRARY( NWRCODE )
NWAGetTranslatedPropertyName
(
   LPCSTR  pszPropertyName,
   LPCSTR  pszClassName,
   pnstr8  pszTranslation,
   nint16  nMaxLen
);

N_EXTERN_LIBRARY( NWRCODE )
NWARemovePropertyNameTranslation
(
   LPCSTR pszPropertyName,
   LPCSTR pszClassName
);

/*------------------------------------------------------------------*/
/*         Launch DS and FS flat browser functions                  */
/*------------------------------------------------------------------*/
N_EXTERN_LIBRARY( NWRCODE )
NWALaunchDSFlatBrowser
(
   HWND               hwndParent,
   nuint32            userParam,
   LPCSTR             contextStr,
   LPCSTR             selObjectStr,
   LPCSTR             navObjectStr,
   LPCSTR             selObjectLabel,
   nuint32            brwsFlags,
   NWAFlatBrowserProc pfn
);

N_EXTERN_LIBRARY( NWRCODE )
NWALaunchFSFlatBrowser
(
  HWND               hwndParent,
  nuint32            userParam,
  LPCSTR             dsContextStr,
  LPCSTR             fsContextStr,
  LPCSTR             selObjectStr,
  LPCSTR             navObjectStr,
  LPCSTR             selObjectLabel,
  nuint32            brwsFlags,
  NWAFlatBrowserProc pfn
);

/*------------------------------------------------------------------*/
/*  Launch MVED functions                                           */
/*------------------------------------------------------------------*/
N_EXTERN_LIBRARY( NWRCODE )
NWACreateMved
(
  HWND        hwndParent,
  nuint32     userParam,
  nuint16     idEdit,
  nuint16     idSpin,
  nuint16     idButton,
  nuint16     editType,
  nuint16     lengthEdit,
  LPCSTR      label,
  LPCSTR      contextStr,
  nuint32     brwsFlags,
  NWAMVEDProc pfn
);

N_EXTERN_LIBRARY( NWRCODE )
NWAGetMvedCount
(
  HWND         hwndParent, 
  nuint32      idEdit,
  pnuint16     pCount
);

N_EXTERN_LIBRARY( NWRCODE )
NWAGetMvedValue
(
  HWND         hwndParent,
  nuint16      idEdit,
  nuint16      itemIndex,
  nuint16      bufSize,
  pnstr        pValue
);

/*------------------------------------------------------------------*/
/*    NWAdmin and Snapin Interface Version Functions                */
/*------------------------------------------------------------------*/

N_EXTERN_LIBRARY( NWRCODE )
NWAGetNWAdminVersion
(
  pnuint32 pVersion
);

N_EXTERN_LIBRARY( HWND )
NWAGetConsoleWindowHandle
(
  void
);

#ifdef __cplusplus
}
#endif

#endif

