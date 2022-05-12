//
// TAPIUTILS.H
//
#include <tapi.h>


extern int (*dlrMessageBox)(LPCSTR msg);
extern void (*dlrDbgOutStr_p)(LPCSTR msg);
extern void (*dlrThrowMemoryException)();

#ifdef _DEBUG
#define dlrDbgOutStr(x) dlrDbgOutStr_p(x)
#else
#define dlrDbgOutStr(x)
#endif


#define INLINE_INHERIT_ASSIGNEMENT(cls, supercls, var) cls& operator=(const cls& var) \
{\
	supercls::operator=(var);\
	return *this;\
}

// Template helper class to manage variable size structures used by TAPI.
// Each such structure is supposed to have dwTotalSize field designating its total size
template<class T> class CSVarStruct
{
public:
	T* m_ptr;

protected:
	// assignement helpr routine
	void assign(const CSVarStruct& vs)
	{
		if (m_ptr)
		{
			LocalFree((HLOCAL)m_ptr);
		}

		if (!vs.m_ptr)
		{
			m_ptr = NULL;
			return;
		}
					
		UINT sz = LocalSize(vs.m_ptr);
		m_ptr = (T*) LocalAlloc(LPTR, sz);
		if (!m_ptr)
			dlrThrowMemoryException();
	
		memcpy(m_ptr, vs.m_ptr, sz);
	}

	// adopt a structure belonging to another CSVarStruct object
	void adopt(CSVarStruct& vs)
	{
		if (m_ptr)
		{
			LocalFree((HLOCAL)m_ptr);
		}

		m_ptr = vs.m_ptr;
		vs.m_ptr = NULL;
	}



public:
	// allocate the variable size structure of type T with additional 'addSize' bytes
	// of storage at the end
	CSVarStruct(UINT addSize=0)
	{
		m_ptr = (T*) LocalAlloc(LPTR | LMEM_ZEROINIT, sizeof(T)+addSize);
		if (!m_ptr)
			dlrThrowMemoryException();

		m_ptr->dwTotalSize = sizeof(T)+addSize;
	}


	CSVarStruct(const CSVarStruct& vs) :  m_ptr(NULL)
	{
		assign(vs);
	}

	CSVarStruct& operator=(const CSVarStruct& vs)
	{
		assign(vs);
		return *this;
	}


	~CSVarStruct()
	{
		if (m_ptr)
			LocalFree((HLOCAL) m_ptr);
	}


			
public:
	operator T*() { return m_ptr; }

	T* operator->() { return m_ptr; }

	LPSTR Contents(DWORD offset) const { return offset + (LPSTR) m_ptr; }
};


// Template helper class to manage variable size structures used by TAPI.
// The class support the dynamic reallocation
// Each such structure is supposed to have dwTotalSize and dwNeededSize fields
template<class T>  class CDVarStruct: public CSVarStruct<T>
{
public:
	CDVarStruct(UINT sz=256) : CSVarStruct<T>(sz) { }
	INLINE_INHERIT_ASSIGNEMENT(CDVarStruct, CSVarStruct<T>, vs)

	BOOL HasEnoughSpace() const
	{
		return m_ptr->dwNeededSize <= m_ptr->dwTotalSize;
	}

	void ReserveNeededSpace()
	{
		DWORD neededSize =  m_ptr->dwNeededSize;

		if (LocalSize((HLOCAL) m_ptr) < neededSize)
		{
		   LocalFree((HLOCAL) m_ptr);
		   m_ptr = (T*) LocalAlloc(LPTR|LMEM_ZEROINIT, neededSize);
		   if (!m_ptr)
		   		dlrThrowMemoryException();
		   m_ptr->dwTotalSize = neededSize;
		}
	}


};

// TAPI VARSTRING encapluslation class
class CVarString: public CDVarStruct<VARSTRING>
{
public:

	CVarString(UINT sz) : CDVarStruct<VARSTRING>(sz) 
	{ 
		m_ptr->dwStringOffset = sizeof(VARSTRING);
	}

	CVarString(LPCSTR buf, UINT len)  : CDVarStruct<VARSTRING>(len) 
	{
		m_ptr->dwStringOffset = sizeof(VARSTRING);
		m_ptr->dwStringSize = len;
		memcpy((LPSTR)Str(), buf, len);
	}

	ULONG Length() const { return m_ptr->dwStringSize; }
	LPCSTR Str() const { return Contents(m_ptr->dwStringOffset); }
	operator LPCSTR() const { return Contents(m_ptr->dwStringOffset); }
	operator LPSTR() { return Contents(m_ptr->dwStringOffset); }
	INLINE_INHERIT_ASSIGNEMENT(CVarString, CDVarStruct<VARSTRING>, vs)

