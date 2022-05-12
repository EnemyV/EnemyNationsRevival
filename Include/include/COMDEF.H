/***
* comdef.h - Native C++ compiler COM support - main definitions header
*
*	Copyright (C) 1996-1997 Microsoft Corporation
*	All rights reserved.
*
****/

#if _MSC_VER > 1000
#pragma once
#endif

#if !defined(_INC_COMDEF)
#define _INC_COMDEF

#ifndef  __cplusplus
#error Native Compiler support only available in C++ compiler
#endif

#include <ole2.h>
#include <olectl.h>

#include <comutil.h>

#if defined(_COM_DEBUG)
#pragma comment(lib, "comsuppd.lib")
#else
#pragma comment(lib, "comsupp.lib")
#endif	/* _COM_DEBUG */

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

class _com_error;

void __stdcall
	_com_issue_error(HRESULT) throw(_com_error);
void __stdcall
	_com_issue_errorex(HRESULT, IUnknown*, REFIID) throw(_com_error);

HRESULT __stdcall
	_com_dispatch_propget(IDispatch*, DISPID, VARTYPE, void*) throw(_com_error);
HRESULT __cdecl
	_com_dispatch_propput(IDispatch*, DISPID, VARTYPE, ...) throw(_com_error);
HRESULT __cdecl
	_com_dispatch_method(IDispatch*, DISPID, WORD, VARTYPE, void*,
						 const wchar_t*, ...) throw(_com_error);

HRESULT __stdcall
	_com_dispatch_raw_propget(IDispatch*, DISPID, VARTYPE, void*) throw();
HRESULT __cdecl
	_com_dispatch_raw_propput(IDispatch*, DISPID, VARTYPE, ...) throw();
HRESULT __cdecl
	_com_dispatch_raw_method(IDispatch*, DISPID, WORD, VARTYPE, void*,
							 const wchar_t*, ...) throw();

class _com_error {
public:
	// Constructors
	//
	_com_error(HRESULT hr, IErrorInfo* perrinfo = NULL) throw();
	_com_error(const _com_error& that) throw();

	// Destructor
	//
	virtual ~_com_error() throw();

	// Assignment operator
	//
	_com_error& operator=(const _com_error& that) throw();

	// Accessors
	//
	HRESULT Error() const throw();
	WORD WCode() const throw();
	IErrorInfo * ErrorInfo() const throw();

	// IErrorInfo method accessors
	//
	_bstr_t Description() const throw(_com_error);
	DWORD HelpContext() const throw();
	_bstr_t HelpFile() const throw(_com_error);
	_bstr_t Source() const throw(_com_error);
	GUID GUID() const throw();

	// FormatMessage accessors
	const TCHAR * ErrorMessage() const throw();

	// EXCEPINFO.wCode <-> HRESULT mappers
	static HRESULT WCodeToHRESULT(WORD wCode) throw();
	static WORD HRESULTToWCode(HRESULT hr) throw();

private:
	enum {
		WCODE_HRESULT_FIRST = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x200),
		WCODE_HRESULT_LAST = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF+1, 0) - 1
	};
	const HRESULT			m_hresult;
	IErrorInfo *			m_perrinfo;
	mutable TCHAR *			m_pszMsg;
};

inline _com_error::_com_error(HRESULT hr, IErrorInfo* perrinfo) throw()
	: m_hresult(hr), m_perrinfo(perrinfo), m_pszMsg(NULL)
{
	if (m_perrinfo != NULL) {
		m_perrinfo->AddRef();
	}
}

inline _com_error::_com_error(const _com_error& that) throw()
	: m_hresult(that.m_hresult), m_perrinfo(that.m_perrinfo), m_pszMsg(NULL)
{
	if (m_perrinfo != NULL) {
		m_perrinfo->AddRef();
	}
}

inline _com_error::~_com_error() throw()
{
	if (m_perrinfo != NULL) {
		m_perrinfo->Release();
	}
	if (m_pszMsg != NULL) {
		LocalFree((HLOCAL)m_pszMsg);
	}
}

inline _com_error& _com_error::operator=(const _com_error& that) throw()
{
	if (this != &that) {
		this->_com_error::~_com_error();
		this->_com_error::_com_error(that);
	}
	return *this;
}

inline HRESULT _com_error::Error() const throw()
{
	return m_hresult;
}

inline WORD _com_error::WCode() const throw()
{
	return HRESULTToWCode(m_hresult);
}

inline IErrorInfo * _com_error::ErrorInfo() const throw()
{
	if (m_perrinfo != NULL) {
		m_perrinfo->AddRef();
	}
	return m_perrinfo;
}

inline _bstr_t _com_error::Description() const throw(_com_error)
{
	BSTR bstr = NULL;
	if (m_perrinfo != NULL) {
		m_perrinfo->GetDescription(&bstr);
	}
	return _bstr_t(bstr, false);
}

inline DWORD _com_error::HelpContext() const throw()
{
	DWORD dwHelpContext = 0;
	if (m_perrinfo != NULL) {
		m_perrinfo->GetHelpContext(&dwHelpContext);
	}
	return dwHelpContext;
}

inline _bstr_t _com_error::HelpFile() const throw(_com_error)
{
	BSTR bstr = NULL;
	if (m_perrinfo != NULL) {
		m_perrinfo->GetHelpFile(&bstr);
	}
	return _bstr_t(bstr, false);
}

inline _bstr_t _com_error::Source() const throw(_com_error)
{
	BSTR bstr = NULL;
	if (m_perrinfo != NULL) {
		m_perrinfo->GetSource(&bstr);
	}
	return _bstr_t(bstr, false);
}

inline _GUID _com_error::GUID() const throw()
{
	_GUID guid;
	memcpy(&guid, &__uuidof(NULL), sizeof(_GUID));
	if (m_perrinfo != NULL) {
		m_perrinfo->GetGUID(&guid);
	}
	return guid;
}

inline const TCHAR * _com_error::ErrorMessage() const throw()
{
	if (m_pszMsg == NULL) {
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
					  NULL,
					  m_hresult,
					  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					  (LPTSTR)&m_pszMsg,
					  0,
					  NULL );
		if (m_pszMsg != NULL) {
			int nLen = lstrlen(m_pszMsg);
			if (nLen > 1 && m_pszMsg[nLen - 1] == '\n') {
				m_pszMsg[nLen - 1] == 0;
			}
		} else {
			m_pszMsg = (LPTSTR)LocalAlloc(0, 32 * sizeof(TCHAR));
			if (m_pszMsg != NULL) {
				WORD wCode = WCode();
				if (wCode != 0) {
					wsprintf(m_pszMsg, TEXT("IDispatch error #%d"), wCode);
				} else {
					wsprintf(m_pszMsg, TEXT("Unknown error 0x%0lX"), m_hresult);
				}
			}
		}
	}
	return m_pszMsg;
}

inline HRESULT _com_error::WCodeToHRESULT(WORD wCode) throw()
{
	return wCode >= 0xFE00 ? WCODE_HRESULT_LAST : WCODE_HRESULT_FIRST + wCode;
}

