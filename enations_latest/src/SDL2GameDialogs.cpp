#include "stdafx.h"

#include "SDL2GameDialogs.h"
#include "SDL2Panel.h"
#include "SDL2Compositor.h"
#include "SDL2MainMenu.h"
#include "GameWindow.h"
#include "bitmaps.h"
#include "lastplnt.h"
#include "player.h"
#include "research.h"
#include "relation.h"
#include "building.inl"
#include "vehicle.inl"
#include "unit.inl"
#include "terrain.inl"  // CHexCoord/CMapLoc inlines (needed at /Ob2)
#include "area.h"
#include "netcmd.h"
#include "ipcmsg.hpp"   // CMsgIPC ??? email/chat network message (wire format)
#include "event.h"      // EVENT_HAVE_MAIL / EVENT_NOTIFY ??? "you have mail" cue
#include "base.h"
#include "icons.h"      // theIcons / ICON_RESEARCH ??? flask progress sprite

#include <SDL_ttf.h>
#include <vector>
#include <sstream>
#include <locale>
#include <iomanip>
#include <string>

// ---------------------------------------------------------------------------
// Chat message store ??? receives incoming cmd_chat network messages.
// SDL2Chat_AddMessage() is called from netapi.cpp; SDL2ChatWindow reads it.
// ---------------------------------------------------------------------------
static std::vector<std::string> g_chatMessages;

void SDL2Chat_AddMessage(const std::string& line) {
    g_chatMessages.push_back(line);
    // Cap history at 200 lines so the listbox stays reasonable
    if (g_chatMessages.size() > 200)
        g_chatMessages.erase(g_chatMessages.begin());
}

// Clear the chat backlog. Called when entering a network room (host-create /
// client-join) so a new room starts with an empty chat instead of carrying over
// the previous session's history (operator: chat should clear on leave/join).
void SDL2Chat_Clear() { g_chatMessages.clear(); }

int SDL2Chat_Count() { return (int)g_chatMessages.size(); }

std::string SDL2Chat_Line(int i) {
    return (i >= 0 && i < (int)g_chatMessages.size()) ? g_chatMessages[i] : std::string();
}

void SDL2Chat_Send(const std::string& text) {
    if (text.empty()) return;
    CPlayer* pMe = theGame.GetMe();
    std::string from = pMe ? (const char*)pMe->GetName() : "Me";

    if (theGame.IsNetGame() && theGame.GetMyNetNum() != 0 && pMe) {
        // Broadcast to the other players only (Broadcast doesn't loop back to us,
        // PostToAll adds the local copy separately) and echo locally ??? so chat
        // works even pre-game where the message queue isn't being drained.
        CNetChat* pMsg = CNetChat::Alloc(pMe, text.c_str());
        theNet.Broadcast(pMsg, pMsg->m_iLen, TRUE);
        delete[] pMsg;
    }
    SDL2Chat_AddMessage(from + ": " + text);
}

// ---------------------------------------------------------------------------
// Mail inbox ??? received email messages. Replaces CWndComm's CEMsgList; the
// SDL2MailDialog reads it. SDL2Mail_HandleIncoming() is called from netapi.cpp
// when an ipc_msg network command arrives.
// ---------------------------------------------------------------------------
struct SDL2MailMsg {
    int         fromPlyr = -1;
    std::string fromName;
    std::string subject;
    std::string body;
    bool        read = false;
};
static std::vector<SDL2MailMsg> g_mailInbox;

int SDL2Mail_UnreadCount() {
    int n = 0;
    for (const auto& m : g_mailInbox) if (!m.read) n++;
    return n;
}

void SDL2Mail_HandleIncoming(CMsgIPC* pMsg) {
    if (!pMsg) return;

    // The wire buffer is laid out by CMsgIPC::ToBuf as:
    //   [ sizeof(CMsgIPC) bytes of the struct ][ message\0 ][ subject\0 ]
    // The scalar header fields (m_iType/m_iFrom/m_iTo) are valid in place; the
    // CString members in the header are garbage pointers and must NOT be touched
    // ??? the actual text lives in the appended tail.
    int type = pMsg->m_iType;
    int from = pMsg->m_iFrom;
    int to   = pMsg->m_iTo;

    const char* tail   = reinterpret_cast<const char*>(pMsg) + sizeof(CMsgIPC);
    std::string message = tail;
    std::string subject = tail + message.size() + 1;

    // Only process mail actually addressed to us.
    CPlayer* pMe = theGame.GetMe();
    if (pMe && to != pMe->GetPlyrNum())
        return;

    CPlayer* pFrom = theGame.GetPlayerByPlyr(from);
    std::string fromName = pFrom ? (const char*)pFrom->GetName() : "Unknown";

    if (type == CMsgIPC::email) {
        SDL2MailMsg m;
        m.fromPlyr = from;
        m.fromName = fromName;
        m.subject  = subject;
        m.body     = message;
        m.read     = false;
        g_mailInbox.push_back(m);
        // Same "you have new mail" cue the MFC CWndComm::ProcessEmail fired.
        theGame.Event(EVENT_HAVE_MAIL, EVENT_NOTIFY);
    } else if (type == CMsgIPC::chat) {
        // Legacy CMsgIPC chat path (modern chat uses CNetChat) ??? fold into the log.
        SDL2Chat_AddMessage(fromName + ": " + message);
    }
}

// ---------------------------------------------------------------------------
// Non-modal comms windows. The game must keep running (and keep processing
// network messages) while you read/write mail or chat ??? DoModal ran its own
// loop and froze the local client, which on a network game means stalling and
// catch-up. Each window is a single live instance; re-opening raises it. The
// onDone callback nulls the pointer and GameWindow deletes the object.
// ---------------------------------------------------------------------------
static SDL2ChatWindow*    g_pChatWin    = nullptr;
static SDL2MailDialog*    g_pMailWin    = nullptr;
static SDL2ComposeDialog* g_pComposeWin = nullptr;

void SDL2Comms_OpenChat(GameWindow* gw) {
    if (g_pChatWin) { g_pChatWin->RaiseAndAlert(); return; }
    g_pChatWin = new SDL2ChatWindow(gw);
    g_pChatWin->ShowNonModal([](int) { g_pChatWin = nullptr; });
}

void SDL2Comms_OpenMail(GameWindow* gw) {
    if (g_pMailWin) { g_pMailWin->RaiseAndAlert(); return; }
    g_pMailWin = new SDL2MailDialog(gw);
    g_pMailWin->ShowNonModal([](int) { g_pMailWin = nullptr; });
}

void SDL2Comms_OpenCompose(GameWindow* gw, int toPlyr,
                           const std::string& subject, const std::string& body) {
    if (g_pComposeWin) { g_pComposeWin->RaiseAndAlert(); return; }
    g_pComposeWin = new SDL2ComposeDialog(gw, toPlyr, subject, body);
    g_pComposeWin->ShowNonModal([](int) { g_pComposeWin = nullptr; });
}

void SDL2Comms_CloseAll() {
    // EndDialog on a non-modal dialog destroys its window now and fires onDone
    // (which nulls the pointer); null defensively in case it's mid-teardown.
    if (g_pComposeWin) { g_pComposeWin->EndDialog(0); g_pComposeWin = nullptr; }
    if (g_pMailWin)    { g_pMailWin->EndDialog(0);    g_pMailWin    = nullptr; }
    if (g_pChatWin)    { g_pChatWin->EndDialog(0);    g_pChatWin    = nullptr; }
}

// ----------------------------------------------------------------------------
// ResearchFlaskBar ??? the light-bulb/flask progress strip from CDlgResearch.
// The original painted ICON_RESEARCH via CStatInst::DrawStatDone: the flask
// sprite tiled (overlapping by half its width) across `percent`% of the strip.
// We mirror that here, querying the live percent each frame so the strip fills
// as research advances while the (non-modal) window stays open.
// ----------------------------------------------------------------------------
class ResearchFlaskBar : public SDL2Widget {
public:
    ResearchFlaskBar(int x, int y, int w, int h, SDL_Surface* sheet,
                     int cx, int cy, int leftOff, int rightOff,
                     std::function<int()> getPercent)
        : SDL2Widget(x, y, w, h), m_sheet(sheet), m_cx(cx), m_cy(cy),
          m_leftOff(leftOff), m_rightOff(rightOff), m_getPercent(std::move(getPercent)) {}

    void Render(SDL_Surface* dst, TTF_Font*) override {
        if (!m_visible || !m_sheet || m_cx <= 0 || m_cy <= 0) return;
        int per = m_getPercent ? m_getPercent() : 0;
        if (per <= 0) return;
        if (per > 100) per = 100;

        // Mirror CStatInst::DrawStatDone (icons.cpp), in absolute coords.
        int top   = m_rect.y + (m_rect.h - m_cy) / 2;
        int left  = m_rect.x + m_leftOff;
        int right = m_rect.x + m_rect.w - m_rightOff;
        int width = right - left;
        int iEnd  = right;
        if (per < 100) iEnd -= m_cx / 2;       // leave the last flask for 100%
        int iRight = left + (width * per) / 100;
        iRight = __max(left + 1, iRight);      // at least one flask when > 0
        int iAdd = m_cx / 2;
        if (iAdd < 1) iAdd = 1;
        for (int x = left; x < iRight; x += iAdd) {
            if (x + m_cx > iEnd) break;
            SDL_Rect sr = { 0, 0, m_cx, m_cy };   // m_iBaseOn == 0 (base flask)
            SDL_Rect dr = { x, top, m_cx, m_cy };
            SDL_BlitSurface(m_sheet, &sr, dst, &dr);
        }
    }

private:
    SDL_Surface* m_sheet;
    int m_cx, m_cy, m_leftOff, m_rightOff;
    std::function<int()> m_getPercent;
};

// Current research progress as a percent (1..99), matching CDlgResearch::UpdateProgress.
// Runs every frame from the (non-modal) flask bar, so it must tolerate having no
// local player: GetMe() asserts + returns NULL during game teardown / before the
// player is set. Use the assert-free _GetMe() and bail when there's no player.
static int ResearchCurrentPercent() {
    CPlayer* pMe = theGame._GetMe();
    if (!pMe) return 0;
    int iSel = pMe->GetRsrchItem();
    if (iSel <= 0) return 0;
    int disc = pMe->GetRsrch(iSel).m_iPtsDiscovered;
    int req  = theRsrch.ElementAt(iSel).m_iPtsRequired;
    if (req <= 0) return 0;
    int per = (disc * 50) / req;
    per = __min(per, 99);
    per = __max(1, per);
    return per;
}