	void Store(LPCSTR iniKey, LPCSTR section, LPCSTR iniFile);
	void Load(LPCSTR iniKey, LPCSTR section, LPCSTR iniFile); 

protected:
	enum { CHUNKSIZE=64 };
	void  StoreChunk(LPCSTR iniKey, LPCSTR section, LPCSTR inifile, int index, LPSTR keyBuf, LPSTR dataBuf);
	void  LoadChunk(LPCSTR iniKey, LPCSTR section, LPCSTR inifile, int index, LPSTR keyBuf, LPSTR dataBuf);

};

// LINE Device configuration
class CLineDevConfig: public CVarString
{
public:
	CLineDevConfig();
	CLineDevConfig(DWORD devId, LPCSTR devClass=NULL);
	CLineDevConfig(const CLineDevConfig& cf) : CVarString(cf) 
	{ 
	}
	
	CLineDevConfig(LPCSTR buf, UINT len) : CVarString(buf, len)
	{
	}

	
	INLINE_INHERIT_ASSIGNEMENT(CLineDevConfig, CVarString, cf)

	void* const Data() const { return (void*const) Str(); }

	// query the device configuration parameters
	void Query(DWORD devId, LPCSTR devClass=NULL);
	// show the configuartion dialog
	void Edit(DWORD devId, HWND hWndOwner=NULL, LPCSTR devClass=NULL);
	// apply the configuration parameters to the device
	void Set(DWORD devId, LPCSTR devClass = NULL);
};


class CTapiError
{
public:
	DWORD m_errCode;

	CTapiError(DWORD e = 0) : m_errCode(e) { }
};



// Encapsulation of the LINEDEVCAPS  (device capabilities) structure
class CLineDevCaps: public CDVarStruct<LINEDEVCAPS>
{
public:
	CLineDevCaps();	
	CLineDevCaps(HLINEAPP hLineApp, DWORD devId, DWORD& apiVersion);
	INLINE_INHERIT_ASSIGNEMENT(CLineDevCaps, CDVarStruct<LINEDEVCAPS>, caps)

	void Query(HLINEAPP hLineApp, DWORD devId, DWORD& apiVersion);

};

// Encapsulation of TAPI LINECALLPARAMS structure
class CLineCallParams: public CSVarStruct<LINECALLPARAMS>
{
public:

	CLineCallParams(LPCSTR callAddress = NULL, 
					DWORD bearerMode = LINEBEARERMODE_VOICE, 
					DWORD mediaMode = LINEMEDIAMODE_DATAMODEM,
					DWORD addressMode = LINEADDRESSMODE_ADDRESSID,
					DWORD flags = LINECALLPARAMFLAGS_IDLE);

	INLINE_INHERIT_ASSIGNEMENT(CLineCallParams, CSVarStruct<LINECALLPARAMS>, cp)


};

// Encapsulation of the TAPI LINEADDRESSCAPS structure
class CLineAddressCaps: public CDVarStruct<LINEADDRESSCAPS>
{
public:

	CLineAddressCaps();
	CLineAddressCaps(HLINEAPP app, DWORD devId, DWORD addrId, DWORD apiV, DWORD extV=0);

	INLINE_INHERIT_ASSIGNEMENT(CLineAddressCaps, CDVarStruct<LINEADDRESSCAPS>, ap)


	void Query(HLINEAPP app, DWORD devId, DWORD addrId, DWORD apiV, DWORD extV=0);
};

// Encapsulation of the TAPI LINEDEVSTATUS structure
class CLineDevStatus: public CDVarStruct<LINEDEVSTATUS>
{
public:
	  CLineDevStatus();
	  CLineDevStatus(HLINE hLine);

	  INLINE_INHERIT_ASSIGNEMENT(CLineDevStatus, CDVarStruct<LINEDEVSTATUS>, ap)

	  void Query(HLINE hLine);
};

class CLineDeviceId: public CVarString
{

public:
	CLineDeviceId() : CVarString(128) { }

	CLineDeviceId(HLINE hLine, DWORD addrId, HCALL hCall, DWORD select, LPCSTR devClass) : 
		CVarString(16)
	{
		Query(hLine, addrId, hCall, select, devClass);
	}


	void Query(HLINE hLine, DWORD addrId, HCALL hCall, DWORD select, LPCSTR devClass);

};


class CCallDeviceId: public CLineDeviceId
{

public:
	CCallDeviceId() : CLineDeviceId() { }

