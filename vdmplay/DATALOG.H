#ifndef __DATALOG_H__
#define __DATALOG_H__


class CDataLogger
{
public:
	typedef void (*Callback)(LPVOID);


	CDataLogger() : m_cancelCbk(NULL), m_data(NULL) { }
	virtual ~CDataLogger() { }



	// Show the window
	virtual void Start(char const * pPort) { }

	// Close the window
	virtual void Stop() {}

	// Show the status
	virtual void Status (int iStat) {}

	// show the number being dialed
	virtual void Number (char const *psNum) {}

	// Show the outgoing data
	virtual void OutgoingData(LPCSTR data, unsigned size) { }

	// Show the incoming data
	virtual void IncomingData(LPCSTR data, unsigned size) { }
			
	// Initialize the cancel callback	
	void SetCallback(Callback p, LPVOID data) { m_cancelCbk = p; m_data = data; }


	// should be called whe user press the Cancel button
	void OnCancel()
	{
		if (m_cancelCbk)
			m_cancelCbk(m_data);
	}

	Callback m_cancelCbk;
	LPVOID   m_data;
};


class CComStatDlg;
class CDialogLogger : public CDataLogger
{
public:
	typedef void (*Callback)(LPVOID);


	CDialogLogger ();
	virtual ~CDialogLogger ();



	// Show the window
	virtual void Start(char const * pPort);

	// Close the window
	virtual void Stop();

	// Show the status
	virtual void Status (int iStat);

	// show the number being dialed
	virtual void Number (char const *psNum);

	// Show the outgoing data
	virtual void OutgoingData(LPCSTR data, unsigned size);

	// Show the incoming data
	virtual void IncomingData(LPCSTR data, unsigned size) { OutgoingData (data, size); }
#ifdef xxWIN32			
	HWND			m_hDlg;
#else
	CComStatDlg*    m_dlg;
#endif
};


#endif
