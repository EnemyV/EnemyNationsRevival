#ifndef __THREAD32_H__
#define __THREAD32_H__


#define TM_QUIT     0x0001         /* Thread has ended or must end */

typedef void (WINAPI * AITHREAD) (void *pData);

#define API_EXPORT  __declspec (dllexport)

extern "C"
{
void API_EXPORT ediEndTaskThread ();
WORD API_EXPORT ediGetThrdUtlsVersion();
void API_EXPORT ediSetAiFunc (AITHREAD pfn);
void API_EXPORT ediStartThread (void *pData);
WORD API_EXPORT ediYieldThread ();
}


#endif
