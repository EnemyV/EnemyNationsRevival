#ifndef __THREAD32_H__
#define __THREAD32_H__


#define TM_QUIT     0x0001         /* Thread has ended or must end */

typedef void (WINAPI * AITHREAD) (void *pData);

#define API_EXPORT  __declspec (dllexport)

extern "C"
{
void API_EXPORT WINAPI ediEndTaskThread ();
WORD API_EXPORT WINAPI ediGetThrdUtlsVersion();
void API_EXPORT WINAPI ediSetAiFunc (AITHREAD pfn);
void API_EXPORT WINAPI ediStartThread (void *pData);
WORD API_EXPORT WINAPI ediYieldThread ();
}


#endif
