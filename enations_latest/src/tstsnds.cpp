// tstsnds.cpp : implementation file
//

#include "stdafx.h"
#include "lastplnt.h"
#include "tstsnds.h"
#include "sfx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgTestSounds dialog


CDlgTestSounds::CDlgTestSounds(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgTestSounds::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgTestSounds)
	m_strDesc = _T("");
	m_strFormat = _T("");
	m_iTrack = 0;
	m_iMidi = 0;
	m_iGroup = 0;
	//}}AFX_DATA_INIT
}


void CDlgTestSounds::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgTestSounds)
	DDX_Control(pDX, IDC_PLAY_SHUTUP, m_btnShut);
	DDX_Control(pDX, IDC_PLAY_FORE, m_btnFore);
	DDX_Control(pDX, IDC_PLAY_EXCL, m_btnExcl);
	DDX_Control(pDX, IDC_PLAY_BACK, m_btnBack);
	DDX_Text(pDX, IDC_SFX_DESC, m_strDesc);
	DDX_Text(pDX, IDC_SFX_FORMAT, m_strFormat);
	DDX_Text(pDX, IDC_SFX_TRACK, m_iTrack);
	DDX_Radio(pDX, IDC_SFX_TYP_MIDI, m_iMidi);
	DDX_Radio(pDX, IDC_SFX_GROUP, m_iGroup);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgTestSounds, CDialog)
	//{{AFX_MSG_MAP(CDlgTestSounds)
	ON_BN_CLICKED(IDC_PLAY_BACK, OnPlayBack)
	ON_BN_CLICKED(IDC_PLAY_EXCL, OnPlayExcl)
	ON_BN_CLICKED(IDC_PLAY_FORE, OnPlayFore)
	ON_BN_CLICKED(IDC_SFX_GROUP, OnSfxGroup)
	ON_BN_CLICKED(IDC_SFX_GROUP2, OnSfxGroup)
	ON_BN_CLICKED(IDC_SFX_GROUP3, OnSfxGroup)
	ON_BN_CLICKED(IDC_PLAY_SHUTUP, OnPlayShutup)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgTestSounds message handlers

BOOL CDlgTestSounds::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	UpdateData (TRUE);

	// set the music type
	switch ( theMusicPlayer.GetMode () )
		{
		case CMusicPlayer::midi_only :
			m_iMidi = 0;
			m_strFormat = "11K8m, midi music";
			break;
		case CMusicPlayer::mixed :
			m_iMidi = 1;
			m_strFormat = "11K8m sfx && voices, 22K16s music";
			break;
		case CMusicPlayer::wav_only :
			m_iMidi = 2;
			m_strFormat = "22K16m sfx && voices, 22K16s music";
			break;
		}

	if ( theApp.HaveMultVoices () )
		m_strFormat += ", 2 voices";
	else
		m_strFormat += ", 1 voice";

	m_iGroup = 0;

	if ( theMusicPlayer.GetSoundVolume () == 0 )
		{
		GetDlgItem (IDC_SFX_GROUP)->EnableWindow (FALSE);
		GetDlgItem (IDC_SFX_GROUP2)->EnableWindow (FALSE);
		m_iGroup = 2;
		}
	if ( (theMusicPlayer.GetMode () == CMusicPlayer::midi_only) || (theMusicPlayer.GetMusicVolume () == 0) )
		GetDlgItem (IDC_SFX_GROUP3)->EnableWindow (FALSE);

	UpdateData (FALSE);
	((CButton *) GetDlgItem (IDC_SFX_TYP_MIDI + m_iMidi))->SetCheck (1);

	OnPlayShutup ();

	OnSfxGroup();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

int CDlgTestSounds::GetTrack ()
{

	UpdateData (TRUE);

	int iMax, iAdd;
	switch ( m_iGroup )
		{
		case 0 :
			iMax = SOUNDS::sounds_end;
			iAdd = 0;
			break;
		case 1 :
			iMax = VOICES::first_second_voice - VOICES::first_voice;
			if ( theApp.HaveMultVoices () )
				iMax += VOICES::num_second_voice;
			iAdd = VOICES::first_voice;
			break;
		case 2 :
			iMax = MUSIC::play_game - MUSIC::first_music + MUSIC::num_play_game;
			iAdd = MUSIC::first_music;
			break;
		}

	int iRtn = iAdd + ( m_iTrack = __minmax (0, iMax, m_iTrack) );

	m_iTrack ++;
	if (m_iTrack >= iMax)
		m_iTrack = 0;
	UpdateData (FALSE);

	return (iRtn);
}

void CDlgTestSounds::OnPlayBack() 
{
	
	theMusicPlayer.QueueBackgroundSound ( GetTrack () );
	theMusicPlayer.UpdateBackgroundSounds ();
}

void CDlgTestSounds::OnPlayExcl() 
{
	
	theMusicPlayer.PlayExclusiveMusic ( GetTrack () );
}

void CDlgTestSounds::OnPlayFore() 
{
	
	theMusicPlayer.PlayForegroundSound ( GetTrack (), FOREGROUND_PRI );
}

void CDlgTestSounds::OnPlayShutup() 
{
	
	theMusicPlayer.StopMidiMusic ();
	theMusicPlayer.EndExclusiveMusic ();
	theMusicPlayer.SoundsOff ();
}

void CDlgTestSounds::OnSfxGroup() 
{

	UpdateData (TRUE);
	
	switch ( m_iGroup )
		{
		case 0 :
			m_btnFore.EnableWindow (TRUE);
			m_btnBack.EnableWindow (TRUE);
			m_btnExcl.EnableWindow (FALSE);
			m_strDesc = IntToCString (SOUNDS::sounds_end) + " Sound Effects";
			break;
		case 1 : {
			m_btnFore.EnableWindow (TRUE);
			m_btnBack.EnableWindow (FALSE);
			m_btnExcl.EnableWindow (FALSE);
			int iNum = VOICES::first_second_voice - VOICES::first_voice;
			if ( theApp.HaveMultVoices () )
				iNum += VOICES::num_second_voice;
			m_strDesc = IntToCString (iNum) + " Voices";
			break; }
		case 2 :
			m_btnFore.EnableWindow (FALSE);
			m_btnBack.EnableWindow (FALSE);
			m_btnExcl.EnableWindow (TRUE);
			m_strDesc = IntToCString (MUSIC::play_game - MUSIC::first_music + MUSIC::num_play_game) + " WAV Clips";
			break;
		}

	m_iTrack = 0;

	UpdateData (FALSE);
}

