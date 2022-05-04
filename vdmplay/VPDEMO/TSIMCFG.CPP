// TSimCfg.cpp : implementation file
//

#include "stdafx.h"
#include "vpdemo.h"
#include "TSimCfg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void vpFetchString (LPCSTR section, LPCSTR key, LPCSTR defVal, LPSTR pStr, int iLen);
UINT vpFetchInt(LPCSTR section, LPCSTR key, UINT defVal);
UINT vpFetchIntR(LPCSTR section, LPCSTR key, UINT defVal, UINT minVal, UINT maxVal);
void vpStoreInt(LPCSTR section, LPCSTR key, UINT val);
void vpStoreIntD(LPCSTR section, LPCSTR key, UINT val, UINT defVal);

void vpStoreString(LPCSTR section, LPCSTR key, LPCSTR val);
void vpMakeIniFile(LPSTR fName);

/////////////////////////////////////////////////////////////////////////////
// TSimCfg dialog


TSimCfg::TSimCfg(CWnd* pParent /*=NULL*/)
	: CDialog(TSimCfg::IDD, pParent)
{
	//{{AFX_DATA_INIT(TSimCfg)
	m_ackDelay = 0;
	m_comWinSize = 0;
	m_ipxIBufSize = 0;
	m_ipxOBufSize = 0;
	m_ncbQSize = 0;
	m_packetSize = 0;
	m_sendsPerBurst = 0;
	m_sendsPerTick = 0;
	m_tcpIBufSize = 0;
	m_tcpOBufSize = 0;
	m_tickDivisor = 0;
	m_tcpFrag = FALSE;
	m_tickFreq = 0;
	m_tcpFlowSim = FALSE;
	//}}AFX_DATA_INIT
}


void TSimCfg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(TSimCfg)
	DDX_Text(pDX, IDC_ACKDELAY, m_ackDelay);
	DDX_Text(pDX, IDC_COMWINSIZE, m_comWinSize);
	DDX_Text(pDX, IDC_IPXIBUFSIZE, m_ipxIBufSize);
	DDX_Text(pDX, IDC_IPXOBUFSIZE, m_ipxOBufSize);
	DDX_Text(pDX, IDC_NCBQUESIZE, m_ncbQSize);
	DDX_Text(pDX, IDC_PACKETSIZE, m_packetSize);
	DDX_Text(pDX, IDC_SENDSPERBURST, m_sendsPerBurst);
	DDX_Text(pDX, IDC_SENDSPERTICK, m_sendsPerTick);
	DDX_Text(pDX, IDC_TCPIBUFSIZE, m_tcpIBufSize);
	DDX_Text(pDX, IDC_TCPOBUFSIZE, m_tcpOBufSize);
	DDX_Text(pDX, IDC_TICKDIVISOR, m_tickDivisor);
	DDX_Check(pDX, IDC_TCPFRAG, m_tcpFrag);
	DDX_Text(pDX, IDC_TICKFREQ, m_tickFreq);
	DDX_Check(pDX, IDC_TCPFLOW, m_tcpFlowSim);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(TSimCfg, CDialog)
	//{{AFX_MSG_MAP(TSimCfg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// TSimCfg message handlers

void TSimCfg::OnOK() 
{
	// TODO: Add extra validation here

	UpdateData(TRUE);

	vpStoreInt("VPDEMO", "ackTimeout", m_ackDelay);
	vpStoreInt("COMM", "WinSize", m_comWinSize);
	vpStoreIntD("IPX", "IBufSize", m_ipxIBufSize,0);
	vpStoreIntD("IPX", "OBufSize", m_ipxOBufSize,0);
	vpStoreInt("NETBIOS", "NCBQSize", m_ncbQSize);

	vpStoreInt("VPDEMO", "PacketSize", m_packetSize);

	vpStoreInt("VPDEMO", "sendsPerTick", m_sendsPerTick); 
	vpStoreInt("VPDEMO", "sendsPerBurst", m_sendsPerBurst); 

	vpStoreIntD("TCP", "IBufSize", m_tcpIBufSize,0);


	vpStoreIntD("TCP", "OBufSize", m_tcpOBufSize,0);


	vpStoreIntD("VPDEMO", "TickDivisor", m_tickDivisor, 1);
	vpStoreInt("TCP", "SimFrag", m_tcpFrag);
	vpStoreInt("TCP", "SimFlow", m_tcpFlowSim);
	vpStoreInt("VPDEMO", "LoadTestTimer", m_tickFreq);
	
	CDialog::OnOK();
}

BOOL TSimCfg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here

	m_ackDelay = 	vpFetchIntR("VPDEMO", "ackTimeout", 25000, 10, 60000);

	m_comWinSize = vpFetchIntR("COMM", "WinSize", 4, 4, 64);

	m_ipxIBufSize = vpFetchIntR("IPX", "IBufSize", 0, 0, 64);
	m_ipxOBufSize = vpFetchIntR("IPX", "OBufSize", 0, 0, 64);
	m_ncbQSize =  vpFetchIntR("NETBIOS", "NCBQSize", 4, 1, 64);

	m_packetSize = vpFetchIntR("VPDEMO", "PacketSize", 64, 64, VP_MAXSENDDATA);

	m_sendsPerTick = vpFetchIntR("VPDEMO", "sendsPerTick", 1, 1, 10); 
	m_sendsPerBurst = vpFetchIntR("VPDEMO", "sendsPerBurst", 25, 1, 256); 

	m_tcpIBufSize = vpFetchIntR("TCP", "IBufSize", 0, 0, 64);


	m_tcpOBufSize =  vpFetchIntR("TCP", "OBufSize", 0, 0, 64);


	m_tickDivisor = vpFetchIntR("VPDEMO", "TickDivisor", 1, 1, 50);
	m_tcpFrag = vpFetchInt("TCP", "SimFrag", 0);
	m_tcpFlowSim = vpFetchInt("TCP", "SimFlow", 0);
	m_tickFreq = vpFetchIntR("VPDEMO", "LoadTestTimer", 100, 50, 1000);

	UpdateData(FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
