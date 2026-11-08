#include "stdafx.h"

#include "SDL2MFCPanel.h"
#include "GameWindow.h"
#include "SDL2Compositor.h"
#include "SDL2Panel.h"
#include "lastplnt.h"

#include <SDL.h>
#include <vector>

#ifndef PW_RENDERFULLCONTENT
#define PW_RENDERFULLCONTENT 0x00000002
#endif

std::vector<SDL2MFCPanel::Entry>& SDL2MFCPanel::GetEntries() {
    static std::vector<Entry> s_entries;
    return s_entries;
}

SDL2Panel* SDL2MFCPanel::Attach(CWnd* pWnd, const char* name, int zOrder) {
    if (!pWnd || !pWnd->m_hWnd || !theApp.m_gameWindow || !theApp.m_gameWindow->GetCompositor())
        return nullptr;

    // Check if already attached
    for (auto& e : GetEntries())
        if (e.pWnd == pWnd)
            return e.panel;

    CRect screenRect;
    pWnd->GetWindowRect(&screenRect);
    CRect clientRect;
    pWnd->GetClientRect(&clientRect);

    // Clamp position so title bar and resize borders stay on-screen
    int panelX = screenRect.left;
    int panelY = screenRect.top;
    if (panelX < SDL2Panel::RESIZE_BORDER)
        panelX = SDL2Panel::RESIZE_BORDER;
    int minY = SDL2Panel::TITLE_BAR_HT + SDL2Panel::RESIZE_BORDER;
    if (panelY < minY)
        panelY = minY;

    SDL2Panel* panel = theApp.m_gameWindow->GetCompositor()->AddPanel(
        name, panelX, panelY,
        clientRect.Width(), clientRect.Height(), zOrder);

    if (!panel)
        return nullptr;

    // Enable window management (drag, resize, close)
    panel->SetMovable(true);
    panel->SetResizable(true);
    panel->SetClosable(true);
    panel->SetTitle(name);

    // Close button sends WM_CLOSE to the MFC window
    CWnd* pW = pWnd;
    panel->SetCloseCallback([pW]() {
        if (pW && pW->m_hWnd)
            pW->PostMessage(WM_CLOSE);
    });

    // Set up event routing
    panel->SetEventCallback(
        [pW, panel](SDL_Event& event, int localX, int localY) -> bool {
            return SDL2MFCPanel::RouteEvent(pW, panel, event, localX, localY);
        });

    // Make MFC window transparent
    ::SetWindowLong(pWnd->m_hWnd, GWL_EXSTYLE,
        ::GetWindowLong(pWnd->m_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    ::SetLayeredWindowAttributes(pWnd->m_hWnd, 0, 1, LWA_ALPHA);

    GetEntries().push_back({pWnd, panel});
    return panel;
}

void SDL2MFCPanel::Detach(CWnd* pWnd) {
    auto& entries = GetEntries();
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        if (it->pWnd == pWnd) {
            if (theApp.m_gameWindow && theApp.m_gameWindow->GetCompositor())
                theApp.m_gameWindow->GetCompositor()->RemovePanel(it->panel);
            // Restore MFC window visibility
            if (pWnd->m_hWnd) {
                ::SetLayeredWindowAttributes(pWnd->m_hWnd, 0, 255, LWA_ALPHA);
            }
            entries.erase(it);
            return;
        }
    }
}

void SDL2MFCPanel::CaptureAll() {
    // Throttle all MFC panel captures to ~10fps to reduce flicker
    static DWORD s_lastCapture = 0;
    DWORD now = ::timeGetTime();
    if (now - s_lastCapture < 100)
        return;
    s_lastCapture = now;

    auto& entries = GetEntries();
    // Remove entries with destroyed windows
    for (auto it = entries.begin(); it != entries.end(); ) {
        if (!it->pWnd || !it->pWnd->m_hWnd || !::IsWindow(it->pWnd->m_hWnd)) {
            if (it->panel && theApp.m_gameWindow && theApp.m_gameWindow->GetCompositor())
                theApp.m_gameWindow->GetCompositor()->RemovePanel(it->panel);
            it = entries.erase(it);
        } else {
            CaptureOne(it->pWnd, it->panel);
            ++it;
        }
    }
}

void SDL2MFCPanel::CaptureOne(CWnd* pWnd, SDL2Panel* panel) {
    SDL_Surface* surface = panel->GetSurface();
    if (!surface)
        return;

    int w = panel->GetWidth();
    int h = panel->GetHeight();
    if (w <= 0 || h <= 0)
        return;

    // PW_RENDERFULLCONTENT captures layered windows at any alpha.
    // No need to toggle opacity — avoids flicker entirely.
    HDC hdcScreen = ::GetDC(NULL);
    HDC hdcMem = ::CreateCompatibleDC(hdcScreen);
    HBITMAP hBmp = ::CreateCompatibleBitmap(hdcScreen, w, h);
    HBITMAP hOld = (HBITMAP)::SelectObject(hdcMem, hBmp);

    ::PrintWindow(pWnd->m_hWnd, hdcMem, PW_RENDERFULLCONTENT);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    if (SDL_LockSurface(surface) == 0) {
        ::GetDIBits(hdcMem, hBmp, 0, h, surface->pixels, &bmi, DIB_RGB_COLORS);
        SDL_UnlockSurface(surface);
        panel->SetDirty();
    }

    ::SelectObject(hdcMem, hOld);
    ::DeleteObject(hBmp);
    ::DeleteDC(hdcMem);
    ::ReleaseDC(NULL, hdcScreen);
}

bool SDL2MFCPanel::RouteEvent(CWnd* pWnd, SDL2Panel* panel,
                               SDL_Event& event, int localX, int localY) {
    if (!pWnd || !pWnd->m_hWnd)
        return false;

    // Convert panel-local coords to MFC window client coords
    // (they should match since panel is positioned at window rect)
    LPARAM lParam = MAKELPARAM(localX, localY);

    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_LEFT) {
            // Find child at point and send click
            POINT pt = {localX, localY};
            HWND hChild = ::ChildWindowFromPointEx(pWnd->m_hWnd, pt,
                CWP_SKIPINVISIBLE | CWP_SKIPDISABLED);
            if (hChild && hChild != pWnd->m_hWnd) {
                // Convert to child-local coords
                RECT childRect;
                ::GetWindowRect(hChild, &childRect);
                RECT parentRect;
                ::GetWindowRect(pWnd->m_hWnd, &parentRect);
                int childX = localX - (childRect.left - parentRect.left);
                int childY = localY - (childRect.top - parentRect.top);
                ::SendMessage(hChild, WM_LBUTTONDOWN, 0, MAKELPARAM(childX, childY));
            } else {
                ::SendMessage(pWnd->m_hWnd, WM_LBUTTONDOWN, 0, lParam);
            }
        } else if (event.button.button == SDL_BUTTON_RIGHT) {
            ::SendMessage(pWnd->m_hWnd, WM_RBUTTONDOWN, 0, lParam);
        }
        return true;

    case SDL_MOUSEBUTTONUP:
        if (event.button.button == SDL_BUTTON_LEFT) {
            POINT pt = {localX, localY};
            HWND hChild = ::ChildWindowFromPointEx(pWnd->m_hWnd, pt,
                CWP_SKIPINVISIBLE | CWP_SKIPDISABLED);
            if (hChild && hChild != pWnd->m_hWnd) {
                RECT childRect;
                ::GetWindowRect(hChild, &childRect);
                RECT parentRect;
                ::GetWindowRect(pWnd->m_hWnd, &parentRect);
                int childX = localX - (childRect.left - parentRect.left);
                int childY = localY - (childRect.top - parentRect.top);
                ::SendMessage(hChild, WM_LBUTTONUP, 0, MAKELPARAM(childX, childY));
            } else {
                ::SendMessage(pWnd->m_hWnd, WM_LBUTTONUP, 0, lParam);
            }
        } else if (event.button.button == SDL_BUTTON_RIGHT) {
            ::SendMessage(pWnd->m_hWnd, WM_RBUTTONUP, 0, lParam);
        }
        return true;

    case SDL_MOUSEMOTION:
        ::SetCursor(::LoadCursor(NULL, IDC_ARROW));
        ::SendMessage(pWnd->m_hWnd, WM_MOUSEMOVE, 0, lParam);
        return true;

    case SDL_MOUSEWHEEL: {
        int delta = event.wheel.y * WHEEL_DELTA;
        ::SendMessage(pWnd->m_hWnd, WM_MOUSEWHEEL,
            MAKEWPARAM(0, delta), lParam);
        return true;
    }

    case SDL_KEYDOWN:
    case SDL_KEYUP: {
        // Forward keyboard to focused child
        HWND hFocus = ::GetFocus();
        if (!hFocus) hFocus = pWnd->m_hWnd;
        UINT msg = (event.type == SDL_KEYDOWN) ? WM_KEYDOWN : WM_KEYUP;
        // Simple scancode to VK mapping for common keys
        UINT vk = 0;
        switch (event.key.keysym.scancode) {
        case SDL_SCANCODE_RETURN: vk = VK_RETURN; break;
        case SDL_SCANCODE_ESCAPE: vk = VK_ESCAPE; break;
        case SDL_SCANCODE_TAB:    vk = VK_TAB; break;
        case SDL_SCANCODE_UP:     vk = VK_UP; break;
        case SDL_SCANCODE_DOWN:   vk = VK_DOWN; break;
        default: break;
        }
        if (vk)
            ::SendMessage(hFocus, msg, vk, 0);
        return vk != 0;
    }
    }

    return false;
}
