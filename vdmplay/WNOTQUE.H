#ifndef __WNOTQUE_H__
#define __WNOTQUE_H__


struct NTLink: public CVPLink
{
     CNotification*  m_data;
	 NTLink(CNotification* n) : m_data(n) { }
	 
};

declare(VPList, NTLink);
typedef VPList(NTLink) NTList;
declare2(VPListIterator, NTList, NTLink);

// Manage the queu of notifications to the client application, handle the Windows queue overflow
class CWinNotifyQueue
{
public:
		void Add(CNotification* n)  { m_list.Append(new NTLink(n)); }
		unsigned Count() const { return m_list.Count(); }
		CNotification* Get() 
		{
			if (!Count()) return NULL;
			NTLink* d = m_list.First();
			m_list.Remove(d);
			CNotification* n = d->m_data;
			delete d;
			return n;
		}
		
		virtual ~CWinNotifyQueue()
		{
			while(NULL != Get())
			{
			}
		}
		
		virtual CNotification* RecoverNotification(LPCVPMESSAGE msg)
		{
			return CNotification::ContainingObject(msg);
		}
		
		CWinNotifyQueue(UINT msg, HWND win=NULL) : m_msgCode(msg), m_window(win) { }

		void RetryPosting()
		{
			if (!Count())
				return;

			NTLink* nl;
			
			while(NULL != (nl = m_list.First()))
			{
				CNotification*n =  nl->m_data;
				n->m_vpmsg.postTime = timeGetTime();
				if (!PostMessage((WPARAM) n->m_vpmsg.notificationCode, (LPARAM) &n->m_vpmsg))
					break;

				
				Get();
			}
			 	
		}

		void PostNotification(CNotification* n)
		{
        	if (!m_window) // no window to send the notification so simulate its completion 
			{
				n->Complete();
				delete n;   
				return;
			}

			n->m_vpmsg.recTime = timeGetTime();

			if (Count()) // the pending notifcation queue is not empty, append this one to the end 
			{
				Add(n);
				RetryPosting();
			}

			n->m_vpmsg.postTime = timeGetTime();
			if (!PostMessage((WPARAM) n->m_vpmsg.notificationCode, (LPARAM) &n->m_vpmsg))
			{
				Add(n);
			}
		}

	    
        virtual BOOL  PostMessage(WPARAM wParam, LPARAM lParam)
        {
        	return ::PostMessage(m_window, m_msgCode, wParam, lParam);
        } 

public:

	   NTList  m_list;
	   HWND    m_window;
	   UINT    m_msgCode;
};


#endif  

