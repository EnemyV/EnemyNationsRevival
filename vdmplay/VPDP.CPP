#include "stdafx.h"
#include "vpnet.h"
#include "dpnet.h"

#include <dplay.h>




// {6B819D60-FD19-11cf-AB39-0020AF71E433}
static const GUID vpDPGUID = 
{ 0x6b819d60, 0xfd19, 0x11cf, { 0xab, 0x39, 0x0, 0x20, 0xaf, 0x71, 0xe4, 0x33 } };


CDPNet::CDPNet(LPCSTR drvrId)
{
	m_dp = NULL;

	

    HRESULT hr = DirectPlayCreate(&vpDPGUID, &m_dp, NULL);
LPDIRECTPLAY FAR *lplpDP,
IUnknown FAR *pUnkOuter )