// ============================================================================
// SDL2ResearchDialog ??? matches CDlgResearch layout from the original MFC dialog
//
// MFC client area was 447x291. Layout from CDlgResearch::OnInitDialog:
//   list      (18,  35) 155x198    ??? listbox of researchable items
//   desc text (192, 37) ??? (420,156) ??? rectText, green PALETTERGB(41,255,8)
//   IDOK     (188, 170) 116x25     ??? "Research" button
//   IDCANCEL (311, 170) 116x25     ??? "Close" button
//   Discovery(311, 211) 116x25     ??? "Discovery" button (enabled when there's
//                                     a new discovery to view)
//   bulbs    (17, 248) ??? (425,272) ??? light-bulb progress strip (DIB+ICON_RESEARCH)
// ============================================================================

SDL2ResearchDialog::SDL2ResearchDialog(GameWindow* gw)
    // Interior = 447x291 to match MFC. Outer dim = interior + side borders
    // (6 each) + title bar (26). See SDL2BuildStructure for the same recipe.
    : SDL2Dialog(gw, "Research", 447 + 12, 291 + 12 + 26)
{
    auto load = [](int idx) -> SDL_Surface* {
        CDIB* p = theBitmaps.GetByIndex(idx);
        return p ? SDL2MainMenu::CreateSurfaceFromDIB(p) : nullptr;
    };
    m_bkgnd    = load(DIB_RSRCH_BKGND);
    m_btnSheet = load(DIB_RESEARCH_BTNS);
}

SDL2ResearchDialog::~SDL2ResearchDialog() {
    if (m_bkgnd)      SDL_FreeSurface(m_bkgnd);
    if (m_btnSheet)   SDL_FreeSurface(m_btnSheet);
    if (m_flaskSheet) SDL_FreeSurface(m_flaskSheet);
}

void SDL2ResearchDialog::OnInit() {
    if (m_bkgnd) SetCustomBackground(m_bkgnd);

    // Dialog interior origin == MFC client area origin (skip side border + title)
    const int bdrSide = 6, bdrTop = 6, titleH = 26;
    int ox = m_x + bdrSide;
    int oy = m_y + bdrTop + titleH;

    // List: matches CDlgResearch::OnDrawItem ??? red text PALETTERGB(255,33,8) on a
    // white row, inverted to red-on-black when selected.
    // Double-click starts researching the item (matches CDlgResearch::OnDblclkRsrchList).
    m_list = AddWidget<SDL2Listbox>(ox + 18, oy + 35, 155, 198,
        [this](int idx) { SelectItem(idx); },
        [this](int idx) { SelectItem(idx); OnStart(); });
    const SDL_Color kRsrchRed = { 255, 33, 8, 255 };
    m_list->SetColors(/*bg*/{255, 255, 255, 255}, /*selBg*/{0, 0, 0, 255},
                      /*text*/kRsrchRed, /*selText*/kRsrchRed);

    // Description text: green PALETTERGB(41,255,8), top-aligned, wrapped.
    // MFC rectText(192, 37, 420, 156) ??? x=192,y=37, w=228, h=119
    m_lblDesc = AddWidget<SDL2Label>(ox + 192, oy + 37, 228, 119, "");
    m_lblDesc->SetWrapped(true);
    m_lblDesc->SetTopAligned(true);
    m_lblDesc->SetColor({41, 255, 8, 255});

    // Light-bulb/flask progress strip (MFC rect 17,248,425,272 ??? ICON_RESEARCH).
    // Renders live each frame so it fills as the current research advances.
    {
        CStatData* pSd = theIcons.GetByIndex(ICON_RESEARCH);
        if (pSd && pSd->m_pcDib) {
            m_flaskSheet = SDL2MainMenu::CreateSurfaceFromDIB(pSd->m_pcDib);
            AddWidget<ResearchFlaskBar>(ox + 17, oy + 248, 408, 24, m_flaskSheet,
                pSd->m_cxIcon, pSd->m_cyIcon, pSd->m_leftOff, pSd->m_rightOff,
                []() { return ResearchCurrentPercent(); });
        }
    }

    // Buttons ??? use the 3-state research button sheet (circuit art) with red text
    // PALETTERGB(255,33,8), matching CDlgResearch::OnDrawItem.
    const SDL_Color kBtnRed = { 255, 33, 8, 255 };
    m_btnStart = AddWidget<SDL2Button>(ox + 188, oy + 170, 116, 25, "Research",
        [this]() { OnStart(); });
    m_btnStart->SetEnabled(false);
    m_btnStart->SetTextColor(kBtnRed);
    if (m_btnSheet) m_btnStart->SetBtnSheet(m_btnSheet);

    m_btnClose = AddWidget<SDL2Button>(ox + 311, oy + 170, 116, 25, "Close",
        [this]() { EndDialog(0); });
    m_btnClose->SetTextColor(kBtnRed);
    if (m_btnSheet) m_btnClose->SetBtnSheet(m_btnSheet);

    m_btnDiscover = AddWidget<SDL2Button>(ox + 311, oy + 211, 116, 25, "Discovery",
        [this]() { OnDiscover(); });
    // Re-shows the most recently discovered topic's result text. Seeded from the
    // player's persisted last-discovery (serialized in saves, see CPlayer), so it
    // works immediately after loading a game; NotifyDiscovered() re-arms it when a
    // topic completes while the window is open. Disabled until there's one to show.
    m_lastDiscovered = theGame.GetMe()->GetLastDiscovered();
    m_btnDiscover->SetEnabled(m_lastDiscovered > 0);
    m_btnDiscover->SetTextColor(kBtnRed);
    if (m_btnSheet) m_btnDiscover->SetBtnSheet(m_btnSheet);

    PopulateList();

    // Highlight current research
    int curRsrch = theGame.GetMe()->GetRsrchItem();
    for (int i = 0; i < (int)m_items.size(); i++) {
        if (m_items[i].index == curRsrch) {
            m_list->SetSelected(i);
            m_list->EnsureVisible(i);
            SelectItem(i);
            break;
        }
    }
}

void SDL2ResearchDialog::NotifyDiscovered(int iItem) {
    // Arm the "Discovery" button to re-show this topic's result text (matches the
    // original CDlgResearch::ItemDiscovered enabling m_btnDiscovery).
    if (iItem > 0 && iItem < theRsrch.GetSize()) {
        m_lastDiscovered = iItem;
        if (m_btnDiscover) m_btnDiscover->SetEnabled(true);
    }
    Refresh();
}

void SDL2ResearchDialog::OnDiscover() {
    // Re-show the most recently discovered topic's result text, as a small
    // non-modal popup (modal would freeze the running sim). No-op until a topic has
    // been discovered while this window was open.
    if (m_lastDiscovered <= 0 || m_lastDiscovered >= theRsrch.GetSize())
        return;
    CRsrchItem const& item = theRsrch[m_lastDiscovered];
    auto* dlg = new SDL2DiscoverDialog(m_gameWindow, item.m_sName.c_str(),
                                       item.m_sResult.c_str());
    dlg->ShowNonModal();   // GameWindow owns deletion after it closes
}

void SDL2ResearchDialog::PopulateList() {
    m_items.clear();
    m_list->Clear();

    CPlayer* pMe = theGame.GetMe();

    // The topic currently being researched gets a persistent green marker in the
    // list (separate from the blue browse-selection) so it stays identifiable even
    // while the player clicks other items to read their descriptions.
    int curRsrch = pMe->GetRsrchItem();

    // Use the engine's authoritative research state, NOT a points threshold:
    //   discovered == CRsrchStatus::m_bDiscovered (set by CPlayer::Research)
    //   available  == CPlayer::CanRsrch (prereqs discovered + buildings built +
    //                 scenario gate + not already discovered)
    // The old code tested m_iPtsDiscovered >= m_iPtsRequired, which is only the 50%
    // mark of the probabilistic discovery window (CPlayer::Research keeps awarding
    // points up to required*2 before the topic is actually flagged discovered). That
    // made the topic being researched vanish from the list once it crossed 50%, and
    // unlocked child topics early. Skip slot 0 (CRsrchArray::nothing).
    for (int i = 1; i < theRsrch.GetSize(); i++) {
        CRsrchItem const& item = theRsrch[i];

        bool discovered = pMe->GetRsrch(i).m_bDiscovered;
        bool available  = pMe->CanRsrch(i);   // CanRsrch is FALSE once discovered

        // Show currently-researchable topics; already-discovered ones only when the
        // "[Done]" view is on; locked topics (prereqs/scenario not met) are hidden,
        // matching the original CDlgResearch::UpdateChoices.
        if (!available && !(discovered && m_showDone))
            continue;

        RsrchEntry entry;
        entry.index = i;
        entry.name = item.m_sName.c_str();
        entry.available = available;

        std::string displayName = entry.name;
        if (discovered) displayName += " [Done]";

        if (i == curRsrch)
            m_list->SetMarked((int)m_items.size());  // list index of the active topic

        m_items.push_back(entry);
        m_list->AddItem(displayName);
    }
}

// Re-read research state into the list while preserving the player's browse
// selection (by topic index, not list row). Called when a topic is discovered so
// the open (non-modal) window updates live ??? completed topics drop off, newly
// unlocked ones appear, and the stale "current research" marker clears ??? instead
// of going stale until reopened (mirrors the original ResearchDiscovered ->
// CDlgResearch::UpdateChoices refresh).
void SDL2ResearchDialog::Refresh() {
    if (!m_list) return;   // not initialized yet

    int browseIdx = (m_selected >= 0 && m_selected < (int)m_items.size())
                        ? m_items[m_selected].index : -1;

    PopulateList();

    m_selected = -1;
    if (browseIdx > 0) {
        for (int i = 0; i < (int)m_items.size(); i++)
            if (m_items[i].index == browseIdx) {
                m_list->SetSelected(i);
                SelectItem(i);   // restores description + button state
                break;
            }
    }

    // The browsed topic is gone (e.g. it was just discovered) ??? clear the detail
    // panel so it doesn't describe a topic no longer in the list.
    if (m_selected < 0) {
        m_lblDesc->SetText("");
        m_btnStart->SetEnabled(false);
        m_btnStart->SetToggled(false);
    }
}