inline WORD _com_error::HRESULTToWCode(HRESULT hr) throw()
{
	return (hr >= WCODE_HRESULT_FIRST && hr <= WCODE_HRESULT_LAST)
		? WORD(hr - WCODE_HRESULT_FIRST)
		: 0;
}

#if !defined(_COM_SMARTPTR)
 #if !defined(_INC_COMIP)
  #include <comip.h>
 #endif
 #define _COM_SMARTPTR        _com_ptr_t
 #define _COM_SMARTPTR_LEVEL2 _com_IIID
#endif
#if defined(_COM_SMARTPTR)
 #if !defined(_COM_SMARTPTR_TYPEDEF)
  #if defined(_COM_SMARTPTR_LEVEL2
   #define _COM_SMARTPTR_TYPEDEF(Interface, IID) \
    typedef _COM_SMARTPTR<_COM_SMARTPTR_LEVEL2<Interface, &IID> > \
            Interface ## Ptr
  #else
   #define _COM_SMARTPTR_TYPEDEF(Interface, IID) \
    typedef _COM_SMARTPTR<Interface, &IID> \
            Interface ## Ptr
  #endif
 #endif
#endif

// Interfaces required by COM headers
struct __declspec(uuid("00000000-0000-0000-c000-000000000046")) IUnknown;
struct __declspec(uuid("00020400-0000-0000-c000-000000000046")) IDispatch;

#if !defined(_COM_NO_STANDARD_GUIDS_)

