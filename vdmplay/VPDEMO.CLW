; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CVpdemoDlg
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "vpdemo.h"

ClassCount=5
Class1=CVpdemoApp
Class2=CVpdemoDlg
Class3=CAboutDlg

ResourceCount=10
Resource1=IDD_ABOUTBOX
Resource2=IDR_MAINFRAME
Resource3=IDD_VPDEMO_DIALOG
Resource4=IDR_MAIN_MENU
Resource5=IDD_TCPIPX
Class4=CTcpIpxDlg
Resource6=IDD_NETBIOS
Class5=CNbDlg
Resource7=IDD_NETBIOS (English (U.S.))
Resource8=IDD_TCPIPX (English (U.S.))
Resource9=IDD_ABOUTBOX (English (U.S.))
Resource10=IDD_VPDEMO_DIALOG (English (U.S.))

[CLS:CVpdemoApp]
Type=0
HeaderFile=vpdemo.h
ImplementationFile=vpdemo.cpp
Filter=N
VirtualFilter=AC
LastObject=ID_PROTOCOL_ADVANCED_CL
BaseClass=CWinApp

[CLS:CVpdemoDlg]
Type=0
HeaderFile=vpdemdlg.h
ImplementationFile=vpdemdlg.cpp
Filter=D
VirtualFilter=dWC
LastObject=IDC_LOADTEST
BaseClass=CDialog

[CLS:CAboutDlg]
Type=0
HeaderFile=vpdemdlg.h
ImplementationFile=vpdemdlg.cpp
Filter=D
LastObject=CAboutDlg

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308352
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[DLG:IDD_VPDEMO_DIALOG]
Type=1
Class=CVpdemoDlg
ControlCount=17
Control1=IDOK,button,1342242817
Control2=IDC_SESSIONLIST,listbox,1352728835
Control3=IDC_PLAYERLIST,listbox,1352728835
Control4=IDC_CREATESESSION,button,1342242816
Control5=IDC_PLAYERNAME,edit,1350631552
Control6=IDC_JOIN,button,1342242816
Control7=IDC_TEXT,edit,1350631552
Control8=IDC_SENDMSG,button,1342242816
Control9=IDC_EVENTLOG,listbox,1353777409
Control10=IDC_STATICP,static,1342308352
Control11=IDC_STATICS,static,1342308352
Control12=IDC_RESCAN,button,1342242816
Control13=IDC_STATIC,static,1342308352
Control14=IDC_STATIC,static,1342308352
Control15=IDC_MUSTDELIVER,button,1342242819
Control16=IDC_BROADCAST,button,1342242819
Control17=IDC_SENDBIG,button,1342242816

[MNU:IDR_MAIN_MENU]
Type=1
Command1=IDM_PROTOCOL_TCP
Command2=IDM_PROTOCOL_IPX
Command3=IDM_PROTOCOL_NETBIOS
Command4=ID_PROTOCOL_COMM
Command5=ID_PROTOCOL_MODEM
Command6=ID_PROTOCOL_TAPI
Command7=ID_PROTOCOL_DP
Command8=ID_PROTOCOL_ADVANCED_CL
Command9=ID_PROTOCOL_ADVANCED_SRV
Command10=ID_PROTOCOL_VPSTARTUP
Command11=ID_PROTOCOL_VPENUMSES
CommandCount=11

[DLG:IDD_TCPIPX]
Type=1
Class=CTcpIpxDlg
ControlCount=6
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_STATIC,static,1342308352
Control4=IDC_PORT,edit,1350631552
Control5=IDC_STATIC,static,1342308352
Control6=IDC_SERVER_ADDRESS,edit,1350631552

[CLS:CTcpIpxDlg]
Type=0
HeaderFile=tcpipxdl.h
ImplementationFile=tcpipxdl.cpp
Filter=D
VirtualFilter=dWC
LastObject=CTcpIpxDlg

[DLG:IDD_NETBIOS]
Type=1
Class=CNbDlg
ControlCount=6
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_STATIC,static,1342308352
Control4=IDC_LANA,edit,1350631552
Control5=IDC_STATIC,static,1342308352
Control6=IDC_STATION_NAME,edit,1350631552

[CLS:CNbDlg]
Type=0
HeaderFile=nbdlg.h
ImplementationFile=nbdlg.cpp
Filter=D
VirtualFilter=dWC
LastObject=CNbDlg

[DLG:IDD_TCPIPX (English (U.S.))]
Type=1
Class=CTcpIpxDlg
ControlCount=6
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_STATIC,static,1342308352
Control4=IDC_PORT,edit,1350631552
Control5=IDC_STATIC,static,1342308352
Control6=IDC_SERVER_ADDRESS,edit,1350631552

[DLG:IDD_ABOUTBOX (English (U.S.))]
Type=1
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308352
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[DLG:IDD_VPDEMO_DIALOG (English (U.S.))]
Type=1
Class=CVpdemoDlg
ControlCount=18
Control1=IDOK,button,1342242817
Control2=IDC_SESSIONLIST,listbox,1352728835
Control3=IDC_PLAYERLIST,listbox,1352728835
Control4=IDC_CREATESESSION,button,1342242816
Control5=IDC_PLAYERNAME,edit,1350631552
Control6=IDC_JOIN,button,1342242816
Control7=IDC_TEXT,edit,1350631552
Control8=IDC_SENDMSG,button,1342242816
Control9=IDC_EVENTLOG,listbox,1353777409
Control10=IDC_STATICP,static,1342308352
Control11=IDC_STATICS,static,1342308352
Control12=IDC_RESCAN,button,1342242816
Control13=IDC_STATIC,static,1342308352
Control14=IDC_STATIC,static,1342308352
Control15=IDC_MUSTDELIVER,button,1342242819
Control16=IDC_BROADCAST,button,1342242819
Control17=IDC_SENDBIG,button,1342242816
Control18=IDC_LOADTEST,button,1342242816

[DLG:IDD_NETBIOS (English (U.S.))]
Type=1
ControlCount=6
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_STATIC,static,1342308352
Control4=IDC_LANA,edit,1350631552
Control5=IDC_STATIC,static,1342308352
Control6=IDC_STATION_NAME,edit,1350631552

