#ifndef ___DAVENET_H__
#define ___DAVENET_H__

#include "davenet.h"

#define _FP_SEG(fp) (*((unsigned __far *)&(fp)+1))

typedef struct _MYNCB         /* NetBIOS command block */
{
    NCB			ncb;
		NETMSG	msg;
} MYNCB;


class CProtocol {
public:
						CProtocol () {}
		virtual ~CProtocol ();

		virtual BOOL	InitOk () = 0;

		virtual void	ErrMsgBox (NETMSG * pMsg) = 0;
		virtual void	Close () = 0;
		virtual BOOL	AddName (LPCSTR pName, LPCVOID pData) = 0;
		virtual BOOL	AddGroupName (LPCSTR pName, LPCVOID pData) = 0;
		virtual BOOL	Call (LPCSTR pLocal, LPCSTR pRemote, LPCVOID pUser) = 0;
		virtual void	CancelReceive (int iNum) = 0;
		virtual void	CancelReceiveDatagram (int iNum) = 0;
		virtual BOOL	DeleteName (LPCSTR pName, LPCVOID pData) = 0;
		virtual BOOL	HangUp (int iNum, LPCVOID pUser) = 0;
		virtual BOOL	Listen (LPCSTR pLocal, LPCSTR pRemote, LPCVOID pUser) = 0;
		virtual BOOL	Receive (int iNum, LPCVOID pUser) = 0;
		virtual BOOL	ReceiveDatagram (int iNum, LPCVOID pUser) = 0;
		virtual BOOL	Send (int iNum, LPCVOID pData, int iLen, LPCVOID pUser) = 0;
		virtual BOOL	SendDatagram (int iNum, LPCSTR pName, LPCVOID pData, int iLen, LPCVOID pUser) = 0;

protected:
		void				MsgBox (int iRes);
		void				MsgBox (char const *pErr);

		HINSTANCE		m_hInst;
		HWND				m_hWnd;
		BYTE				m_ID;
};

const int NUM_NCBS = 62;
class CNetbios : public CProtocol {
public:
		CNetbios (HINSTANCE hInst, HWND hWnd);
		~CNetbios ();

		static BOOL Have ();
		BOOL	InitOk ();

		void	ErrMsgBox (NETMSG * pMsg);
		void	Close ();
		BOOL	AddName (LPCSTR pName, LPCVOID pData);
		BOOL	AddGroupName (LPCSTR pName, LPCVOID pData);
		BOOL	Call (LPCSTR pLocal, LPCSTR pRemote, LPCVOID pUser);
		void	CancelReceive (int iNum);
		void	CancelReceiveDatagram (int iNum);
		BOOL	DeleteName (LPCSTR pName, LPCVOID pData);
		BOOL	HangUp (int iNum, LPCVOID pUser);
		BOOL	Listen (LPCSTR pLocal, LPCSTR pRemote, LPCVOID pUser);
		BOOL	Receive (int iNum, LPCVOID pUser);
		BOOL	ReceiveDatagram (int iNum, LPCVOID pUser);
		BOOL	Send (int iNum, LPCVOID pData, int iLen, LPCVOID pUser);
		BOOL	SendDatagram (int iNum, LPCSTR pName, LPCVOID pData, int iLen, LPCVOID pUser);

		static BYTE	m_iLanaNum;

protected:
		void		NetbiosMsgBox (BYTE cErr);
		MYNCB *	GetNcb ();
		void		SetName (PUCHAR pDest, LPCSTR pSrc);

		MYNCB *			m_pNcb;
		HGLOBAL			m_hMem;
};

#endif