// Format a research-point cost compactly + locale-aware (operator request):
//   < 10,000      -> full number, locale-grouped (e.g. "9,125")
//   10k .. < 1m   -> "Nk"   (e.g. 248000 -> "248k", rounded to nearest thousand)
//   >= 1,000,000  -> "Nm"   (e.g. 2000000 -> "2m"; fractional -> 1 dp, e.g. "2.5m")
// The system locale ("") drives the thousands/decimal separators; falls back to the
// classic "C" locale if the system locale is unavailable.
static std::string FmtPoints(long long v) {
    if (v < 0) return "-" + FmtPoints(-v);
    std::ostringstream ss;
    try { ss.imbue(std::locale("")); } catch (...) { /* keep classic locale */ }
    if (v < 10000) {
        ss << v;                                     // full number, grouped per locale
    } else if (v < 1000000) {
        long long k = (v + 500) / 1000;              // round to nearest thousand
        if (k >= 1000) ss << "1m"; else ss << k << "k";
    } else if (v % 1000000 == 0) {
        ss << (v / 1000000) << "m";                  // whole millions -> "2m"
    } else {
        ss << std::fixed << std::setprecision(1) << (v / 1000000.0) << "m";  // "2.5m"
    }
    return ss.str();
}

void SDL2ResearchDialog::SelectItem(int idx) {
    if (idx < 0 || idx >= (int)m_items.size()) return;
    m_selected = idx;

    CRsrchItem const& item = theRsrch[m_items[idx].index];
    // Show the description plus the topic's point cost (Note 11 / BUGS #19): the
    // operator wants the research-points-to-discover figure visible in the detail
    // box (which has room below the wrapped description text). Cost is formatted
    // compactly + locale-aware (operator request) — see FmtPoints above.
    std::string desc = item.m_sDesc;
    if (!desc.empty()) desc += "\n\n";
    desc += "Cost: " + FmtPoints(item.m_iPtsRequired) + " research points";
    m_lblDesc->SetText(desc.c_str());

    // The Research button stays "down" (pressed sprite) while the selected item is
    // the topic currently being researched; for any other item it's an enabled
    // "start researching this" button. Progress itself is shown by the flask strip
    // (always the current research), not per-selection.
    bool isActive = (m_items[idx].index == theGame.GetMe()->GetRsrchItem());
    m_btnStart->SetToggled(isActive);
    m_btnStart->SetEnabled(m_items[idx].available);
}

void SDL2ResearchDialog::OnStart() {
    if (m_selected < 0 || m_selected >= (int)m_items.size()) return;
    if (!m_items[m_selected].available) return;
    int iSel = m_items[m_selected].index;

    // Switching away from another in-progress topic costs it 10% of its points,
    // matching CDlgResearch::OnOK. Then set the new topic ??? but DON'T close the
    // window (the original stayed open and just updated its title/progress).
    int iOn = theGame.GetMe()->GetRsrchItem();
    if (iOn > 0 && iOn != iSel) {
        CRsrchStatus& rs = theGame.GetMe()->GetRsrch(iOn);
        rs.m_iPtsDiscovered -= rs.m_iPtsDiscovered / 10;
    }
    theGame.GetMe()->SetRsrchItem(iSel);

    // Refresh the list (markers) and keep the just-started item selected. The
    // flask strip picks up the new current-research progress on its own.
    PopulateList();
    for (int i = 0; i < (int)m_items.size(); i++)
        if (m_items[i].index == iSel) {
            m_list->SetSelected(i);
            m_list->EnsureVisible(i);  // keep scroll position; don't jump to top
            SelectItem(i);
            break;
        }
}

// ============================================================================
// SDL2RelationsDialog
// ============================================================================

SDL2RelationsDialog::SDL2RelationsDialog(GameWindow* gw)
    : SDL2Dialog(gw, "Relations", 350, 280)
{
    // Don't force this window above the map every frame — the player should be able
    // to tuck it behind the area map instead of having it permanently on top.
    SetKeepOnTop(false);
}

void SDL2RelationsDialog::OnInit() {
    m_list = AddWidget<SDL2Listbox>(m_x + 10, m_y + 34, 150, 180,
        [this](int idx) { SelectPlayer(idx); });

    m_lblInfo = AddWidget<SDL2Label>(m_x + 170, m_y + 34, 170, 40, "");

    // Radio buttons for relation level. Start with NO selection (-1) — nothing is
    // checked until a player is picked from the list; otherwise index 0
    // showed as selected even though no player was chosen.
    // BUGS.md #5: option order MUST match the RELATIONS_* enum (player.h) so the radio
    // index == the relation value passed to NewRelations / returned by GetRelations.
    // enum: RELATIONS_ALLIANCE=0, RELATIONS_PEACE=1, RELATIONS_NEUTRAL=2, RELATIONS_WAR=3.
    // (Was {War,Neutral,Peace,Alliance} with the raw index passed unmapped → every
    // diplomacy choice did the OPPOSITE of its label, and the display was reversed too.)
    std::vector<std::string> options = {"Alliance", "Peace", "Neutral", "War"};
    m_radRelations = AddWidget<SDL2RadioGroup>(m_x + 170, m_y + 80, 160, 120, options,
        -1, [this](int sel) { SetRelation(sel); });
    for (int r = 0; r < 4; r++) m_radRelations->SetEnabled(r, false);

    // Give button ??? hands selected area-map units to the chosen player.
    // Enabled when there's both a selected non-self player AND at least one
    // giveable unit selected on the area map (matches CDlgRelations).
    m_btnGive = AddWidget<SDL2Button>(m_x + 10, m_y + 220, 100, 28, "Give Units",
        [this]() { OnGive(); });
    m_btnGive->SetEnabled(false);

    AddWidget<SDL2Button>(m_x + 240, m_y + 220, 90, 28, "Close",
        [this]() { EndDialog(0); });

    // Populate player list (skip self)
    POSITION pos = theGame.GetAll().GetHeadPosition();
    while (pos != NULL) {
        CPlayer* pPlr = theGame.GetAll().GetNext(pos);
        if (pPlr->IsMe()) continue;

        PlayerEntry entry;
        entry.pPlr = pPlr;
        entry.name = (const char*)pPlr->GetName();
        m_players.push_back(entry);
        m_list->AddItem(entry.name);
    }
}

void SDL2RelationsDialog::SelectPlayer(int idx) {
    if (idx < 0 || idx >= (int)m_players.size()) return;
    m_selected = idx;
    CPlayer* pPlr = m_players[idx].pPlr;

    m_lblInfo->SetText(m_players[idx].name);

    // Enable all relation radios ??? but disable Alliance for AI players
    // (matches MFC GetDlgItem(IDC_PLYR_ALLIANCE)->EnableWindow(!pPlr->IsAI())).
    // Indices now match the enum: 0=Alliance, 1=Peace, 2=Neutral, 3=War (BUGS.md #5).
    for (int r = 0; r < 4; r++) m_radRelations->SetEnabled(r, true);
    if (pPlr->IsAI())
        m_radRelations->SetEnabled(RELATIONS_ALLIANCE, false);   // index 0 = Alliance
    m_radRelations->SetSelected(pPlr->GetRelations());

    // Update Give button state from area-map selection
    CWndArea* pWndArea = theAreaList.GetTop();
    bool canGive = (pWndArea && pWndArea->NumGiveable() > 0);
    m_btnGive->SetEnabled(canGive);
}

void SDL2RelationsDialog::SetRelation(int level) {
    if (m_selected < 0) return;
    CPlayer* pPlr = m_players[m_selected].pPlr;

    NewRelations(pPlr, level);
}

void SDL2RelationsDialog::OnGive() {
    if (m_selected < 0) return;
    CWndArea* pWndArea = theAreaList.GetTop();
    if (!pWndArea || pWndArea->NumGiveable() <= 0) {
        m_btnGive->SetEnabled(false);
        return;
    }
    pWndArea->GiveSelectedUnits(m_players[m_selected].pPlr);
    // Nothing left to give now that the units have changed owner
    m_btnGive->SetEnabled(false);
}

// ============================================================================
// SDL2LoadTruckDialog
// ============================================================================

namespace {
    // The six transportable materials shown in the Load Truck dialog, in the
    // same order/labels as the original IDD_TRUCK template. Row index -> the
    // CMaterialTypes enum value. The enum is NOT contiguous across these
    // (moly/goods/food/gas sit in between), so every slider <-> store access must
    // map through this table — using the row index as a material id (the old bug)
    // mislabeled every row AND made real coal/iron/oil unreachable.
    const int kTruckMat[6] = {
        CMaterialTypes::lumber,
        CMaterialTypes::steel,
        CMaterialTypes::copper,   // shown as "Xilitium" (matches IDD_TRUCK)
        CMaterialTypes::coal,
        CMaterialTypes::iron,
        CMaterialTypes::oil,
    };
    // Dialog ROW indices (positions within kTruckMat) for presets that target
    // specific materials by name.
    enum { ROW_LUMBER = 0, ROW_STEEL = 1, ROW_XIL = 2, ROW_COAL = 3, ROW_IRON = 4, ROW_OIL = 5 };
    const char* const kTruckMatName[6] = { "Lumber", "Steel", "Xilitium", "Coal", "Iron", "Oil" };
}

SDL2LoadTruckDialog::SDL2LoadTruckDialog(GameWindow* gw, CVehicle* pVeh)
    // Dynamic title: "Load Freighter" for boats, otherwise "Load Truck - [Bldg]"
    // (matches CDlgLoadTruck::OnInitDialog using IDS_LOAD_FREIGHTER / IDS_LOAD_TRUCK)
    : SDL2Dialog(gw,
        pVeh && pVeh->GetData()->IsBoat() ? "Load Freighter" :
        ([&]() -> std::string {
            CBuilding* pB = pVeh ? theBuildingHex._GetBuilding(pVeh->GetPtHead()) : nullptr;
            if (pB) return std::string("Load Truck - [") + pB->GetData()->GetDesc().c_str() + "]";
            return "Load Truck";
        })(),
        360, 460)
    , m_pVeh(pVeh)
    , m_pBldg(nullptr)
{
    m_pBldg = theBuildingHex._GetBuilding(pVeh->GetPtHead());
}

