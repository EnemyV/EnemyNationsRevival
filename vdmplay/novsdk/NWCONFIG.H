/*--------------------------------------------------------------------------
     (C) Copyright Novell, Inc. 1991  All Rights Reserved.

 No part of this file may be duplicated, revised, translated, localized or
 modified in any manner or compiled, linked, or uploaded or downloaded to or
 from any computer system without the prior written consent of Novell, Inc.
--------------------------------------------------------------------------*/
#define T_OPTIONAL    0x80

#define T_NUMBER      0x01
#define T_INDEX       0x02
#define T_STRING      0x03
#define T_HEX_STRING  0x04
#define T_HEX_NUMBER  0x05
#define T_LONG_NUMBER 0x06
#define T_LONG_HEX    0x07

#define T_SET_1       0x10
#define T_SET_2       0x11
#define T_SET_3       0x12
#define T_SET_4       0x13
#define T_SET_5       0x14
#define T_SET_6       0x15
#define T_SET_7       0x16
#define T_SET_8       0x17
#define T_SET_9       0x18
#define T_SET_10      0x19
#define T_SET_11      0x1A
#define T_SET_12      0x1B
#define T_SET_13      0x1C
#define T_SET_14      0x1D
#define T_SET_15      0x1E
#define T_SET_16      0x1F

#define MAX_PARAMETERS        8
#define MAX_SECTION_NAME_SIZE 32
#define MAX_VALUE_SIZE        80
#define MAX_SET_ELEMENTS      20

typedef struct
{
  int numberOfElements;
  int *elementCode;
  char NWFAR * NWFAR *elementName;
  int NWFAR *elementValue;
} SetTableStruct;

typedef struct
{
  int paramType;
  long defaultValue;
} TypeDefaultStruct;

typedef union
{
  char NWFAR *string;
  unsigned int number;
  unsigned long longNumber;
} PARAMETER_TABLE_TYPE;

typedef struct
{
  int keywordCode;
  char NWFAR *keyword;
  void (NWFAR *function)(PARAMETER_TABLE_TYPE NWFAR *);
  TypeDefaultStruct typeDefault[MAX_PARAMETERS];
} GrammarTableStruct;

#ifdef __cplusplus
extern "C" {
#endif
int NWAPI NWParseConfig(
  PCHAR configFile,
  PCHAR sectionName,
  UINT  sectionInstance,
  UINT  grammarTableSize,
  GrammarTableStruct NWFAR *grammarTable,
  SetTableStruct NWFAR *setTable);
#ifdef __cplusplus
}
#endif