// Interfaces:
struct __declspec(uuid("00000001-0000-0000-c000-000000000046")) IClassFactory;
struct __declspec(uuid("00000002-0000-0000-c000-000000000046")) IMalloc;
struct __declspec(uuid("00000003-0000-0000-c000-000000000046")) IMarshal;
struct __declspec(uuid("00000004-0000-0000-c000-000000000046")) IRpcChannel;
struct __declspec(uuid("00000005-0000-0000-c000-000000000046")) IRpcStub;
struct __declspec(uuid("00000006-0000-0000-c000-000000000046")) IStubManager;
struct __declspec(uuid("00000007-0000-0000-c000-000000000046")) IRpcProxy;
struct __declspec(uuid("00000008-0000-0000-c000-000000000046")) IProxyManager;
struct __declspec(uuid("00000009-0000-0000-c000-000000000046")) IPSFactory;
struct __declspec(uuid("0000000a-0000-0000-c000-000000000046")) ILockBytes;
struct __declspec(uuid("0000000b-0000-0000-c000-000000000046")) IStorage;
struct __declspec(uuid("0000000c-0000-0000-c000-000000000046")) IStream;
struct __declspec(uuid("0000000d-0000-0000-c000-000000000046")) IEnumSTATSTG;
struct __declspec(uuid("0000000e-0000-0000-c000-000000000046")) IBindCtx;
struct __declspec(uuid("0000000f-0000-0000-c000-000000000046")) IMoniker;
struct __declspec(uuid("00000010-0000-0000-c000-000000000046")) IRunningObjectTable;
struct __declspec(uuid("00000011-0000-0000-c000-000000000046")) IInternalMoniker;
struct __declspec(uuid("00000012-0000-0000-c000-000000000046")) IRootStorage;
struct __declspec(uuid("00000013-0000-0000-c000-000000000046")) IDfReserved1;
struct __declspec(uuid("00000014-0000-0000-c000-000000000046")) IDfReserved2;
struct __declspec(uuid("00000015-0000-0000-c000-000000000046")) IDfReserved3;
struct __declspec(uuid("00000016-0000-0000-c000-000000000046")) IMessageFilter;
struct __declspec(uuid("00000018-0000-0000-c000-000000000046")) IStdMarshalInfo;
struct __declspec(uuid("00000019-0000-0000-c000-000000000046")) IExternalConnection;
struct __declspec(uuid("0000001d-0000-0000-c000-000000000046")) IMallocSpy;
struct __declspec(uuid("00000020-0000-0000-c000-000000000046")) IMultiQI;
struct __declspec(uuid("00000026-0000-0000-c000-000000000046")) IStub;
struct __declspec(uuid("00000027-0000-0000-c000-000000000046")) IProxy;
struct __declspec(uuid("00000100-0000-0000-c000-000000000046")) IEnumUnknown;
struct __declspec(uuid("00000101-0000-0000-c000-000000000046")) IEnumString;
struct __declspec(uuid("00000102-0000-0000-c000-000000000046")) IEnumMoniker;
struct __declspec(uuid("00000103-0000-0000-c000-000000000046")) IEnumFORMATETC;
struct __declspec(uuid("00000104-0000-0000-c000-000000000046")) IEnumOLEVERB;
struct __declspec(uuid("00000105-0000-0000-c000-000000000046")) IEnumSTATDATA;
struct __declspec(uuid("00000106-0000-0000-c000-000000000046")) IEnumGeneric;
struct __declspec(uuid("00000107-0000-0000-c000-000000000046")) IEnumHolder;
struct __declspec(uuid("00000108-0000-0000-c000-000000000046")) IEnumCallback;
struct __declspec(uuid("00000109-0000-0000-c000-000000000046")) IPersistStream;
struct __declspec(uuid("0000010a-0000-0000-c000-000000000046")) IPersistStorage;
struct __declspec(uuid("0000010b-0000-0000-c000-000000000046")) IPersistFile;
struct __declspec(uuid("0000010c-0000-0000-c000-000000000046")) IPersist;
struct __declspec(uuid("0000010d-0000-0000-c000-000000000046")) IViewObject;
struct __declspec(uuid("0000010e-0000-0000-c000-000000000046")) IDataObject;
struct __declspec(uuid("0000010f-0000-0000-c000-000000000046")) IAdviseSink;
struct __declspec(uuid("00000110-0000-0000-c000-000000000046")) IDataAdviseHolder;
struct __declspec(uuid("00000111-0000-0000-c000-000000000046")) IOleAdviseHolder;
struct __declspec(uuid("00000112-0000-0000-c000-000000000046")) IOleObject;
struct __declspec(uuid("00000113-0000-0000-c000-000000000046")) IOleInPlaceObject;
struct __declspec(uuid("00000114-0000-0000-c000-000000000046")) IOleWindow;
struct __declspec(uuid("00000115-0000-0000-c000-000000000046")) IOleInPlaceUIWindow;
struct __declspec(uuid("00000116-0000-0000-c000-000000000046")) IOleInPlaceFrame;
struct __declspec(uuid("00000117-0000-0000-c000-000000000046")) IOleInPlaceActiveObject;
struct __declspec(uuid("00000118-0000-0000-c000-000000000046")) IOleClientSite;
struct __declspec(uuid("00000119-0000-0000-c000-000000000046")) IOleInPlaceSite;
struct __declspec(uuid("0000011a-0000-0000-c000-000000000046")) IParseDisplayName;
struct __declspec(uuid("0000011b-0000-0000-c000-000000000046")) IOleContainer;
struct __declspec(uuid("0000011c-0000-0000-c000-000000000046")) IOleItemContainer;
struct __declspec(uuid("0000011d-0000-0000-c000-000000000046")) IOleLink;
struct __declspec(uuid("0000011e-0000-0000-c000-000000000046")) IOleCache;
struct __declspec(uuid("0000011f-0000-0000-c000-000000000046")) IOleManager;
struct __declspec(uuid("00000120-0000-0000-c000-000000000046")) IOlePresObj;
struct __declspec(uuid("00000121-0000-0000-c000-000000000046")) IDropSource;
struct __declspec(uuid("00000122-0000-0000-c000-000000000046")) IDropTarget;
struct __declspec(uuid("00000123-0000-0000-c000-000000000046")) IDebug;
struct __declspec(uuid("00000124-0000-0000-c000-000000000046")) IDebugStream;
struct __declspec(uuid("00000125-0000-0000-c000-000000000046")) IAdviseSink2;
struct __declspec(uuid("00000126-0000-0000-c000-000000000046")) IRunnableObject;
struct __declspec(uuid("00000127-0000-0000-c000-000000000046")) IViewObject2;
struct __declspec(uuid("00000128-0000-0000-c000-000000000046")) IOleCache2;
struct __declspec(uuid("00000129-0000-0000-c000-000000000046")) IOleCacheControl;
struct __declspec(uuid("0000012a-0000-0000-c000-000000000046")) IContinue;
struct __declspec(uuid("00000138-0000-0000-c000-000000000046")) IPropertyStorage;
struct __declspec(uuid("00000139-0000-0000-c000-000000000046")) IEnumSTATPROPSTG;
struct __declspec(uuid("0000013a-0000-0000-c000-000000000046")) IPropertySetStorage;
struct __declspec(uuid("0000013b-0000-0000-c000-000000000046")) IEnumSTATPROPSETSTG;
struct __declspec(uuid("0000013d-0000-0000-c000-000000000046")) IClientSecurity;
struct __declspec(uuid("0000013e-0000-0000-c000-000000000046")) IServerSecurity;
struct __declspec(uuid("00000140-0000-0000-c000-000000000046")) IClassActivator;
struct __declspec(uuid("00020401-0000-0000-c000-000000000046")) ITypeInfo;
struct __declspec(uuid("00020402-0000-0000-c000-000000000046")) ITypeLib;
struct __declspec(uuid("00020403-0000-0000-c000-000000000046")) ITypeComp;
struct __declspec(uuid("00020404-0000-0000-c000-000000000046")) IEnumVARIANT;
struct __declspec(uuid("00020405-0000-0000-c000-000000000046")) ICreateTypeInfo;
struct __declspec(uuid("00020406-0000-0000-c000-000000000046")) ICreateTypeLib;
struct __declspec(uuid("0002040e-0000-0000-c000-000000000046")) ICreateTypeInfo2;
struct __declspec(uuid("0002040f-0000-0000-c000-000000000046")) ICreateTypeLib2;
struct __declspec(uuid("00020410-0000-0000-c000-000000000046")) ITypeChangeEvents;
struct __declspec(uuid("00020411-0000-0000-c000-000000000046")) ITypeLib2;
struct __declspec(uuid("00020412-0000-0000-c000-000000000046")) ITypeInfo2;
struct __declspec(uuid("00020430-0000-0000-c000-000000000046")) StdOle;
struct __declspec(uuid("00020d00-0000-0000-c000-000000000046")) IRichEditOle;
struct __declspec(uuid("00020d03-0000-0000-c000-000000000046")) IRichEditOleCallback;
struct __declspec(uuid("0002e000-0000-0000-c000-000000000046")) IEnumGUID;
struct __declspec(uuid("0002e011-0000-0000-c000-000000000046")) IEnumCATEGORYINFO;
struct __declspec(uuid("0002e012-0000-0000-c000-000000000046")) ICatRegister;
struct __declspec(uuid("0002e013-0000-0000-c000-000000000046")) ICatInformation;
struct __declspec(uuid("6d5140c1-7436-11ce-8034-00aa006009fa")) IServiceProvider;
struct __declspec(uuid("37d84f60-42cb-11ce-8135-00aa004bb851")) IPersistPropertyBag;
struct __declspec(uuid("55272a00-42cb-11ce-8135-00aa004bb851")) IPropertyBag;
struct __declspec(uuid("3127ca40-446e-11ce-8135-00aa004bb851")) IErrorLog;
struct __declspec(uuid("9bfbbc00-eff1-101a-84ed-00aa00341d07")) IBoundObject;
struct __declspec(uuid("9bfbbc01-eff1-101a-84ed-00aa00341d07")) IBoundObjectSite;
struct __declspec(uuid("9bfbbc02-eff1-101a-84ed-00aa00341d07")) IPropertyNotifySink;
struct __declspec(uuid("01e44665-24ac-101b-84ed-08002b2ec713")) IPropertyPage2;
struct __declspec(uuid("376bd3aa-3845-101b-84ed-08002b2ec713")) IPerPropertyBrowsing;
struct __declspec(uuid("99caf010-415e-11cf-8814-00aa00b569f5")) IFillLockBytes;
struct __declspec(uuid("bef6e002-a874-101a-8bba-00aa00300cab")) IFont;
struct __declspec(uuid("bef6e003-a874-101a-8bba-00aa00300cab")) IFontDisp;
struct __declspec(uuid("7bf80980-bf32-101a-8bbb-00aa00300cab")) IPicture;
struct __declspec(uuid("7bf80981-bf32-101a-8bbb-00aa00300cab")) IPictureDisp;
struct __declspec(uuid("1c2056cc-5ef4-101b-8bc8-00aa003e3b29")) IOleInPlaceObjectWindowless;
struct __declspec(uuid("79eac9c0-baf9-11ce-8c82-00aa004ba90b")) IBinding;
struct __declspec(uuid("79eac9c1-baf9-11ce-8c82-00aa004ba90b")) IBindStatusCallback;
struct __declspec(uuid("79eac9c2-baf9-11ce-8c82-00aa004ba90b")) IHlinkSite;
struct __declspec(uuid("79eac9c3-baf9-11ce-8c82-00aa004ba90b")) IHlink;
struct __declspec(uuid("79eac9c4-baf9-11ce-8c82-00aa004ba90b")) IHlinkTarget;
struct __declspec(uuid("79eac9c5-baf9-11ce-8c82-00aa004ba90b")) IHlinkFrame;
struct __declspec(uuid("79eac9c6-baf9-11ce-8c82-00aa004ba90b")) IEnumHLITEM;
struct __declspec(uuid("79eac9c7-baf9-11ce-8c82-00aa004ba90b")) IHlinkBrowseContext;
struct __declspec(uuid("79eac9c9-baf9-11ce-8c82-00aa004ba90b")) IPersistMoniker;
struct __declspec(uuid("79eac9cb-baf9-11ce-8c82-00aa004ba90b")) IBindStatusCallbackMsg;
struct __declspec(uuid("79eac9cd-baf9-11ce-8c82-00aa004ba90b")) IBindProtocol;
struct __declspec(uuid("79eac9d0-baf9-11ce-8c82-00aa004ba90b")) IAuthenticate;
struct __declspec(uuid("79eac9d1-baf9-11ce-8c82-00aa004ba90b")) ICodeInstall;
struct __declspec(uuid("79eac9d2-baf9-11ce-8c82-00aa004ba90b")) IHttpNegotiate;
struct __declspec(uuid("79eac9d3-baf9-11ce-8c82-00aa004ba90b")) IAsyncMoniker;
struct __declspec(uuid("79eac9d5-bafa-11ce-8c82-00aa004ba90b")) IWindowForBindingUI;
struct __declspec(uuid("79eac9d6-bafa-11ce-8c82-00aa004ba90b")) IWinInetInfo;
struct __declspec(uuid("79eac9d7-bafa-11ce-8c82-00aa004ba90b")) IHttpSecurity;
struct __declspec(uuid("79eac9d8-bafa-11ce-8c82-00aa004ba90b")) IWinInetHttpInfo;
struct __declspec(uuid("1cf2b120-547d-101b-8e65-08002b2bd119")) IErrorInfo;
struct __declspec(uuid("22f03340-547d-101b-8e65-08002b2bd119")) ICreateErrorInfo;
struct __declspec(uuid("df0b3d60-548f-101b-8e65-08002b2bd119")) ISupportErrorInfo;
struct __declspec(uuid("d10f6761-83e9-11cf-8f20-00805f2cd064")) IActiveScriptSiteWindow;
struct __declspec(uuid("cb5bdc81-93c1-11cf-8f20-00805f2cd064")) IObjectSafety;
struct __declspec(uuid("db01a1e3-a42b-11cf-8f20-00805f2cd064")) IActiveScriptSite;
struct __declspec(uuid("eae1ba61-a4ed-11cf-8f20-00805f2cd064")) IActiveScriptError;
struct __declspec(uuid("bb1a2ae1-a4f9-11cf-8f20-00805f2cd064")) IActiveScript;
struct __declspec(uuid("bb1a2ae2-a4f9-11cf-8f20-00805f2cd064")) IActiveScriptParse;
struct __declspec(uuid("d5f78c80-5252-11cf-90fa-00aa0042106e")) ITargetFrame;
struct __declspec(uuid("742b0e01-14e6-101b-914e-00aa00300cab")) ISimpleFrameSite;
struct __declspec(uuid("a9d758a0-4617-11cf-95fc-00aa00680db4")) IProgressNotify;
struct __declspec(uuid("0e6d4d90-6738-11cf-9608-00aa00680db4")) ILayoutStorage;
struct __declspec(uuid("548793c0-9e74-11cf-9655-00a0c9034923")) ITargetEmbedding;
struct __declspec(uuid("1008c4a0-7613-11cf-9af1-0020af6e72f4")) IChannelHook;
struct __declspec(uuid("894ad3b0-ef97-11ce-9bc9-00aa00608e01")) IOleUndoUnit;
struct __declspec(uuid("a1faf330-ef97-11ce-9bc9-00aa00608e01")) IOleParentUndoUnit;
struct __declspec(uuid("b3e7c340-ef97-11ce-9bc9-00aa00608e01")) IEnumOleUndoUnits;
struct __declspec(uuid("d001f200-ef97-11ce-9bc9-00aa00608e01")) IOleUndoManager;
struct __declspec(uuid("a6bc3ac0-dbaa-11ce-9de3-00aa004bb851")) IProvideClassInfo2;
struct __declspec(uuid("3af24290-0c96-11ce-a0cf-00aa00600ab8")) IAdviseSinkEx;
struct __declspec(uuid("3af24292-0c96-11ce-a0cf-00aa00600ab8")) IViewObjectEx;
struct __declspec(uuid("fc4801a1-2ba9-11cf-a229-00aa003d7352")) IBindHost;
struct __declspec(uuid("fc4801a3-2ba9-11cf-a229-00aa003d7352")) IObjectWithSite;
struct __declspec(uuid("b722bcc5-4e68-101b-a2bc-00aa00404770")) IOleDocument;
struct __declspec(uuid("b722bcc6-4e68-101b-a2bc-00aa00404770")) IOleDocumentView;
struct __declspec(uuid("b722bcc7-4e68-101b-a2bc-00aa00404770")) IOleDocumentSite;
struct __declspec(uuid("b722bcc8-4e68-101b-a2bc-00aa00404770")) IEnumOleDocumentViews;
struct __declspec(uuid("b722bcc9-4e68-101b-a2bc-00aa00404770")) IPrint;
struct __declspec(uuid("b722bcca-4e68-101b-a2bc-00aa00404770")) IContinueCallback;
struct __declspec(uuid("b722bccb-4e68-101b-a2bc-00aa00404770")) IOleCommandTarget;
struct __declspec(uuid("f29f6bc0-5021-11ce-aa15-00006901293f")) IROTData;
struct __declspec(uuid("9f6aa700-d188-11cd-ad48-00aa003c9cb6")) ICursor;
struct __declspec(uuid("acff0690-d188-11cd-ad48-00aa003c9cb6")) ICursorMove;
struct __declspec(uuid("bb87e420-d188-11cd-ad48-00aa003c9cb6")) ICursorScroll;
struct __declspec(uuid("d14216a0-d188-11cd-ad48-00aa003c9cb6")) ICursorUpdateARow;
struct __declspec(uuid("db526cc0-d188-11cd-ad48-00aa003c9cb6")) INotifyDBEvents;
struct __declspec(uuid("e01d7850-d188-11cd-ad48-00aa003c9cb6")) ICursorFind;
struct __declspec(uuid("e4d19810-d188-11cd-ad48-00aa003c9cb6")) IEntryID;
struct __declspec(uuid("0c733a30-2a1c-11ce-ade5-00aa0044773d")) ISequentialStream;
struct __declspec(uuid("7fd52380-4e07-101b-ae2d-08002b2ec713")) IPersistStreamInit;
struct __declspec(uuid("49384070-e40a-11ce-b2c9-00aa00680937")) IOverlappedStream;
struct __declspec(uuid("521a28f0-e40b-11ce-b2c9-00aa00680937")) IOverlappedCompletion;
struct __declspec(uuid("d5f569d0-593b-101a-b569-08002b2dbf7a")) IPSFactoryBuffer;
struct __declspec(uuid("d5f56a34-593b-101a-b569-08002b2dbf7a")) IRpcProxyBuffer;
struct __declspec(uuid("d5f56afc-593b-101a-b569-08002b2dbf7a")) IRpcStubBuffer;
struct __declspec(uuid("d5f56b60-593b-101a-b569-08002b2dbf7a")) IRpcChannelBuffer;
struct __declspec(uuid("f4f569d0-593b-101a-b569-08002b2dbf7a")) IServerHandler;
struct __declspec(uuid("f4f569d1-593b-101a-b569-08002b2dbf7a")) IClientSiteHandler;
struct __declspec(uuid("922eada0-3424-11cf-b670-00aa004cd6d8")) IOleInPlaceSiteWindowless;
struct __declspec(uuid("9c2cad80-3424-11cf-b670-00aa004cd6d8")) IOleInPlaceSiteEx;
struct __declspec(uuid("55980ba0-35aa-11cf-b671-00aa004cd6d8")) IPointerInactive;
struct __declspec(uuid("b196b283-bab4-101a-b69c-00aa00341d07")) IProvideClassInfo;
struct __declspec(uuid("b196b284-bab4-101a-b69c-00aa00341d07")) IConnectionPointContainer;
struct __declspec(uuid("b196b285-bab4-101a-b69c-00aa00341d07")) IEnumConnectionPoints;
struct __declspec(uuid("b196b286-bab4-101a-b69c-00aa00341d07")) IConnectionPoint;
struct __declspec(uuid("b196b287-bab4-101a-b69c-00aa00341d07")) IEnumConnections;
struct __declspec(uuid("b196b288-bab4-101a-b69c-00aa00341d07")) IOleControl;
struct __declspec(uuid("b196b289-bab4-101a-b69c-00aa00341d07")) IOleControlSite;
struct __declspec(uuid("b196b28a-bab4-101a-b69c-00aa00341d07")) IPropertyFrame;
struct __declspec(uuid("b196b28b-bab4-101a-b69c-00aa00341d07")) ISpecifyPropertyPages;
struct __declspec(uuid("b196b28c-bab4-101a-b69c-00aa00341d07")) IPropertyPageSite;
struct __declspec(uuid("b196b28d-bab4-101a-b69c-00aa00341d07")) IPropertyPage;
struct __declspec(uuid("b196b28f-bab4-101a-b69c-00aa00341d07")) IClassFactory2;
struct __declspec(uuid("f77459a0-bf9a-11cf-ba4e-00c04fd70816")) IMimeInfo;
struct __declspec(uuid("89bcb740-6119-101a-bcb7-00dd010655af")) IFilter;
struct __declspec(uuid("bd1ae5e0-a6ae-11ce-bd37-504200c10000")) IPersistMemory;
struct __declspec(uuid("3c374a41-bae4-11cf-bf7d-00aa006946ee")) IUrlHistoryStg;
struct __declspec(uuid("3c374a42-bae4-11cf-bf7d-00aa006946ee")) IEnumSTATURL;
struct __declspec(uuid("cf51ed10-62fe-11cf-bf86-00a0c9034836")) IQuickActivate;