void SDL2LoadTruckDialog::OnInit() {
    if (!m_pBldg) {
        AddWidget<SDL2Label>(m_x + 10, m_y + 40, 340, 24, "No building found.");
        AddWidget<SDL2Button>(m_x + 130, m_y + 80, 90, 28, "Close",
            [this]() { EndDialog(0); });
        return;
    }

    int y = m_y + 36;

    for (int i = 0; i < 6; i++) {
        int mat = kTruckMat[i];
        int vehHas = m_pVeh->GetStore(mat);
        int bldgHas = m_pBldg->GetStore(mat);
        int maxVal = vehHas + bldgHas;

        AddWidget<SDL2Label>(m_x + 12, y, 62, 20, kTruckMatName[i]);
        m_sliders[i] = AddWidget<SDL2Slider>(m_x + 78, y, 172, 20, 0, maxVal, vehHas,
            [this, i](int v) { OnSliderChanged(i, v); });
        m_sliders[i]->SetShowValue(false);   // we draw our own "loaded/available" column
        // Right-align the "have/max" readout against the content edge so the
        // numbers line up in a column and never clip off the right of the window.
        m_lblAmounts[i] = AddWidget<SDL2Label>(m_x + 254, y, 96, 20,
            std::to_string(vehHas) + "/" + std::to_string(maxVal));
        m_lblAmounts[i]->SetRightAligned(true);
        y += 28;
    }

    // Capacity readout ??? total/cap below the material rows
    y += 8;
    int maxCargo = m_pVeh->GetMaxMaterials();
    m_lblCapacity = AddWidget<SDL2Label>(m_x + 10, y, 340, 20,
        "Capacity: 0 / " + std::to_string(maxCargo));
    m_lblCapacity->SetCentered(true);
    y += 26;

    // Preset buttons row 1: Load (proportional), Load Bldg (50/50 steel/lumber),
    // Load Veh (80/20 steel/copper) ??? same presets the MFC dialog offered.
    int btnW = 105, btnH = 26, btnGap = 6;
    AddWidget<SDL2Button>(m_x + 10, y, btnW, btnH, "Load",
        [this]() { OnLoad(); });
    AddWidget<SDL2Button>(m_x + 10 + (btnW + btnGap), y, btnW, btnH, "Load Bldg",
        [this]() { OnLoadBldg(); });
    AddWidget<SDL2Button>(m_x + 10 + 2 * (btnW + btnGap), y, btnW, btnH, "Load Veh",
        [this]() { OnLoadVeh(); });
    y += btnH + btnGap;

    // Preset buttons row 2: Unload (clear) + Auto (hand back to router)
    AddWidget<SDL2Button>(m_x + 10, y, btnW, btnH, "Unload",
        [this]() { OnUnload(); });
    AddWidget<SDL2Button>(m_x + 10 + (btnW + btnGap), y, btnW, btnH, "Auto",
        [this]() { OnAuto(); });
    y += btnH + 12;

    // OK / Cancel
    AddWidget<SDL2Button>(m_x + 60, y, 100, btnH, "OK",
        [this]() { OnOK(); });
    AddWidget<SDL2Button>(m_x + 200, y, 100, btnH, "Cancel",
        [this]() { OnCancel(); });

    RefreshTotals();
}

void SDL2LoadTruckDialog::OnSliderChanged(int idx, int val) {
    // Enforce the vehicle's total cargo cap. Each slider is already bounded to its
    // own material's available stock, but the SUM across all six could still exceed
    // GetMaxMaterials() — so dragging one up would overload the truck. If this change
    // pushes the total over, trim THIS slider back to whatever room is left.
    // Use the vehicle's TECHED capacity (GetMaxMaterials applies cargo research),
    // not GetData()->GetMaxMaterials() (the base) — otherwise the drag cap sits at
    // the un-teched limit (e.g. 2100) while the readout shows the real cap (2310).
    int maxCargo = m_pVeh->GetMaxMaterials();
    int others = 0;
    for (int i = 0; i < 6; i++)
        if (i != idx) others += m_sliders[i]->GetValue();
    int allowed = maxCargo - others;
    if (allowed < 0) allowed = 0;
    if (val > allowed)
        m_sliders[idx]->SetValue(allowed);   // SetValue does not re-fire onChange
    RefreshTotals();
}

void SDL2LoadTruckDialog::RefreshTotals() {
    if (!m_lblCapacity) return;
    int total = 0;
    for (int i = 0; i < 6; i++) {
        int v = m_sliders[i]->GetValue();
        total += v;
        // Keep each row's "loaded/available" readout in sync with its slider
        // (the available total is fixed while the dialog is open: truck + building
        // stock for that material — recomputed here so it tracks any preset/drag).
        if (m_lblAmounts[i]) {
            int mat = kTruckMat[i];
            int maxVal = m_pVeh->GetStore(mat) + (m_pBldg ? m_pBldg->GetStore(mat) : 0);
            m_lblAmounts[i]->SetText(std::to_string(v) + "/" + std::to_string(maxVal));
        }
    }
    int maxCargo = m_pVeh->GetMaxMaterials();
    m_lblCapacity->SetText("Capacity: " + std::to_string(total) +
                           " / " + std::to_string(maxCargo));
}

void SDL2LoadTruckDialog::RefreshBldg() {
    // The vehicle owns this dialog and closes it on death, so m_pVeh is valid here;
    // the building, however, can be destroyed under us ??? re-derive it (or null) each
    // time rather than trusting the pointer cached at construction.
    m_pBldg = m_pVeh ? theBuildingHex._GetBuilding(m_pVeh->GetPtHead()) : nullptr;
}

void SDL2LoadTruckDialog::OnLoad() {
    // MFC OnTruckLoad: fill from combined truck+building stocks, scaling DOWN
    // proportionally if the total exceeds vehicle capacity.
    RefreshBldg();
    if (!m_pBldg) return;
    int amounts[6];
    int total = 0;
    for (int i = 0; i < 6; i++) {
        int mat = kTruckMat[i];
        amounts[i] = m_pVeh->GetStore(mat) + m_pBldg->GetStore(mat);
        total += amounts[i];
    }
    int maxCargo = m_pVeh->GetMaxMaterials();
    if (total > maxCargo && total > 0) {
        // Leave one slot of headroom (matching MFC's max-1 trick)
        float fMul = (float)(maxCargo - 1) / (float)total;
        for (int i = 0; i < 6; i++)
            amounts[i] = (int)((float)amounts[i] * fMul);
    }
    for (int i = 0; i < 6; i++) m_sliders[i]->SetValue(amounts[i]);
    RefreshTotals();
}

void SDL2LoadTruckDialog::OnLoadBldg() {
    // 50/50 Steel / Lumber ??? construction load preset.
    RefreshBldg();
    if (!m_pBldg) return;
    int maxCargo = m_pVeh->GetMaxMaterials();
    int iSteel  = maxCargo / 2;
    int iLumber = maxCargo / 2;

    int steelCap  = m_pVeh->GetStore(CMaterialTypes::steel)  + m_pBldg->GetStore(CMaterialTypes::steel);
    int lumberCap = m_pVeh->GetStore(CMaterialTypes::lumber) + m_pBldg->GetStore(CMaterialTypes::lumber);
    if (iSteel  > steelCap)  { iSteel  = steelCap;  iLumber = maxCargo - iSteel;  }
    if (iLumber > lumberCap) { iLumber = lumberCap; }

    for (int i = 0; i < 6; i++) m_sliders[i]->SetValue(0);
    m_sliders[ROW_STEEL]->SetValue(iSteel);
    m_sliders[ROW_LUMBER]->SetValue(iLumber);
    RefreshTotals();
}

void SDL2LoadTruckDialog::OnLoadVeh() {
    // 80/20 Steel / Copper ??? vehicle-factory load preset.
    RefreshBldg();
    if (!m_pBldg) return;
    int maxCargo = m_pVeh->GetMaxMaterials();
    int iSteel  = (maxCargo * 4) / 5;
    int iCopper = maxCargo / 5;

    int steelCap  = m_pVeh->GetStore(CMaterialTypes::steel)  + m_pBldg->GetStore(CMaterialTypes::steel);
    int copperCap = m_pVeh->GetStore(CMaterialTypes::copper) + m_pBldg->GetStore(CMaterialTypes::copper);
    if (iSteel  > steelCap)  { iSteel  = steelCap;  iCopper = maxCargo - iSteel; }
    if (iCopper > copperCap) { iCopper = copperCap; }

    for (int i = 0; i < 6; i++) m_sliders[i]->SetValue(0);
    m_sliders[ROW_STEEL]->SetValue(iSteel);
    m_sliders[ROW_XIL]->SetValue(iCopper);
    RefreshTotals();
}

void SDL2LoadTruckDialog::OnOK() {
    // Transfer materials from sliders to vehicle/building, capping at the
    // combined truck+building total per material (matches MFC Transfer()).
    RefreshBldg();
    if (!m_pBldg) { EndDialog(0); return; }   // building gone ??? nothing to transfer
    for (int i = 0; i < 6; i++) {
        int mat = kTruckMat[i];
        int iAmount = m_sliders[i]->GetValue();
        int iTotal = m_pVeh->GetStore(mat) + m_pBldg->GetStore(mat);
        if (iAmount > iTotal) iAmount = iTotal;
        m_pBldg->SetStore(mat, iTotal - iAmount);
        m_pVeh->SetStore(mat, iAmount);
    }
    m_pBldg->MaterialMessage();
    m_pBldg->EventOff();
    m_pVeh->ExitBuilding();
    EndDialog(1);   // onDone clears CVehicle::m_pSdlLoad
}

void SDL2LoadTruckDialog::OnCancel() {
    EndDialog(0);
}

void SDL2LoadTruckDialog::OnUnload() {
    for (int i = 0; i < 6; i++)
        m_sliders[i]->SetValue(0);
    RefreshTotals();
}

void SDL2LoadTruckDialog::OnAuto() {
    m_pVeh->DumpContents();
    EndDialog(1);
}

// ============================================================================
// SDL2PauseDialog
// ============================================================================

SDL2PauseDialog::SDL2PauseDialog(GameWindow* gw, const std::string& message)
    : SDL2Dialog(gw, "Game Paused", 300, 120)
    , m_message(message)
{}

