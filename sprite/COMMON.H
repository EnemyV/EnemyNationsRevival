#ifndef COMMON_H
#define COMMON_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

//  Get rid of the warnings that windows.h generates in WL4.
#pragma warning( disable : 4201 )
#pragma warning( disable : 4514 )
#pragma warning( disable : 4705 )

#ifndef STRICT
#define STRICT
#endif
#include <windows.h>
#include <windowsx.h>

//  This one needs to be disabled _after_ including windows.h
#pragma warning( disable : 4237 )	//  bool/true/false

//  Create standard types that I tend to use.
typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;
typedef enum { false = 0, true = 1 } bool;

//  standard macros that I like
#define ABS( A )			( ( A ) < 0 ? -( A ) : ( A ) )
#define MAX( A, B )			( ( A ) < ( B ) ? ( B ) : ( A ) )
#define MIN( A, B )			( ( A ) < ( B ) ? ( A ) : ( B ) )
#define CLAMP( A, X, B )	( ( X ) < ( A ) ? ( A ) : ( ( X ) > ( B ) ? ( B ) : ( X ) ) )
#define DEG_TO_RAD( A )		( ( A ) * 0.017453 )

#endif