// CoClasses:
struct __declspec(uuid("00000017-0000-0000-c000-000000000046")) StdMarshal;
struct __declspec(uuid("0000001b-0000-0000-c000-000000000046")) IdentityUnmarshal;
struct __declspec(uuid("0000001c-0000-0000-c000-000000000046")) InProcFreeMarshaler;
struct __declspec(uuid("0000030c-0000-0000-c000-000000000046")) PSGenObject;
struct __declspec(uuid("0000030d-0000-0000-c000-000000000046")) PSClientSite;
struct __declspec(uuid("0000030e-0000-0000-c000-000000000046")) PSClassObject;
struct __declspec(uuid("0000030f-0000-0000-c000-000000000046")) PSInPlaceActive;
struct __declspec(uuid("00000310-0000-0000-c000-000000000046")) PSInPlaceFrame;
struct __declspec(uuid("00000311-0000-0000-c000-000000000046")) PSDragDrop;
struct __declspec(uuid("00000312-0000-0000-c000-000000000046")) PSBindCtx;
struct __declspec(uuid("00000313-0000-0000-c000-000000000046")) PSEnumerators;
struct __declspec(uuid("00000315-0000-0000-c000-000000000046")) Picture_Metafile;
struct __declspec(uuid("00000315-0000-0000-c000-000000000046")) StaticMetafile;
struct __declspec(uuid("00000316-0000-0000-c000-000000000046")) Picture_Dib;
struct __declspec(uuid("00000316-0000-0000-c000-000000000046")) StaticDib;
struct __declspec(uuid("00000319-0000-0000-c000-000000000046")) Picture_EnhMetafile;
struct __declspec(uuid("00000330-0000-0000-c000-000000000046")) AllClasses;
struct __declspec(uuid("00000331-0000-0000-c000-000000000046")) LocalMachineClasses;
struct __declspec(uuid("00000332-0000-0000-c000-000000000046")) CurrentUserClasses;
struct __declspec(uuid("0002e005-0000-0000-c000-000000000046")) StdComponentCategoriesMgr;
struct __declspec(uuid("fb8f0821-0164-101b-84ed-08002b2ec713")) PersistPropset;
struct __declspec(uuid("fb8f0822-0164-101b-84ed-08002b2ec713")) ConvertVBX;
struct __declspec(uuid("79eac9d0-baf9-11ce-8c82-00aa004ba90b")) StdHlink;
struct __declspec(uuid("79eac9d1-baf9-11ce-8c82-00aa004ba90b")) StdHlinkBrowseContext;
struct __declspec(uuid("79eac9e0-baf9-11ce-8c82-00aa004ba90b")) StdURLMoniker;
struct __declspec(uuid("79eac9e1-baf9-11ce-8c82-00aa004ba90b")) StdURLProtocol;
struct __declspec(uuid("79eac9e2-baf9-11ce-8c82-00aa004ba90b")) HttpProtocol;
struct __declspec(uuid("79eac9e3-baf9-11ce-8c82-00aa004ba90b")) FtpProtocol;
struct __declspec(uuid("79eac9e4-baf9-11ce-8c82-00aa004ba90b")) GopherProtocol;
struct __declspec(uuid("79eac9e5-baf9-11ce-8c82-00aa004ba90b")) HttpSProtocol;
struct __declspec(uuid("79eac9e6-baf9-11ce-8c82-00aa004ba90b")) MkProtocol;
struct __declspec(uuid("79eac9e7-baf9-11ce-8c82-00aa004ba90b")) FileProtocol;
struct __declspec(uuid("79eac9f1-baf9-11ce-8c82-00aa004ba90b")) PSUrlMonProxy;
struct __declspec(uuid("0be35200-8f91-11ce-9de3-00aa004bb851")) CFontPropPage;
struct __declspec(uuid("0be35201-8f91-11ce-9de3-00aa004bb851")) CColorPropPage;
struct __declspec(uuid("0be35202-8f91-11ce-9de3-00aa004bb851")) CPicturePropPage;
struct __declspec(uuid("0be35203-8f91-11ce-9de3-00aa004bb851")) StdFont;
struct __declspec(uuid("0be35204-8f91-11ce-9de3-00aa004bb851")) StdPicture;