void SDL2PauseDialog::OnInit() {
    auto* lbl = AddWidget<SDL2Label>(m_x + 10, m_y + 36, 280, 40, m_message);
    lbl->SetCentered(true);

    AddWidget<SDL2Button>(m_x + 105, m_y + 80, 90, 28, "OK",
        [this]() { EndDialog(1); });
}

// ============================================================================
// SDL2DiscoverDialog
// ============================================================================

// Standard EN dialog chrome (gold frame + title bar + parchment), like the
// original CDlgDiscover (IDD_RSRCH_FOUND) and every other in-game dialog. The
// research window's circuit-board art was tried as a background but scaled to this
// small size it looked terrible, so we use the clean shared chrome instead. Compact
// (300x190) ??? close to the original's 186x110 DLU, not the old 350x200 placeholder.
SDL2DiscoverDialog::SDL2DiscoverDialog(GameWindow* gw,
    const std::string& title, const std::string& description)
    // The tech name lives in the title bar (matches the original CDlgDiscover
    // caption "Discovered - [<name>]"), not a floating body label.
    : SDL2Dialog(gw, std::string("Discovered - [") + title + "]", 300, 190)
    , m_discDesc(description)
{}

void SDL2DiscoverDialog::OnInit() {
    // No custom background ??? the base draws the gold frame, the title bar (carrying
    // the tech name) and the parchment interior.
    const int titleBot = 6 /*top border*/ + 26 /*title bar*/;

    // Result text: default dialog blue PALETTERGB(48,58,148) ??? readable on the
    // parchment ??? wrapped and top-aligned (mirrors the original's read-only box).
    auto* lblDesc = AddWidget<SDL2Label>(m_x + 14, m_y + titleBot + 8,
                                         m_width - 28, m_height - titleBot - 8 - 44,
                                         m_discDesc);
    lblDesc->SetWrapped(true);
    lblDesc->SetTopAligned(true);

    // Standard centered OK button (matches the parchment chrome).
    AddWidget<SDL2Button>(m_x + (m_width - 96) / 2, m_y + m_height - 40, 96, 28, "OK",
        [this]() { EndDialog(1); });
}

// ============================================================================
// SDL2MessageBox
// ============================================================================

SDL2MessageBox::SDL2MessageBox(GameWindow* gw, const std::string& message, Style style)
    : SDL2Dialog(gw, "Enemy Nations", 360, 140)
    , m_message(message)
    , m_style(style)
{}

void SDL2MessageBox::OnInit() {
    auto* lbl = AddWidget<SDL2Label>(m_x + 15, m_y + 34, m_width - 30, 55, m_message);
    lbl->SetWrapped(true);
    lbl->SetCentered(true);

    int btnW = 80, btnH = 28;
    int btnY = m_y + m_height - btnH - 15;

    if (m_style == YesNoCancel) {
        int spacing = 10;
        int totalW = btnW * 3 + spacing * 2;
        int startX = m_x + (m_width - totalW) / 2;
        AddWidget<SDL2Button>(startX, btnY, btnW, btnH, "Yes",
            [this]() { EndDialog(IDYES); });
        AddWidget<SDL2Button>(startX + btnW + spacing, btnY, btnW, btnH, "No",
            [this]() { EndDialog(IDNO); });
        AddWidget<SDL2Button>(startX + (btnW + spacing) * 2, btnY, btnW, btnH, "Cancel",
            [this]() { EndDialog(IDCANCEL); });
    } else {
        int spacing = 20;
        int totalW = btnW * 2 + spacing;
        int startX = m_x + (m_width - totalW) / 2;
        AddWidget<SDL2Button>(startX, btnY, btnW, btnH, "Yes",
            [this]() { EndDialog(IDYES); });
        AddWidget<SDL2Button>(startX + btnW + spacing, btnY, btnW, btnH, "No",
            [this]() { EndDialog(IDNO); });
    }
}

// ============================================================================
// SDL2UnitInfoPanel ??? right-click unit tooltip
// ============================================================================

SDL2UnitInfoPanel::SDL2UnitInfoPanel() {
    m_fontPath = EnResolveFontPath();   // T-0073: one shared resolver, bundled font first
}

SDL2UnitInfoPanel::~SDL2UnitInfoPanel() {
    Hide();
    if (m_bgGold) SDL_FreeSurface(m_bgGold);
    if (m_borderH) SDL_FreeSurface(m_borderH);
    if (m_borderV) SDL_FreeSurface(m_borderV);
    if (m_matIcons) SDL_FreeSurface(m_matIcons);
    if (m_vehIcons) SDL_FreeSurface(m_vehIcons);
    for (auto& p : m_fontCache)
        if (p.second) TTF_CloseFont(p.second);
}

void SDL2UnitInfoPanel::LoadArt() {
    if (m_artLoaded) return;
    m_artLoaded = true;

    // DIB_GOLD = 24 (gold background), DIB_BORDER_HORZ = 10, DIB_BORDER_VERT = 11
    CDIB* pGold = theBitmaps.GetByIndex(DIB_GOLD);
    if (pGold) m_bgGold = SDL2MainMenu::CreateSurfaceFromDIB(pGold);

    CDIB* pH = theBitmaps.GetByIndex(DIB_BORDER_HORZ);
    if (pH) m_borderH = SDL2MainMenu::CreateSurfaceFromDIB(pH);

    CDIB* pV = theBitmaps.GetByIndex(DIB_BORDER_VERT);
    if (pV) m_borderV = SDL2MainMenu::CreateSurfaceFromDIB(pV);

    // Material icons (barrel/log/etc.) ??? the original drew these in the unit info
    // box instead of spelling out the material name. The ICON_MATERIALS strip holds
    // one icon per material type, laid out left-to-right at srcX = type * m_cxIcon
    // (see CUnit::PaintStatusMaterials in the legacy new_unit.cpp).
    CStatData* pMat = theIcons.GetByIndex(ICON_MATERIALS);
    if (pMat && pMat->m_pcDib) {
        m_matIcons = SDL2MainMenu::CreateSurfaceFromDIB(pMat->m_pcDib);
        m_matIconW = pMat->m_cxIcon;
        m_matIconH = pMat->m_cyIcon;
    }

    // Vehicle icons — DIB_LIST_UNIT_VEHICLES is a vertical strip of 64px tiles,
    // one per CTransportData::TRANS_TYPE (srcY = vehType * 64). Used to draw the
    // little unit icons for vehicles parked inside a building (seaport/factory),
    // matching the original game's hover display.
    CDIB* pVeh = theBitmaps.GetByIndex(DIB_LIST_UNIT_VEHICLES);
    if (pVeh) {
        m_vehIcons = SDL2MainMenu::CreateSurfaceFromDIB(pVeh);
        m_vehTileW = m_vehIcons ? m_vehIcons->w : 0;
        m_vehTileH = 64;
    }
}

TTF_Font* SDL2UnitInfoPanel::GetFont(int size) {
    if (m_fontPath.empty()) return nullptr;
    auto it = m_fontCache.find(size);
    if (it != m_fontCache.end()) return it->second;
    TTF_Font* f = TTF_OpenFont(m_fontPath.c_str(), size);
    m_fontCache[size] = f;
    return f;
}

void SDL2UnitInfoPanel::Show(CUnit* pUnit, int screenX, int screenY) {
    if (!pUnit || !theApp.m_gameWindow || !theApp.m_gameWindow->GetCompositor())
        return;

    LoadArt();
    m_pUnit = pUnit;
    BuildContent();

    int borderH = m_borderH ? m_borderH->h : 3;
    int borderV = m_borderV ? m_borderV->w : 3;
    int w = 240;
    int h = borderH * 2 + 8;
    for (auto& line : m_lines) h += LineHeight(line);

    // Clamp to screen
    int scrW = theApp.m_gameWindow->GetWidth();
    int scrH = theApp.m_gameWindow->GetHeight();
    if (screenX + w > scrW) screenX = scrW - w;
    if (screenY + h > scrH) screenY = scrH - h;
    if (screenX < 0) screenX = 0;
    if (screenY < 0) screenY = 0;

    // Remove old panel if size changed
    if (m_panel) {
        SDL_Surface* surf = m_panel->GetSurface();
        if (surf && (surf->w != w || surf->h != h)) {
            theApp.m_gameWindow->GetCompositor()->RemovePanel(m_panel);
            m_panel = nullptr;
        }
    }

    if (m_panel) {
        m_panel->SetPosition(screenX, screenY);
        m_panel->SetVisible(true);
    } else {
        m_panel = theApp.m_gameWindow->GetCompositor()->AddPanel(
            "unit_info", screenX, screenY, w, h, 40);
        m_panel->SetMovable(false);
        m_panel->SetResizable(false);
        m_panel->SetClosable(false);
        m_panel->SetTitle("");
    }

    Render();
}

void SDL2UnitInfoPanel::Hide() {
    if (m_panel) {
        m_panel->SetVisible(false);
    }
    m_pUnit = nullptr;
}

bool SDL2UnitInfoPanel::IsVisible() const {
    return m_panel && m_panel->IsVisible();
}

void SDL2UnitInfoPanel::Update() {
    if (!m_pUnit || !m_panel || !m_panel->IsVisible()) return;
    BuildContent();
    Render();
}

