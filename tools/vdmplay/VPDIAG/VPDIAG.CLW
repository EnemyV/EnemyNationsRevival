; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CTcpDlg
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "vpdiag.h"
LastPage=0

ClassCount=5
Class1=CVpdiagApp
Class2=CAboutDlg
Class3=CVpdiagDlg

ResourceCount=8
Resource1=IDD_NETBIOS_DIALOG
Resource2=IDD_VPDIAG_DIALOG (English (U.S.))
Resource3=IDD_TCP_DIALOG
Resource4=IDD_VPDIAG_DIALOG
Class4=CNbDlg
Class5=CTcpDlg
Resource5=IDD_ABOUTBOX (English (U.S.))
Resource6=IDD_ABOUTBOX
Resource7=IDD_NETBIOS_DIALOG (English (U.S.))
Resource8=IDD_TCP_DIALOG (English (U.S.))

[CLS:CVpdiagApp]
Type=0
BaseClass=CWinApp
HeaderFile=vpdiag.h
ImplementationFile=vpdiag.cpp

[CLS:CAboutDlg]
Type=0
BaseClass=CDialog
HeaderFile=vpdiagdl.cpp
ImplementationFile=vpdiagdl.cpp
LastObject=CAboutDlg

[CLS:CVpdiagDlg]
Type=0
BaseClass=CDialog
HeaderFile=vpdiagdl.h
ImplementationFile=vpdiagdl.cpp
LastObject=IDC_TESTIPX
Filter=D
VirtualFilter=dWC





[DLG:IDD_VPDIAG_DIALOG (English (U.S.))]
Type=1
Class=CVpdiagDlg
ControlCount=11
Control1=IDC_TCPHOSTNAME,edit,1350631552
Control2=IDC_TCPADDRESS,edit,1350631552
Control3=IDC_IPXADDRESS,edit,1350631552
Control4=IDC_STATIC,static,1342308354
Control5=IDC_STATIC,static,1342308354
Control6=IDC_STATIC,static,1342308354
Control7=IDC_LANALIST,listbox,1352745219
Control8=IDC_STATIC,static,1342308354
Control9=IDC_TESTTCP,button,1342242816
Control10=IDC_TESTIPX,button,1342242816
Control11=IDC_TESTNETBIOS,button,1342242816

[DLG:IDD_VPDIAG_DIALOG]
Type=1
Class=CVpdiagDlg
ControlCount=11
Control1=IDC_TCPHOSTNAME,edit,1350631552
Control2=IDC_TCPADDRESS,edit,1350631552
Control3=IDC_IPXADDRESS,edit,1350631552
Control4=IDC_STATIC,static,1342308354
Control5=IDC_STATIC,static,1342308354
Control6=IDC_STATIC,static,1342308354
Control7=IDC_LANALIST,listbox,1352745219
Control8=IDC_STATIC,static,1342308354
Control9=IDC_TESTTCP,button,1342242816
Control10=IDC_TESTIPX,button,1342242816
Control11=IDC_TESTNETBIOS,button,1342242816

[DLG:IDD_NETBIOS_DIALOG]
Type=1
Class=CNbDlg
ControlCount=7
Control1=IDOK,button,1342242817
Control2=IDC_LIST2,listbox,1352745217
Control3=IDC_SERVER,button,1342242819
Control4=IDC_LANAS,listbox,1352728835
Control5=IDC_SAVE,button,1342242816
Control6=IDC_CLOSE,button,1342242816
Control7=IDC_STATIC,static,1342308352

[CLS:CNbDlg]
Type=0
HeaderFile=NbDlg.h
ImplementationFile=NbDlg.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=IDC_SERVER

[DLG:IDD_TCP_DIALOG]
Type=1
Class=CTcpDlg
ControlCount=11
Control1=IDOK,button,1342242817
Control2=IDC_REMOTEADDRESS,edit,1350631552
Control3=IDC_LOCALADDRESS,edit,1350633600
Control4=IDC_SERVER,button,1342242819
Control5=IDC_MESSAGES,listbox,1352728833
Control6=IDC_PORT,edit,1350631552
Control7=IDC_BROADCAST,button,1342242819
Control8=IDC_STATIC,static,1342308354
Control9=IDC_STATIC,static,1342308354
Control10=IDC_STATIC,static,1342308354
Control11=IDC_DOECHO,button,1476460547

[CLS:CTcpDlg]
Type=0
HeaderFile=TcpDlg.h
ImplementationFile=TcpDlg.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=CTcpDlg

[DLG:IDD_ABOUTBOX (English (U.S.))]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[DLG:IDD_ABOUTBOX]
Type=1
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[DLG:IDD_NETBIOS_DIALOG (English (U.S.))]
Type=1
ControlCount=7
Control1=IDOK,button,1342242817
Control2=IDC_LIST2,listbox,1352745217
Control3=IDC_SERVER,button,1342242819
Control4=IDC_LANAS,listbox,1352728835
Control5=IDC_SAVE,button,1342242816
Control6=IDC_CLOSE,button,1342242816
Control7=IDC_STATIC,static,1342308352

[DLG:IDD_TCP_DIALOG (English (U.S.))]
Type=1
ControlCount=11
Control1=IDOK,button,1342242817
Control2=IDC_REMOTEADDRESS,edit,1350631552
Control3=IDC_LOCALADDRESS,edit,1350633600
Control4=IDC_SERVER,button,1342242819
Control5=IDC_MESSAGES,listbox,1352728833
Control6=IDC_PORT,edit,1350631552
Control7=IDC_BROADCAST,button,1342242819
Control8=IDC_STATIC,static,1342308354
Control9=IDC_STATIC,static,1342308354
Control10=IDC_STATIC,static,1342308354
Control11=IDC_DOECHO,button,1476460547