// Interface Smart Pointers:
_COM_SMARTPTR_TYPEDEF(IUnknown, __uuidof(IUnknown));
_COM_SMARTPTR_TYPEDEF(IClassFactory, __uuidof(IClassFactory));
_COM_SMARTPTR_TYPEDEF(IMalloc, __uuidof(IMalloc));
_COM_SMARTPTR_TYPEDEF(IMarshal, __uuidof(IMarshal));
_COM_SMARTPTR_TYPEDEF(IRpcChannel, __uuidof(IRpcChannel));
_COM_SMARTPTR_TYPEDEF(IRpcStub, __uuidof(IRpcStub));
_COM_SMARTPTR_TYPEDEF(IStubManager, __uuidof(IStubManager));
_COM_SMARTPTR_TYPEDEF(IRpcProxy, __uuidof(IRpcProxy));
_COM_SMARTPTR_TYPEDEF(IProxyManager, __uuidof(IProxyManager));
_COM_SMARTPTR_TYPEDEF(IPSFactory, __uuidof(IPSFactory));
_COM_SMARTPTR_TYPEDEF(ILockBytes, __uuidof(ILockBytes));
_COM_SMARTPTR_TYPEDEF(IStorage, __uuidof(IStorage));
_COM_SMARTPTR_TYPEDEF(IStream, __uuidof(IStream));
_COM_SMARTPTR_TYPEDEF(IEnumSTATSTG, __uuidof(IEnumSTATSTG));
_COM_SMARTPTR_TYPEDEF(IBindCtx, __uuidof(IBindCtx));
_COM_SMARTPTR_TYPEDEF(IMoniker, __uuidof(IMoniker));
_COM_SMARTPTR_TYPEDEF(IRunningObjectTable, __uuidof(IRunningObjectTable));
_COM_SMARTPTR_TYPEDEF(IInternalMoniker, __uuidof(IInternalMoniker));
_COM_SMARTPTR_TYPEDEF(IRootStorage, __uuidof(IRootStorage));
_COM_SMARTPTR_TYPEDEF(IDfReserved1, __uuidof(IDfReserved1));
_COM_SMARTPTR_TYPEDEF(IDfReserved2, __uuidof(IDfReserved2));
_COM_SMARTPTR_TYPEDEF(IDfReserved3, __uuidof(IDfReserved3));
_COM_SMARTPTR_TYPEDEF(IMessageFilter, __uuidof(IMessageFilter));
_COM_SMARTPTR_TYPEDEF(IStdMarshalInfo, __uuidof(IStdMarshalInfo));
_COM_SMARTPTR_TYPEDEF(IExternalConnection, __uuidof(IExternalConnection));
_COM_SMARTPTR_TYPEDEF(IMallocSpy, __uuidof(IMallocSpy));
_COM_SMARTPTR_TYPEDEF(IMultiQI, __uuidof(IMultiQI));
_COM_SMARTPTR_TYPEDEF(IStub, __uuidof(IStub));
_COM_SMARTPTR_TYPEDEF(IProxy, __uuidof(IProxy));
_COM_SMARTPTR_TYPEDEF(IEnumUnknown, __uuidof(IEnumUnknown));
_COM_SMARTPTR_TYPEDEF(IEnumString, __uuidof(IEnumString));
_COM_SMARTPTR_TYPEDEF(IEnumMoniker, __uuidof(IEnumMoniker));
_COM_SMARTPTR_TYPEDEF(IEnumFORMATETC, __uuidof(IEnumFORMATETC));
_COM_SMARTPTR_TYPEDEF(IEnumOLEVERB, __uuidof(IEnumOLEVERB));
_COM_SMARTPTR_TYPEDEF(IEnumSTATDATA, __uuidof(IEnumSTATDATA));
_COM_SMARTPTR_TYPEDEF(IEnumGeneric, __uuidof(IEnumGeneric));
_COM_SMARTPTR_TYPEDEF(IEnumHolder, __uuidof(IEnumHolder));
_COM_SMARTPTR_TYPEDEF(IEnumCallback, __uuidof(IEnumCallback));
_COM_SMARTPTR_TYPEDEF(IPersistStream, __uuidof(IPersistStream));
_COM_SMARTPTR_TYPEDEF(IPersistStorage, __uuidof(IPersistStorage));
_COM_SMARTPTR_TYPEDEF(IPersistFile, __uuidof(IPersistFile));
_COM_SMARTPTR_TYPEDEF(IPersist, __uuidof(IPersist));
_COM_SMARTPTR_TYPEDEF(IViewObject, __uuidof(IViewObject));
_COM_SMARTPTR_TYPEDEF(IDataObject, __uuidof(IDataObject));
_COM_SMARTPTR_TYPEDEF(IAdviseSink, __uuidof(IAdviseSink));
_COM_SMARTPTR_TYPEDEF(IDataAdviseHolder, __uuidof(IDataAdviseHolder));
_COM_SMARTPTR_TYPEDEF(IOleAdviseHolder, __uuidof(IOleAdviseHolder));
_COM_SMARTPTR_TYPEDEF(IOleObject, __uuidof(IOleObject));
_COM_SMARTPTR_TYPEDEF(IOleInPlaceObject, __uuidof(IOleInPlaceObject));
_COM_SMARTPTR_TYPEDEF(IOleWindow, __uuidof(IOleWindow));
_COM_SMARTPTR_TYPEDEF(IOleInPlaceUIWindow, __uuidof(IOleInPlaceUIWindow));
_COM_SMARTPTR_TYPEDEF(IOleInPlaceFrame, __uuidof(IOleInPlaceFrame));
_COM_SMARTPTR_TYPEDEF(IOleInPlaceActiveObject, __uuidof(IOleInPlaceActiveObject));
_COM_SMARTPTR_TYPEDEF(IOleClientSite, __uuidof(IOleClientSite));
_COM_SMARTPTR_TYPEDEF(IOleInPlaceSite, __uuidof(IOleInPlaceSite));
_COM_SMARTPTR_TYPEDEF(IParseDisplayName, __uuidof(IParseDisplayName));
_COM_SMARTPTR_TYPEDEF(IOleContainer, __uuidof(IOleContainer));
_COM_SMARTPTR_TYPEDEF(IOleItemContainer, __uuidof(IOleItemContainer));
_COM_SMARTPTR_TYPEDEF(IOleLink, __uuidof(IOleLink));
_COM_SMARTPTR_TYPEDEF(IOleCache, __uuidof(IOleCache));
_COM_SMARTPTR_TYPEDEF(IOleManager, __uuidof(IOleManager));
_COM_SMARTPTR_TYPEDEF(IOlePresObj, __uuidof(IOlePresObj));
_COM_SMARTPTR_TYPEDEF(IDropSource, __uuidof(IDropSource));
_COM_SMARTPTR_TYPEDEF(IDropTarget, __uuidof(IDropTarget));
_COM_SMARTPTR_TYPEDEF(IDebug, __uuidof(IDebug));
_COM_SMARTPTR_TYPEDEF(IDebugStream, __uuidof(IDebugStream));
_COM_SMARTPTR_TYPEDEF(IAdviseSink2, __uuidof(IAdviseSink2));
_COM_SMARTPTR_TYPEDEF(IRunnableObject, __uuidof(IRunnableObject));
_COM_SMARTPTR_TYPEDEF(IViewObject2, __uuidof(IViewObject2));
_COM_SMARTPTR_TYPEDEF(IOleCache2, __uuidof(IOleCache2));
_COM_SMARTPTR_TYPEDEF(IOleCacheControl, __uuidof(IOleCacheControl));
_COM_SMARTPTR_TYPEDEF(IContinue, __uuidof(IContinue));
_COM_SMARTPTR_TYPEDEF(IPropertyStorage, __uuidof(IPropertyStorage));
_COM_SMARTPTR_TYPEDEF(IEnumSTATPROPSTG, __uuidof(IEnumSTATPROPSTG));
_COM_SMARTPTR_TYPEDEF(IPropertySetStorage, __uuidof(IPropertySetStorage));
_COM_SMARTPTR_TYPEDEF(IEnumSTATPROPSETSTG, __uuidof(IEnumSTATPROPSETSTG));
_COM_SMARTPTR_TYPEDEF(IClientSecurity, __uuidof(IClientSecurity));
_COM_SMARTPTR_TYPEDEF(IServerSecurity, __uuidof(IServerSecurity));
_COM_SMARTPTR_TYPEDEF(IClassActivator, __uuidof(IClassActivator));
_COM_SMARTPTR_TYPEDEF(IDispatch, __uuidof(IDispatch));
_COM_SMARTPTR_TYPEDEF(ITypeInfo, __uuidof(ITypeInfo));
_COM_SMARTPTR_TYPEDEF(ITypeLib, __uuidof(ITypeLib));
_COM_SMARTPTR_TYPEDEF(ITypeComp, __uuidof(ITypeComp));
_COM_SMARTPTR_TYPEDEF(IEnumVARIANT, __uuidof(IEnumVARIANT));
_COM_SMARTPTR_TYPEDEF(ICreateTypeInfo, __uuidof(ICreateTypeInfo));
_COM_SMARTPTR_TYPEDEF(ICreateTypeLib, __uuidof(ICreateTypeLib));
_COM_SMARTPTR_TYPEDEF(ICreateTypeInfo2, __uuidof(ICreateTypeInfo2));
_COM_SMARTPTR_TYPEDEF(ICreateTypeLib2, __uuidof(ICreateTypeLib2));
_COM_SMARTPTR_TYPEDEF(ITypeChangeEvents, __uuidof(ITypeChangeEvents));
_COM_SMARTPTR_TYPEDEF(ITypeLib2, __uuidof(ITypeLib2));
_COM_SMARTPTR_TYPEDEF(ITypeInfo2, __uuidof(ITypeInfo2));
_COM_SMARTPTR_TYPEDEF(StdOle, __uuidof(StdOle));
_COM_SMARTPTR_TYPEDEF(IRichEditOle, __uuidof(IRichEditOle));
_COM_SMARTPTR_TYPEDEF(IRichEditOleCallback, __uuidof(IRichEditOleCallback));
_COM_SMARTPTR_TYPEDEF(IEnumGUID, __uuidof(IEnumGUID));
_COM_SMARTPTR_TYPEDEF(IEnumCATEGORYINFO, __uuidof(IEnumCATEGORYINFO));
_COM_SMARTPTR_TYPEDEF(ICatRegister, __uuidof(ICatRegister));
_COM_SMARTPTR_TYPEDEF(ICatInformation, __uuidof(ICatInformation));
_COM_SMARTPTR_TYPEDEF(IServiceProvider, __uuidof(IServiceProvider));
_COM_SMARTPTR_TYPEDEF(IPersistPropertyBag, __uuidof(IPersistPropertyBag));
_COM_SMARTPTR_TYPEDEF(IPropertyBag, __uuidof(IPropertyBag));
_COM_SMARTPTR_TYPEDEF(IErrorLog, __uuidof(IErrorLog));
_COM_SMARTPTR_TYPEDEF(IBoundObject, __uuidof(IBoundObject));
_COM_SMARTPTR_TYPEDEF(IBoundObjectSite, __uuidof(IBoundObjectSite));
_COM_SMARTPTR_TYPEDEF(IPropertyNotifySink, __uuidof(IPropertyNotifySink));
_COM_SMARTPTR_TYPEDEF(IPropertyPage2, __uuidof(IPropertyPage2));
_COM_SMARTPTR_TYPEDEF(IPerPropertyBrowsing, __uuidof(IPerPropertyBrowsing));
_COM_SMARTPTR_TYPEDEF(IFillLockBytes, __uuidof(IFillLockBytes));
_COM_SMARTPTR_TYPEDEF(IFont, __uuidof(IFont));
_COM_SMARTPTR_TYPEDEF(IFontDisp, __uuidof(IFontDisp));
_COM_SMARTPTR_TYPEDEF(IPicture, __uuidof(IPicture));
_COM_SMARTPTR_TYPEDEF(IPictureDisp, __uuidof(IPictureDisp));
_COM_SMARTPTR_TYPEDEF(IOleInPlaceObjectWindowless, __uuidof(IOleInPlaceObjectWindowless));
_COM_SMARTPTR_TYPEDEF(IBinding, __uuidof(IBinding));
_COM_SMARTPTR_TYPEDEF(IBindStatusCallback, __uuidof(IBindStatusCallback));
_COM_SMARTPTR_TYPEDEF(IHlinkSite, __uuidof(IHlinkSite));
_COM_SMARTPTR_TYPEDEF(IHlink, __uuidof(IHlink));
_COM_SMARTPTR_TYPEDEF(IHlinkTarget, __uuidof(IHlinkTarget));
_COM_SMARTPTR_TYPEDEF(IHlinkFrame, __uuidof(IHlinkFrame));
_COM_SMARTPTR_TYPEDEF(IEnumHLITEM, __uuidof(IEnumHLITEM));
_COM_SMARTPTR_TYPEDEF(IHlinkBrowseContext, __uuidof(IHlinkBrowseContext));
_COM_SMARTPTR_TYPEDEF(IPersistMoniker, __uuidof(IPersistMoniker));
_COM_SMARTPTR_TYPEDEF(IBindStatusCallbackMsg, __uuidof(IBindStatusCallbackMsg));
_COM_SMARTPTR_TYPEDEF(IBindProtocol, __uuidof(IBindProtocol));
_COM_SMARTPTR_TYPEDEF(IAuthenticate, __uuidof(IAuthenticate));
_COM_SMARTPTR_TYPEDEF(ICodeInstall, __uuidof(ICodeInstall));
_COM_SMARTPTR_TYPEDEF(IHttpNegotiate, __uuidof(IHttpNegotiate));
_COM_SMARTPTR_TYPEDEF(IAsyncMoniker, __uuidof(IAsyncMoniker));
_COM_SMARTPTR_TYPEDEF(IWindowForBindingUI, __uuidof(IWindowForBindingUI));
_COM_SMARTPTR_TYPEDEF(IWinInetInfo, __uuidof(IWinInetInfo));
_COM_SMARTPTR_TYPEDEF(IHttpSecurity, __uuidof(IHttpSecurity));
_COM_SMARTPTR_TYPEDEF(IWinInetHttpInfo, __uuidof(IWinInetHttpInfo));
_COM_SMARTPTR_TYPEDEF(IErrorInfo, __uuidof(IErrorInfo));
_COM_SMARTPTR_TYPEDEF(ICreateErrorInfo, __uuidof(ICreateErrorInfo));
_COM_SMARTPTR_TYPEDEF(ISupportErrorInfo, __uuidof(ISupportErrorInfo));
_COM_SMARTPTR_TYPEDEF(IActiveScriptSiteWindow, __uuidof(IActiveScriptSiteWindow));
_COM_SMARTPTR_TYPEDEF(IObjectSafety, __uuidof(IObjectSafety));
_COM_SMARTPTR_TYPEDEF(IActiveScriptSite, __uuidof(IActiveScriptSite));
_COM_SMARTPTR_TYPEDEF(IActiveScriptError, __uuidof(IActiveScriptError));
_COM_SMARTPTR_TYPEDEF(IActiveScript, __uuidof(IActiveScript));
_COM_SMARTPTR_TYPEDEF(IActiveScriptParse, __uuidof(IActiveScriptParse));
_COM_SMARTPTR_TYPEDEF(ITargetFrame, __uuidof(ITargetFrame));
_COM_SMARTPTR_TYPEDEF(ISimpleFrameSite, __uuidof(ISimpleFrameSite));
_COM_SMARTPTR_TYPEDEF(IProgressNotify, __uuidof(IProgressNotify));
_COM_SMARTPTR_TYPEDEF(ILayoutStorage, __uuidof(ILayoutStorage));
_COM_SMARTPTR_TYPEDEF(ITargetEmbedding, __uuidof(ITargetEmbedding));
_COM_SMARTPTR_TYPEDEF(IChannelHook, __uuidof(IChannelHook));
_COM_SMARTPTR_TYPEDEF(IOleUndoUnit, __uuidof(IOleUndoUnit));
_COM_SMARTPTR_TYPEDEF(IOleParentUndoUnit, __uuidof(IOleParentUndoUnit));
_COM_SMARTPTR_TYPEDEF(IEnumOleUndoUnits, __uuidof(IEnumOleUndoUnits));
_COM_SMARTPTR_TYPEDEF(IOleUndoManager, __uuidof(IOleUndoManager));
_COM_SMARTPTR_TYPEDEF(IProvideClassInfo2, __uuidof(IProvideClassInfo2));
_COM_SMARTPTR_TYPEDEF(IAdviseSinkEx, __uuidof(IAdviseSinkEx));
_COM_SMARTPTR_TYPEDEF(IViewObjectEx, __uuidof(IViewObjectEx));
_COM_SMARTPTR_TYPEDEF(IBindHost, __uuidof(IBindHost));
_COM_SMARTPTR_TYPEDEF(IObjectWithSite, __uuidof(IObjectWithSite));
_COM_SMARTPTR_TYPEDEF(IOleDocument, __uuidof(IOleDocument));
_COM_SMARTPTR_TYPEDEF(IOleDocumentView, __uuidof(IOleDocumentView));
_COM_SMARTPTR_TYPEDEF(IOleDocumentSite, __uuidof(IOleDocumentSite));
_COM_SMARTPTR_TYPEDEF(IEnumOleDocumentViews, __uuidof(IEnumOleDocumentViews));
_COM_SMARTPTR_TYPEDEF(IPrint, __uuidof(IPrint));
_COM_SMARTPTR_TYPEDEF(IContinueCallback, __uuidof(IContinueCallback));
_COM_SMARTPTR_TYPEDEF(IOleCommandTarget, __uuidof(IOleCommandTarget));
_COM_SMARTPTR_TYPEDEF(IROTData, __uuidof(IROTData));
_COM_SMARTPTR_TYPEDEF(ICursor, __uuidof(ICursor));
_COM_SMARTPTR_TYPEDEF(ICursorMove, __uuidof(ICursorMove));
_COM_SMARTPTR_TYPEDEF(ICursorScroll, __uuidof(ICursorScroll));
_COM_SMARTPTR_TYPEDEF(ICursorUpdateARow, __uuidof(ICursorUpdateARow));
_COM_SMARTPTR_TYPEDEF(INotifyDBEvents, __uuidof(INotifyDBEvents));
_COM_SMARTPTR_TYPEDEF(ICursorFind, __uuidof(ICursorFind));
_COM_SMARTPTR_TYPEDEF(IEntryID, __uuidof(IEntryID));
_COM_SMARTPTR_TYPEDEF(ISequentialStream, __uuidof(ISequentialStream));
_COM_SMARTPTR_TYPEDEF(IPersistStreamInit, __uuidof(IPersistStreamInit));
_COM_SMARTPTR_TYPEDEF(IOverlappedStream, __uuidof(IOverlappedStream));
_COM_SMARTPTR_TYPEDEF(IOverlappedCompletion, __uuidof(IOverlappedCompletion));
_COM_SMARTPTR_TYPEDEF(IPSFactoryBuffer, __uuidof(IPSFactoryBuffer));
_COM_SMARTPTR_TYPEDEF(IRpcProxyBuffer, __uuidof(IRpcProxyBuffer));
_COM_SMARTPTR_TYPEDEF(IRpcStubBuffer, __uuidof(IRpcStubBuffer));
_COM_SMARTPTR_TYPEDEF(IRpcChannelBuffer, __uuidof(IRpcChannelBuffer));
_COM_SMARTPTR_TYPEDEF(IServerHandler, __uuidof(IServerHandler));
_COM_SMARTPTR_TYPEDEF(IClientSiteHandler, __uuidof(IClientSiteHandler));
_COM_SMARTPTR_TYPEDEF(IOleInPlaceSiteWindowless, __uuidof(IOleInPlaceSiteWindowless));
_COM_SMARTPTR_TYPEDEF(IOleInPlaceSiteEx, __uuidof(IOleInPlaceSiteEx));
_COM_SMARTPTR_TYPEDEF(IPointerInactive, __uuidof(IPointerInactive));
_COM_SMARTPTR_TYPEDEF(IProvideClassInfo, __uuidof(IProvideClassInfo));
_COM_SMARTPTR_TYPEDEF(IConnectionPointContainer, __uuidof(IConnectionPointContainer));
_COM_SMARTPTR_TYPEDEF(IEnumConnectionPoints, __uuidof(IEnumConnectionPoints));
_COM_SMARTPTR_TYPEDEF(IConnectionPoint, __uuidof(IConnectionPoint));
_COM_SMARTPTR_TYPEDEF(IEnumConnections, __uuidof(IEnumConnections));
_COM_SMARTPTR_TYPEDEF(IOleControl, __uuidof(IOleControl));
_COM_SMARTPTR_TYPEDEF(IOleControlSite, __uuidof(IOleControlSite));
_COM_SMARTPTR_TYPEDEF(IPropertyFrame, __uuidof(IPropertyFrame));
_COM_SMARTPTR_TYPEDEF(ISpecifyPropertyPages, __uuidof(ISpecifyPropertyPages));
_COM_SMARTPTR_TYPEDEF(IPropertyPageSite, __uuidof(IPropertyPageSite));
_COM_SMARTPTR_TYPEDEF(IPropertyPage, __uuidof(IPropertyPage));
_COM_SMARTPTR_TYPEDEF(IClassFactory2, __uuidof(IClassFactory2));
_COM_SMARTPTR_TYPEDEF(IMimeInfo, __uuidof(IMimeInfo));
_COM_SMARTPTR_TYPEDEF(IFilter, __uuidof(IFilter));
_COM_SMARTPTR_TYPEDEF(IPersistMemory, __uuidof(IPersistMemory));
_COM_SMARTPTR_TYPEDEF(IUrlHistoryStg, __uuidof(IUrlHistoryStg));
_COM_SMARTPTR_TYPEDEF(IEnumSTATURL, __uuidof(IEnumSTATURL));
_COM_SMARTPTR_TYPEDEF(IQuickActivate, __uuidof(IQuickActivate));

#endif	/* _COM_NO_STANDARD_GUIDS_ */

#endif  /* _INC_COMDEF */