void SDL2UnitInfoPanel::BuildContent() {
    m_lines.clear();
    if (!m_pUnit) return;

    // Unit name
    m_lines.push_back({m_pUnit->GetData()->GetDesc().c_str(), false});

    // Vehicle transport status
    if (m_pUnit->GetUnitType() == CUnit::vehicle) {
        CVehicle* pVeh = (CVehicle*)m_pUnit;
        if (pVeh->GetData()->IsTransport()) {
            std::string sText;
            if (!pVeh->IsHpControl())
                sText = CTransportData::m_sAuto.c_str();
            else if (pVeh->GetEvent() == CVehicle::route)
                sText = CTransportData::m_sRoute.c_str();

            CBuilding* pBldg = theBuildingHex.GetBuilding(pVeh->GetPtHead());
            if (pBldg == NULL || pVeh->GetHexOwnership())
                pBldg = theBuildingHex.GetBuilding(pVeh->GetHexDest());
            if (pBldg != NULL && pBldg->GetOwner()->IsMe())
                sText += std::string("[") + pBldg->GetData()->GetDesc().c_str() + "]";
            else if (pVeh->GetRouteMode() == CVehicle::stop)
                sText += CTransportData::m_sIdle.c_str();
            else
                sText += CTransportData::m_sTravel.c_str();

            m_lines.push_back({sText, false});
        }
    }

    // Damage
    int dmg = __min(99, 100 - m_pUnit->GetDamagePer());
    m_lines.push_back({"Damage: " + std::to_string(dmg) + "%", m_pUnit->GetDamagePer() < 50});

    // Building status text
    if (m_pUnit->GetUnitType() == CUnit::building) {
        CBuilding* pBldg = (CBuilding*)m_pUnit;
        switch (pBldg->GetData()->GetUnionType()) {
        case CStructureData::UTvehicle:
        case CStructureData::UThousing:
        case CStructureData::UTpower:
        case CStructureData::UTresearch:
        case CStructureData::UTrepair:
        case CStructureData::UTmine:
        case CStructureData::UTfarm:
        case CStructureData::UTshipyard: {
            std::string sText;
            pBldg->ShowStatusText(sText);
            m_lines.push_back({sText, m_pUnit->GetDamagePer() < 50});
            break;
        }
        }
    }

    // Materials
    for (int i = 0; i < CMaterialTypes::GetNumTypes(); i++) {
        int iNeed = 0;
        if (i < CMaterialTypes::GetNumBuildTypes() && m_pUnit->GetUnitType() == CUnit::building)
            iNeed = ((CBuilding*)m_pUnit)->GetBldgResReq(i, FALSE);
        if (m_pUnit->GetStore(i) != 0 || iNeed > 0) {
            // The original game showed the material's icon (a barrel of oil, a log,
            // etc.) instead of its name, followed by the stored amount. When the icon
            // strip is available we carry the material index and emit just the number;
            // Render() blits the icon. Fall back to the text label if art is missing.
            std::string s;
            if (m_matIcons)
                s = std::to_string(m_pUnit->GetStore(i));
            else
                s = CMaterialTypes::GetDesc(i) + ": " + std::to_string(m_pUnit->GetStore(i));
            if (iNeed > 0)
                s += " (" + std::to_string(iNeed) + ")";
            m_lines.push_back({s, false, m_matIcons ? i : -1});
        }
    }

    // Carrier cargo
    if (m_pUnit->GetUnitType() == CUnit::vehicle) {
        POSITION pos = ((CVehicle*)m_pUnit)->GetCargoHeadPosition();
        while (pos != NULL) {
            CVehicle* pVeh = ((CVehicle*)m_pUnit)->GetCargoNext(pos);
            m_lines.push_back({pVeh->GetData()->GetDesc().c_str(), false});
        }
    }

    // Vehicles inside building
    if (m_pUnit->GetUnitType() == CUnit::building) {
        POSITION pos = theVehicleMap.GetStartPosition();
        while (pos != NULL) {
            DWORD dwID;
            CVehicle* pVeh;
            theVehicleMap.GetNextAssoc(pos, dwID, pVeh);
            if (pVeh->GetOwner()->IsMe() && !pVeh->GetHexOwnership() &&
                theBuildingHex._GetBuilding(pVeh->GetPtHead()) == m_pUnit) {
                // Lead the line with the vehicle's icon (DIB_LIST_UNIT_VEHICLES row
                // = vehType) when the sheet is available, matching the original
                // game's hover display; fall back to text-only if art is missing.
                int vt = m_vehIcons ? (int)pVeh->GetData()->GetType() : -1;
                m_lines.push_back({pVeh->GetData()->GetDesc().c_str(), false, -1, vt});
            }
        }
    }
}

void SDL2UnitInfoPanel::Render() {
    if (!m_panel) return;
    SDL_Surface* dst = m_panel->GetSurface();
    if (!dst) return;

    int w = dst->w, h = dst->h;
    int borderH = m_borderH ? m_borderH->h : 3;
    int borderV = m_borderV ? m_borderV->w : 3;

    // --- Gold background (stretched, matching original PaintBorder) ---
    if (m_bgGold) {
        SDL_Rect sr = {0, 0, m_bgGold->w, m_bgGold->h};
        SDL_Rect dr = {0, 0, w, h};
        SDL_BlitScaled(m_bgGold, &sr, dst, &dr);
    } else {
        SDL_FillRect(dst, nullptr, SDL_MapRGB(dst->format, 160, 140, 90));
    }

    // --- Carved-gold frame (shared corner-correct routine) ---
    SDL2MainMenu::DrawGoldBorder(dst, 0, 0, w, h, m_borderH, m_borderV);

    // --- Text (matching original: gray shadow at +1,+1, then black or red) ---
    TTF_Font* font = GetFont(11);
    if (!font) { m_panel->SetDirty(); return; }

    int textX = borderV + 4;
    int y = borderH + 4;

    int fontH = TTF_FontHeight(font);

    for (auto& line : m_lines) {
        int rowHt = LineHeight(line);
        if (line.text.empty()) { y += rowHt; continue; }

        // Baseline so text is vertically centered within the (possibly taller) row.
        int textY = y + (rowHt - fontH) / 2;

        // Material lines lead with the resource icon (barrel, log, ???) in place of
        // the name; the text following it is just the quantity. Reserve a fixed
        // icon-width gutter so the numbers line up across rows.
        int lineTextX = textX;
        if (line.matIdx >= 0 && m_matIcons && m_matIconW > 0) {
            SDL_Rect isr = {line.matIdx * m_matIconW, 0, m_matIconW, m_matIconH};
            SDL_Rect idr = {textX, textY + (fontH - m_matIconH) / 2, m_matIconW, m_matIconH};
            SDL_SetSurfaceBlendMode(m_matIcons, SDL_BLENDMODE_BLEND);
            SDL_BlitSurface(m_matIcons, &isr, dst, &idr);
            lineTextX = textX + m_matIconW + 4;
        } else if (line.vehIcon >= 0 && m_vehIcons && m_vehTileW > 0) {
            // Vehicle icon: scale the 64px tile down into a square box, preserving
            // aspect, and vertically center it in the taller row.
            int srcY = line.vehIcon * m_vehTileH;
            if (srcY + m_vehTileH <= m_vehIcons->h) {
                int dh = VEH_ICON_SZ;
                int dw = m_vehTileW * dh / m_vehTileH;
                if (dw > VEH_ICON_SZ) { dw = VEH_ICON_SZ; dh = m_vehTileH * dw / m_vehTileW; }
                SDL_Rect isr = {0, srcY, m_vehTileW, m_vehTileH};
                SDL_Rect idr = {textX + (VEH_ICON_SZ - dw) / 2, y + (rowHt - dh) / 2, dw, dh};
                SDL_SetSurfaceBlendMode(m_vehIcons, SDL_BLENDMODE_BLEND);
                SDL_BlitScaled(m_vehIcons, &isr, dst, &idr);
            }
            lineTextX = textX + VEH_ICON_SZ + 4;
        }
        int lineMaxW = w - lineTextX - (borderV + 4);

        SDL_Color shadow = {128, 128, 128, 255};
        SDL_Color fg = line.red ? SDL_Color{255, 50, 27, 255} : SDL_Color{0, 0, 0, 255};

        SDL_Surface* ts = TTF_RenderUTF8_Blended(font, line.text.c_str(), shadow);
        if (ts) {
            SDL_Rect sr = {0, 0, __min(ts->w, lineMaxW), ts->h};
            SDL_Rect dr = {lineTextX + 1, textY + 1, sr.w, sr.h};
            SDL_BlitSurface(ts, &sr, dst, &dr);
            SDL_FreeSurface(ts);
        }
        ts = TTF_RenderUTF8_Blended(font, line.text.c_str(), fg);
        if (ts) {
            SDL_Rect sr = {0, 0, __min(ts->w, lineMaxW), ts->h};
            SDL_Rect dr = {lineTextX, textY, sr.w, sr.h};
            SDL_BlitSurface(ts, &sr, dst, &dr);
            SDL_FreeSurface(ts);
        }
        y += rowHt;
    }

    m_panel->SetDirty();
}

// ============================================================================
// SDL2ChatWindow ??? multiplayer chat
// ============================================================================

SDL2ChatWindow::SDL2ChatWindow(GameWindow* gw)
    : SDL2Dialog(gw, "Chat", 350, 300)
{}

void SDL2ChatWindow::OnInit() {
    // Message history
    m_msgList = AddWidget<SDL2Listbox>(m_x + 10, m_y + 36, m_width - 20, 200);

    // Input field
    AddWidget<SDL2Label>(m_x + 10, m_y + 240, 50, 24, "Say:");
    m_editMsg = AddWidget<SDL2EditBox>(m_x + 60, m_y + 240, m_width - 130, 24);

    AddWidget<SDL2Button>(m_x + m_width - 60, m_y + 240, 50, 24, "Send",
        [this]() { OnSend(); });

    // Mail hub (compose / inbox) ??? the comms window is the entry point for both
    // chat and email, matching the original CWndComm. Show the unread count.
    int unread = SDL2Mail_UnreadCount();
    std::string mailLabel = unread > 0 ? ("Mail (" + std::to_string(unread) + ")") : "Mail";
    AddWidget<SDL2Button>(m_x + m_width / 2 - 90, m_y + m_height - 36, 80, 28, mailLabel.c_str(),
        [this]() { SDL2Comms_OpenMail(m_gameWindow); });

    AddWidget<SDL2Button>(m_x + m_width / 2 + 10, m_y + m_height - 36, 80, 28, "Close",
        [this]() { EndDialog(0); });

    RefreshMessages();
}

void SDL2ChatWindow::OnFrame() {
    RefreshMessages();
}

void SDL2ChatWindow::OnSend() {
    if (!m_editMsg) return;
    std::string text = m_editMsg->GetText();
    if (text.empty()) return;

    // Research gating: in-game chat requires Telephone research (parity with the
    // original ??? CPlayer::CanChat()). The pre-game lobby chat is a separate path
    // and stays ungated.
    CPlayer* pMe = theGame.GetMe();
    if (pMe && !pMe->CanChat()) {
        SDL2Chat_AddMessage("(Chat requires Telephone research)");
        m_editMsg->SetText("");
        RefreshMessages();
        return;
    }

    SDL2Chat_Send(text);   // broadcast to others + local echo (shared helper)

    m_editMsg->SetText("");
    RefreshMessages();
}