	CCallDeviceId(HCALL hCall, LPCSTR devClass="comm/datamodem")
	{
		Query(hCall, devClass);
	}

	
	void Query(HCALL hCall, LPCSTR devClass="comm/datamodem")
	{
		CLineDeviceId::Query(0, 0, hCall, LINECALLSELECT_CALL, devClass);
	}

	HANDLE CommHandle()
	{
		return *((LPHANDLE) Str());
	}
};




class CTapiConnection
{

 protected:
    // this area contains the protected members of the CTapiConnection class
    DWORD m_dwNumDevs;   // the number of line devices available
    DWORD m_dwDeviceID;  // the device ID
    DWORD m_dwRequestedID;
    LONG m_lAsyncReply;


    // BOOLEANS to handle reentrancy
    BOOL m_bShuttingDown;
    BOOL m_bStoppingCall;
    BOOL m_bInitializing;
    BOOL m_bReplyReceived;
    

    BOOL m_bTapiInUse;   // whether TAPI is in use or not
    BOOL m_bInitialized; // whether TAPI has been initialized
	BOOL m_bConnected;

	HANDLE m_hComm;

	HINSTANCE  m_hInst;

	BOOL	   m_answered;
	BOOL	   m_dialing;
 public:
    // this area contains the public members of the CTapiConnection class
    HLINEAPP m_hLineApp; // the usage handle of this application for TAPI
    HCALL m_hCall;       // handle to the call
    HLINE m_hLine;       // handle to the open line
    DWORD m_dwAPIVersion;   // the API version
    char m_szPhoneNumber[64];  // the phone number to call
	CLineDevConfig m_cfgInfo;


 protected:
    // Here is where I put the protected (internal) functions
    BOOL ShutdownTAPI();
    BOOL HandleLineErr(long lLineErr);

#if 0
    LPLINEDEVCAPS GetDeviceLine(DWORD *dwAPIVersion, LPLINEDEVCAPS lpLineDevCaps);
    LPLINEDEVCAPS MylineGetDevCaps(LPLINEDEVCAPS lpLineDevCaps,
        DWORD dwDeviceID, DWORD dwAPIVersion);
    LPVOID CheckAndReAllocBuffer(LPVOID lpBuffer, size_t sizeBufferMinimum);
    LPLINEADDRESSCAPS MylineGetAddressCaps (LPLINEADDRESSCAPS lpLineAddressCaps,
        DWORD dwDeviceID, DWORD dwAddressID,DWORD dwAPIVersion, DWORD dwExtVersion);
    LPLINECALLPARAMS CreateCallParams (LPLINECALLPARAMS lpCallParams, LPCSTR lpszDisplayableAddress);
#endif
    BOOL MakeTheCall(LPLINEDEVCAPS lpLineDevCaps,LPCSTR lpszAddress);


    long WaitForReply (long lRequestID);
    void HandleLineCallState(DWORD dwDevice, DWORD dwMessage, DWORD dwCallbackInstance,
        DWORD dwParam1, DWORD dwParam2, DWORD dwParam3);



	BOOL InitLine(CLineDevCaps& devCaps);
	BOOL OpenLine(BOOL answer);

	BOOL lowLevelOpen(DWORD privelege, DWORD mediaMode);

	virtual void OnConnect() { }
	virtual void OnDisconnect() { }
	virtual void OnDialFail() { }

 private:
   // This section is for private functions

 public:
    // Public functions

	typedef void (*DevEnumProc)(LPVOID ctx, LPCSTR devName, UINT devId);


	BOOL Connected() const { return m_bConnected; }
	HANDLE CommHandle() 
	{ 
		HANDLE h = m_hComm;
		m_hComm = NULL;
		return h;
	}
	DWORD  DeviceId() const { return m_dwDeviceID; }

	void GetDeviceName(LPSTR devName);

    CTapiConnection();   
    ~CTapiConnection();
    
    BOOL EnumDevices(DevEnumProc proc, LPVOID ctx);
	BOOL SelectDevice(UINT devId, LPCSTR devName=NULL);

    BOOL Create(HINSTANCE hInst, char *szPhoneNumber = NULL, BOOL quiet = FALSE);
    BOOL DialCall(LPCSTR szPhoneNumber = NULL);
	BOOL WaitForCall();
    BOOL HangupCall();

    static void CALLBACK lineCallbackFunc(
        DWORD dwDevice, DWORD dwMsg, DWORD dwCallbackInstance, 
        DWORD dwParam1, DWORD dwParam2, DWORD dwParam3);

	void SetConfig(CLineDevConfig& cf); 

	void LoadConfig(LPCSTR iniFile, LPCSTR section, BOOL fullLoad=FALSE);
	void StoreConfig(LPCSTR iniFile, LPCSTR section);


};




