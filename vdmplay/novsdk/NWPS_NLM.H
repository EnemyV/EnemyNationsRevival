/*
//ษออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
//บ (c) 1993 Novell, Inc.	All rights reserved.                              บ
//บ																								  บ
//บ THIS WORK IS SUBJECT TO U.S. AND INTERNATIONAL COPYRIGHT LAWS AND		  บ
//บ TREATIES.																					  บ
//บ																								  บ
//บ NO PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED, COPIED,			  บ
//บ DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED, CONDENSED,         บ
//บ EXPANDED, COLLECTED, COMPILED, LINKED, RECAST, TRANSFORMED OR ADAPTED	  บ
//บ WITHOUT THE PRIOR WRITTEN CONSENT OF NOVELL, INC.                        บ
//บ																								  บ
//บ ANY USE OR EXPLOITATION OF THIS WORK WITHOUT AUTHORIZATION COULD SUBJECT บ
//บ THE PERPETRATOR TO CRIMINAL AND CIVIL LIABILITY.								  บ
//ฬออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออน
//บ Source module name: NWPS_NLM.H														  บ
//ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ
*/
#ifndef NWPS_DEF_INC
#include <nwps_def.h>
#endif

#ifndef NWPS_NLM_INC
#define NWPS_NLM_INC
/****************************************************************************/
/*************** NLM - NLM Library Special Entry Points *********************/
/****************************************************************************/

/****************************************************************************/
/**  N W P S x x x x x x x x x	C a l l s  *********************************/
/****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*++++++++++++ Register and DeRegister a Client ++++++++++++++++++++++++++++*/
int NWPSRegisterLibraryClient( void );

int NWPSDeRegisterLibraryClient( void );

#ifdef __cplusplus
}
#endif

#endif