void SDL2ChatWindow::RefreshMessages() {
    if (!m_msgList) return;
    int count = (int)g_chatMessages.size();
    if (count == m_lastMsgCount) return;
    m_lastMsgCount = count;

    m_msgList->Clear();
    // Show the most recent N lines that fit
    int start = __max(0, count - 50);
    for (int i = start; i < count; i++)
        m_msgList->AddItem(g_chatMessages[i]);
    // Auto-scroll to the newest line: select the last item AND scroll the view
    // to it. SetSelected alone only moves the highlight — it does not move the
    // scroll window, so a full listbox stayed pinned at the top on each new
    // message. EnsureVisible scrolls the newest line into view.
    if (count > 0) {
        int last = m_msgList->GetCount() - 1;
        m_msgList->SetSelected(last);
        m_msgList->EnsureVisible(last);
    }
}

// ============================================================================
// SDL2PlayerListDialog ??? in-game player list
// ============================================================================

SDL2PlayerListDialog::SDL2PlayerListDialog(GameWindow* gw)
    : SDL2Dialog(gw, "Players", 360, 320)
{}

void SDL2PlayerListDialog::OnInit() {
    m_list = AddWidget<SDL2Listbox>(m_x + 10, m_y + 36, 200, 240);
    m_lblInfo = AddWidget<SDL2Label>(m_x + 220, m_y + 36, 130, 240, "");
    m_lblInfo->SetWrapped(true);

    AddWidget<SDL2Button>(m_x + m_width / 2 - 40, m_y + m_height - 36, 80, 28, "Close",
        [this]() { EndDialog(0); });

    PopulateList();

    // Show info for all players in the info label
    std::string allInfo;
    POSITION pos2 = theGame.GetAll().GetHeadPosition();
    while (pos2 != NULL) {
        CPlayer* pPlr = theGame.GetAll().GetNext(pos2);
        if (!pPlr) continue;
        allInfo += (const char*)pPlr->GetName();
        if (pPlr->IsMe()) allInfo += " (You)";
        else if (pPlr->IsAI()) allInfo += " (AI)";
        allInfo += "\n";
    }
    m_lblInfo->SetText(allInfo);
}

void SDL2PlayerListDialog::PopulateList() {
    if (!m_list) return;
    m_list->Clear();

    POSITION pos = theGame.GetAll().GetHeadPosition();
    while (pos != NULL) {
        CPlayer* pPlr = theGame.GetAll().GetNext(pos);
        if (pPlr)
            m_list->AddItem((const char*)pPlr->GetName());
    }
}

// ============================================================================
// SDL2ComposeDialog ??? multiplayer message compose
// ============================================================================

SDL2ComposeDialog::SDL2ComposeDialog(GameWindow* gw)
    : SDL2Dialog(gw, "Compose Message", 400, 430)
{}

SDL2ComposeDialog::SDL2ComposeDialog(GameWindow* gw, int toPlyr,
                                     const std::string& subject, const std::string& body)
    : SDL2Dialog(gw, "Compose Message", 400, 430)
    , m_preToPlyr(toPlyr)
    , m_preSubject(subject)
    , m_preBody(body)
{}

void SDL2ComposeDialog::OnInit() {
    int lx = m_x + 10;
    int y = m_y + 36;
    int w = m_width - 20;

    AddWidget<SDL2Label>(lx, y, 80, 20, "To:");
    m_recipientList = AddWidget<SDL2Listbox>(lx + 30, y, w - 30, 80);

    // Populate recipients ??? human opponents only (you can't email AI/yourself).
    // Track each row's plyr num so OnSend can address the message.
    int preselectRow = -1;
    POSITION pos = theGame.GetAll().GetHeadPosition();
    while (pos != NULL) {
        CPlayer* pPlr = theGame.GetAll().GetNext(pos);
        if (pPlr && !pPlr->IsMe() && !pPlr->IsAI()) {
            if (m_preToPlyr >= 0 && pPlr->GetPlyrNum() == m_preToPlyr)
                preselectRow = (int)m_recipientPlyrs.size();
            m_recipientPlyrs.push_back(pPlr->GetPlyrNum());
            m_recipientList->AddItem((const char*)pPlr->GetName());
        }
    }
    if (preselectRow >= 0)
        m_recipientList->SetSelected(preselectRow);
    else if (m_recipientPlyrs.size() == 1)
        m_recipientList->SetSelected(0);   // only one possible recipient ??? preselect it

    y += 90;
    AddWidget<SDL2Label>(lx, y, 80, 20, "Subject:");
    m_editSubject = AddWidget<SDL2EditBox>(lx + 60, y, w - 60, 22, m_preSubject);

    y += 30;
    AddWidget<SDL2Label>(lx, y, 80, 20, "Message:");
    m_editBody = AddWidget<SDL2EditBox>(lx, y + 22, w, 150, m_preBody);
    m_editBody->SetMultiline(true);   // Enter inserts newlines; body can be paragraphs

    int btnW = 80, btnH = 28, by = m_y + m_height - 40;
    AddWidget<SDL2Button>(m_x + m_width / 2 - btnW - 10, by, btnW, btnH, "Send",
        [this]() { OnSend(); });
    AddWidget<SDL2Button>(m_x + m_width / 2 + 10, by, btnW, btnH, "Cancel",
        [this]() { EndDialog(0); });
}

void SDL2ComposeDialog::OnOK() {
    // Enter from the (single-line) Subject field sends; the multi-line Message
    // field consumes Enter itself (newline) and never reaches here.
    OnSend();
}

void SDL2ComposeDialog::OnSend() {
    CPlayer* pMe = theGame.GetMe();
    if (!pMe || !m_recipientList || !m_editBody) { EndDialog(0); return; }

    if (!theGame.IsNetGame()) {
        EnMessageBox("Email can only be sent in a network game.", MB_OK);
        return;
    }

    // Research gating (parity): sending mail needs Mail or E-Mail research.
    if (!pMe->CanEMail() && !pMe->CanDelayMail()) {
        EnMessageBox("Research Mail or E-Mail before sending messages.", MB_OK | MB_ICONINFORMATION);
        return;
    }

    int row = m_recipientList->GetSelected();
    if (row < 0 || row >= (int)m_recipientPlyrs.size()) {
        EnMessageBox("Select a recipient first.", MB_OK);
        return;   // keep the dialog open so the user can pick someone
    }

    // Build the email and hand it to the same network path the original used
    // (CMsgIPC::PostToClient serializes subject+body and PostClientToClient's it
    // to the recipient). Receipt is handled by SDL2Mail_HandleIncoming.
    CMsgIPC msg(CMsgIPC::email);
    msg.m_iFrom    = pMe->GetPlyrNum();
    msg.m_iTo      = m_recipientPlyrs[row];
    msg.m_sSubject = m_editSubject ? m_editSubject->GetText().c_str() : "";
    msg.m_sMessage = m_editBody->GetText().c_str();
    msg.PostToClient();

    EndDialog(1);
}

// ============================================================================
// SDL2MailDialog ??? inbox + reader (replaces CWndComm mail hub + CWndMailRead)
// ============================================================================

SDL2MailDialog::SDL2MailDialog(GameWindow* gw)
    : SDL2Dialog(gw, "Mail", 560, 400)
{}

void SDL2MailDialog::OnInit() {
    int lx = m_x + 12, y = m_y + 40;
    int listW = 200;
    int bottomY = m_y + m_height - 50;
    int listH = bottomY - y;

    m_list = AddWidget<SDL2Listbox>(lx, y, listW, listH,
        [this](int idx) { m_sel = idx; ShowSelected(); },
        nullptr);

    int rx = lx + listW + 14;
    int rw = m_x + m_width - 12 - rx;
    SDL_Color blue{ 48, 58, 148, 255 };
    m_lblFrom    = AddWidget<SDL2Label>(rx, y,      rw, 20, "", blue);
    m_lblSubject = AddWidget<SDL2Label>(rx, y + 24, rw, 20, "", blue);
    m_lblBody    = AddWidget<SDL2Label>(rx, y + 52, rw, listH - 56, "", blue);
    m_lblBody->SetWrapped(true);
    m_lblBody->SetTopAligned(true);

    // Action buttons along the bottom ??? Compose / Reply / Forward / Delete / Close.
    int by = m_y + m_height - 42, bw = 86, bh = 28, gap = 6, bx = lx;
    AddWidget<SDL2Button>(bx, by, bw, bh, "Compose", [this] { OnCompose(); }); bx += bw + gap;
    AddWidget<SDL2Button>(bx, by, bw, bh, "Reply",   [this] { OnReply();   }); bx += bw + gap;
    AddWidget<SDL2Button>(bx, by, bw, bh, "Forward", [this] { OnForward(); }); bx += bw + gap;
    AddWidget<SDL2Button>(bx, by, bw, bh, "Delete",  [this] { OnDelete();  }); bx += bw + gap;
    AddWidget<SDL2Button>(bx, by, bw, bh, "Close",   [this] { EndDialog(0); });

    RefreshList();
}

void SDL2MailDialog::RefreshList() {
    if (!m_list) return;
    m_list->Clear();
    for (const auto& m : g_mailInbox) {
        // Unread messages get a leading marker, matching the inbox read-state idea.
        std::string label = (m.read ? "   " : ">  ") + m.subject + "  (" + m.fromName + ")";
        m_list->AddItem(label);
    }
    if (m_sel >= 0 && m_sel < (int)g_mailInbox.size())
        m_list->SetSelected(m_sel);
    ShowSelected();
}

void SDL2MailDialog::ShowSelected() {
    if (m_sel < 0 || m_sel >= (int)g_mailInbox.size()) {
        if (m_lblFrom)    m_lblFrom->SetText("");
        if (m_lblSubject) m_lblSubject->SetText("");
        if (m_lblBody)    m_lblBody->SetText(g_mailInbox.empty() ? "(no messages)" : "");
        return;
    }
    SDL2MailMsg& m = g_mailInbox[m_sel];
    m.read = true;   // viewing marks it read
    if (m_lblFrom)    m_lblFrom->SetText("From: " + m.fromName);
    if (m_lblSubject) m_lblSubject->SetText("Subject: " + m.subject);
    if (m_lblBody)    m_lblBody->SetText(m.body);
}

void SDL2MailDialog::OnFrame() {
    // New mail can arrive (SDL2Mail_HandleIncoming) while this window is open ???
    // refresh the list when the inbox size changes.
    if ((int)g_mailInbox.size() != m_lastInboxCount) {
        m_lastInboxCount = (int)g_mailInbox.size();
        RefreshList();
    }
}

