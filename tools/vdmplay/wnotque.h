#ifndef __WNOTQUE_H__
#define __WNOTQUE_H__


#ifdef _WIN32
#include <timeapi.h>   // timeGetTime — win32_compat provides it on POSIX
#endif
#include "stdafx.h"

struct NTLink : public CVPLink {
    CNotification *m_data;

    NTLink(CNotification *n) : m_data(n) {}

};

declare(VPList, NTLink);

typedef VPList(NTLink) NTList;

declare2(VPListIterator, NTList, NTLink);

// Manage the queu of notifications to the client application, handle the Windows queue overflow
class CWinNotifyQueue {
public:
    void Add(CNotification *n) { m_list.Append(new NTLink(n)); }

    unsigned Count() const { return m_list.Count(); }

    CNotification *Get() {
        if (!Count()) return NULL;
        NTLink *d = m_list.First();
        m_list.Remove(d);
        CNotification *n = d->m_data;
        delete d;
        return n;
    }

    virtual ~CWinNotifyQueue() {
        while (NULL != Get()) {
        }
    }

    virtual CNotification *RecoverNotification(LPCVPMESSAGE msg) {
        return CNotification::ContainingObject(msg);
    }

    CWinNotifyQueue(UINT msg, HWND win = NULL) : m_msgCode(msg), m_window(win) {}

    void RetryPosting() {
        if (!Count())
            return;

        NTLink *nl;

        while (NULL != (nl = m_list.First())) {
            CNotification *n = nl->m_data;
            n->m_vpmsg.postTime = timeGetTime();
            if (!PostMessage((WPARAM) n->m_vpmsg.notificationCode, (LPARAM) &n->m_vpmsg))
                break;


            Get();
        }

    }

    // EN_VPNQ=1: notification-lifecycle trace (post / dispatch / ack-delete) —
    // diffing the three streams exposes a UAF as dispatch-after-delete or a
    // double-dispatch of one pointer (mac SIGSEGV hunt, 2026-07-01, 8 crashes).
    static bool VpnqLogOn() {
        static int on = -1;
        if (on < 0) { const char* e = getenv("EN_VPNQ"); on = (e && *e && *e != '0') ? 1 : 0; }
        return on == 1;
    }

    void PostNotification(CNotification *n) {
        if (VpnqLogOn())
            fprintf(stderr, "[vpnq] post   n=%p vpmsg=%p code=%u u.data=%p\n",
                    (void*)n, (void*)&n->m_vpmsg, (unsigned)n->m_vpmsg.notificationCode,
                    (void*)n->m_vpmsg.u.data);
        if (!m_window) // no window to send the notification so simulate its completion
        {
            if (VpnqLogOn())
                fprintf(stderr, "[vpnq] nowin-complete-delete n=%p\n", (void*)n);
            n->Complete();
            delete n;
            return;
        }

        n->m_vpmsg.recTime = timeGetTime();

        if (Count()) // the pending notifcation queue is not empty, append this one to the end
        {
            Add(n);
            RetryPosting();
            // DO NOT fall through (2026-07-01): RetryPosting may already have posted n's
            // m_vpmsg to the window; falling through posted it a SECOND time. The app
            // acknowledges the first delivery (vpAcknowledge -> Complete + DELETE the
            // notification, Unref'ing the genericMsg), so the second WM_VPNOTIFY dispatch
            // handed the app a FREED m_vpmsg/data buffer -> use-after-free garbage decoded
            // as a game command. Cross-platform: killed mac clients 6/6 (SIGSEGV in
            // CMsgVehNew::AssertValid, always during notification bursts = right after a
            // SenumREP broadcast made the queue momentarily non-empty) AND the Windows
            // host (same site, AV 0xC0000005) in the first 3-platform MP game.
            return;
        }

        n->m_vpmsg.postTime = timeGetTime();
        if (!PostMessage((WPARAM) n->m_vpmsg.notificationCode, (LPARAM) &n->m_vpmsg)) {
            Add(n);
        }
    }


    virtual BOOL  PostMessage(WPARAM wParam, LPARAM lParam) {
        return ::PostMessage(m_window, m_msgCode, wParam, lParam);
    }

public:

    NTList m_list;
    HWND m_window;
    UINT m_msgCode;
};


#endif  