void SDL2MailDialog::OnCompose() {
    SDL2Comms_OpenCompose(m_gameWindow, -1, "", "");
}

void SDL2MailDialog::OnReply() {
    if (m_sel < 0 || m_sel >= (int)g_mailInbox.size()) return;
    SDL2MailMsg& m = g_mailInbox[m_sel];
    std::string subj = (m.subject.rfind("Re:", 0) == 0) ? m.subject : ("Re: " + m.subject);
    std::string quoted = "\n\n--- " + m.fromName + " wrote ---\n" + m.body;
    SDL2Comms_OpenCompose(m_gameWindow, m.fromPlyr, subj, quoted);
}

void SDL2MailDialog::OnForward() {
    if (m_sel < 0 || m_sel >= (int)g_mailInbox.size()) return;
    SDL2MailMsg& m = g_mailInbox[m_sel];
    std::string subj = (m.subject.rfind("Fwd:", 0) == 0) ? m.subject : ("Fwd: " + m.subject);
    std::string fwd  = "--- Forwarded from " + m.fromName + " ---\n" + m.body;
    SDL2Comms_OpenCompose(m_gameWindow, -1, subj, fwd);
}

void SDL2MailDialog::OnDelete() {
    if (m_sel < 0 || m_sel >= (int)g_mailInbox.size()) return;
    g_mailInbox.erase(g_mailInbox.begin() + m_sel);
    if (m_sel >= (int)g_mailInbox.size())
        m_sel = (int)g_mailInbox.size() - 1;
    RefreshList();
}

// ============================================================================
// SDL2CutSceneDialog ??? win/lose/scenario text screen
// ============================================================================

// cut=0, repeat=1, scenario_end=2, win=3, lose=4
static bool CutIsEndScreen(int typ) { return typ == 2 || typ == 3 || typ == 4; }

SDL2CutSceneDialog::SDL2CutSceneDialog(GameWindow* gw, int typ, const std::string& text, int scenario)
    : SDL2Dialog(gw, "", gw->GetWidth(), gw->GetHeight())
    , m_typ(typ)
    , m_text(text)
    , m_scenario(scenario)
{
    // No title bar text ??? the content IS the message. EVERY cut scene renders
    // full-screen + chromeless: a full-screen painting with text over it. End screens
    // use the WN/LS/DN paintings (OnInitEndScreen); the scenario INTRO uses the CS
    // painting (OnInitIntro) ??? restoring the original CWndCutScene look.
}

SDL_Surface* SDL2CutSceneDialog::LoadEndArt(int typ) {
    // Pick the painting chunk that CWndCutScene::OnCreate uses (CUTSCENE.CPP):
    //   WN=win, LS=lose, DN="done"/scenario-end, CS=scenario INTRO (cut/repeat).
    // Each is keyed by the current bit depth ("08"/"24"); if that mode's chunk is
    // missing, fall back to the tiled WL wallpaper.
    const char* tag = (typ == 3) ? "WN" : (typ == 4) ? "LS" : (typ == 2) ? "DN" : "CS";

    CMmio* pMmio = nullptr;
    SDL_Surface* surf = nullptr;
    try {
        pMmio = theDataFile.OpenAsMMIO("misc", "MISC");
        if (!pMmio) return nullptr;
        pMmio->DescendRiff('M', 'I', 'S', 'C');
        try {
            pMmio->DescendList(tag[0], tag[1], theApp.m_szOtherBPS[0], theApp.m_szOtherBPS[1]);
            pMmio->DescendChunk('D', 'A', 'T', 'A');
        } catch (...) {
            delete pMmio;
            pMmio = theDataFile.OpenAsMMIO("misc", "MISC");
            pMmio->DescendRiff('M', 'I', 'S', 'C');
            pMmio->DescendList('W', 'L', theApp.m_szOtherBPS[0], theApp.m_szOtherBPS[1]);
            pMmio->DescendChunk('D', 'A', 'T', 'A');
        }

        CDIB* pDib = new CDIB(ptrthebltformat->GetColorFormat(), CBLTFormat::DIB_MEMORY,
                              ptrthebltformat->GetMemDirection());
        pDib->Load(*pMmio);
        surf = SDL2MainMenu::CreateSurfaceFromDIB(pDib);
        delete pDib;
    } catch (...) {
        surf = nullptr;
    }
    delete pMmio;
    return surf;
}

void SDL2CutSceneDialog::OnInitEndScreen() {
    // Full-screen painting behind the message (mirrors CWndCutScene::OnCreate +
    // OnPaint). If the art can't load we fall back to a black background.
    if (SDL_Surface* bg = LoadEndArt(m_typ))
        SetFullscreenBackground(bg, true);

    // High-contrast title. The original's muted two-tone palette (e.g. lose =
    // dark-red 57,0,0 over brown) was nearly illegible on a busy painting, so use
    // a BRIGHT fill with a solid BLACK outline halo instead ??? readable over anything.
    SDL_Color fg = (m_typ == 4) ? SDL_Color{255,  80,  70, 255}   // lose: bright red
                                : SDL_Color{255, 226, 110, 255};  // win/scenario: bright gold
    SDL_Color outline = {0, 0, 0, 255};

    // Single-line title, auto-shrunk to ~85% of the screen width (the original
    // auto-sized the Milford font the same way).
    int fontSize = m_height / 11;
    while (fontSize > 16) {
        TTF_Font* f = GetFont(fontSize);
        int tw = 0, th = 0;
        if (f) TTF_SizeUTF8(f, m_text.c_str(), &tw, &th);
        if (!f || tw <= (m_width * 85) / 100) break;
        fontSize -= 2;
    }

    // Centered at the top quarter. Draw the text 8 times in black around the centre
    // (a 3px outline) so it stays readable on any background, then the bright fill on top.
    int topY = m_y + m_height / 4;
    int rowH = fontSize * 2;
    const int o = 3;
    const int dx[8] = { -o, -o, -o,  0,  0,  o,  o,  o };
    const int dy[8] = { -o,  0,  o, -o,  o, -o,  0,  o };
    for (int k = 0; k < 8; k++) {
        SDL2Label* s = AddWidget<SDL2Label>(m_x + dx[k], topY + dy[k], m_width, rowH, m_text, outline);
        s->SetCentered(true); s->SetTopAligned(true); s->SetFontSize(fontSize);
    }
    SDL2Label* hi = AddWidget<SDL2Label>(m_x, topY, m_width, rowH, m_text, fg);
    hi->SetCentered(true); hi->SetTopAligned(true); hi->SetFontSize(fontSize);

    // Continue button, lower-right ??? much larger than the original tiny 150x40 (per
    // user: 2-4x bigger) with a font to match, anchored off the bottom edge.
    int btnW = 380, btnH = 120;
    int btnX = m_x + m_width - btnW - m_width / 12;
    int btnY = m_y + m_height - btnH - m_height / 10;
    auto* btn = AddWidget<SDL2Button>(btnX, btnY, btnW, btnH, "Continue", [this]() { OnOK(); });
    btn->SetFontSize(40);
}

void SDL2CutSceneDialog::OnInit() {
    if (CutIsEndScreen(m_typ))
        OnInitEndScreen();
    else
        OnInitIntro();
}

// Scenario INTRO (cut / repeat) ??? full-screen CS painting with the briefing text on
// the right third and OK/Cancel/Save along the lower-left. Mirrors the original
// CWndCutScene::OnCreateCut / OnPaintCut layout.
void SDL2CutSceneDialog::OnInitIntro() {
    // Full-screen scenario painting (MISC 'CS' chunk) behind the text. Black fallback.
    if (SDL_Surface* bg = LoadEndArt(m_typ))
        SetFullscreenBackground(bg, true);

    // Briefing text in the RIGHT THIRD (original: x from w*2/3 to w*95/100), word-
    // wrapped, drawn yellow over a black drop-shadow (original drew black then yellow
    // offset by 2px). Font auto-scaled like the original's ~28*h/1280 Milford.
    const int textX = m_x + (m_width * 2) / 3;
    const int textR = m_x + (m_width * 95) / 100;
    const int textW = textR - textX;
    const int textY = m_y + m_height / 8;
    const int textH = (m_height * 3) / 4 - m_height / 8;
    const int fontSize = __max(14, (28 * m_height) / 1280);

    SDL_Color black  = {  0,   0,   0, 255};
    SDL_Color yellow = {255, 251, 120, 255};
    SDL2Label* sh = AddWidget<SDL2Label>(textX, textY, textW, textH, m_text, black);
    sh->SetWrapped(true); sh->SetTopAligned(true); sh->SetFontSize(fontSize);
    SDL2Label* hi = AddWidget<SDL2Label>(textX - 2, textY - 2, textW, textH, m_text, yellow);
    hi->SetWrapped(true); hi->SetTopAligned(true); hi->SetFontSize(fontSize);

    // Buttons along the lower-left (original placed OK/Cancel/Save there). Repeat scenes
    // get OK only; the first scenario (0) has no Save (matches OnCreateCut).
    const bool canCancel = (m_typ == 0);
    const bool canSave   = (m_typ == 0) && (m_scenario > 0);
    const int btnW = 160, btnH = 44, gap = 18;
    int bx = m_x + m_width / 18;
    const int by = m_y + (m_height * 72) / 100;
    AddWidget<SDL2Button>(bx, by, btnW, btnH, "OK", [this]() { OnOK(); }); bx += btnW + gap;
    if (canCancel) { AddWidget<SDL2Button>(bx, by, btnW, btnH, "Cancel", [this]() { OnCancel(); }); bx += btnW + gap; }
    if (canSave)   { AddWidget<SDL2Button>(bx, by, btnW, btnH, "Save",   [this]() { OnSave(); }); }
}

void SDL2CutSceneDialog::OnOK() {
    m_result = 1; // CUT_OK
    EndDialog(1);
}

void SDL2CutSceneDialog::OnCancel() {
    m_result = 2; // CUT_CANCEL
    EndDialog(2);
}

void SDL2CutSceneDialog::OnSave() {
    // Save game, then continue
    theGame.SaveGame( (CWnd*)NULL );
    m_result = 1; // CUT_OK (continue after save)
    EndDialog(1);
}
