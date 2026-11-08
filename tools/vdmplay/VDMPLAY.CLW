; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CAdvComDlg
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "vdmplay.h"
VbxHeaderFile=vdmplay.h
VbxImplementationFile=vdmplay.cpp
LastPage=0

ClassCount=14
Class1=CMainFrame
Class2=CNbDlg
Class3=CTcpIpxDlg
Class4=CNBDlg
Class5=CIpxDlg
Class6=CTcpDlg
Class7=CMdmDlg

ResourceCount=18
Resource1=IDD_ADV_TCPIP
Resource2=IDD_ADV_IPX
Resource3=IDD_ADV_NETBIOS
Resource4=IDD_ADV_COMM
Resource5=IDD_ADV_MODEM
Resource6=IDD_ADV_MODEM_SETTINGS
Class8=CAdvComDlg
Class9=CAdvIpxDdlg
Class10=CAdvMdmDlg
Class11=CAdvMdmSetting
Class12=CAdvNbDlg
Class13=CAdvTcpDlg
Class14=CComStatDlg
Resource7=IDD_COMM_STAT
Resource8=IDD_ADV_TAPI
Resource9=IDD_ADV_DP (French (France))
Resource10=IDD_ADV_TCPIP (French (France))
Resource11=IDD_ADV_IPX (French (France))
Resource12=IDD_ADV_NETBIOS (French (France))
Resource13=IDD_ADV_MODEM (French (France))
Resource14=IDD_ADV_MODEM_SETTINGS (French (France))
Resource15=IDD_COMM_STAT (French (France))
Resource16=IDD_ADV_TAPI (French (France))
Resource17=IDD_ADV_COMM (French (France))
Resource18=IDD_ADV_DP

[CLS:CMainFrame]
Type=0
HeaderFile=mainfrm.h
ImplementationFile=mainfrm.cpp

[CLS:CNbDlg]
Type=0
HeaderFile=nbdlg.h
ImplementationFile=nbdlg.cpp

[CLS:CTcpIpxDlg]
Type=0
HeaderFile=tcpipxdl.h
ImplementationFile=tcpipxdl.cpp

[CLS:CNBDlg]
Type=0
HeaderFile=vdmplui.h
ImplementationFile=vdmplui.cpp

[CLS:CIpxDlg]
Type=0
HeaderFile=vdmplui.h
ImplementationFile=vdmplui.cpp

[CLS:CTcpDlg]
Type=0
HeaderFile=vdmplui.h
ImplementationFile=vdmplui.cpp

[CLS:CMdmDlg]
Type=0
HeaderFile=vdmplui.h
ImplementationFile=vdmplui.cpp

[DLG:IDD_NETBIOS]
Class=CNbDlg

[DLG:IDD_TCPIPX]
Class=CTcpIpxDlg

[DLG:IDD_IPX]
Class=CIpxDlg

[DLG:IDD_TCP]
Class=CTcpDlg

[DLG:IDD_MODEM]
Class=CMdmDlg

[DLG:IDD_ADV_TCPIP]
Type=1
Class=CAdvTcpDlg
ControlCount=7
Control1=IDC_STATIC,static,1342308354
Control2=IDC_IP_SERVER_ADDR,edit,1350631552
Control3=IDC_STATIC,static,1342308354
Control4=IDC_IP_SOCKET,edit,1350631552
Control5=IDOK,button,1342242817
Control6=IDCANCEL,button,1342242816
Control7=IDX_HELP,button,1342242816

[DLG:IDD_ADV_IPX]
Type=1
Class=CAdvIpxDdlg
ControlCount=9
Control1=IDC_STATIC,static,1342308354
Control2=IDC_IPX_NET_NUM,edit,1350631552
Control3=IDC_STATIC,static,1342308354
Control4=IDC_IPX_NODE_NUM,edit,1350631552
Control5=IDC_STATIC,static,1342308354
Control6=IDC_IPX_SOCKET,edit,1350631552
Control7=IDOK,button,1342242817
Control8=IDCANCEL,button,1342242816
Control9=IDX_HELP,button,1342242816

[DLG:IDD_ADV_NETBIOS]
Type=1
Class=CAdvNbDlg
ControlCount=7
Control1=65535,static,1342308354
Control2=IDC_NETBIOS_LANA,combobox,1344340227
Control3=65535,static,1342308354
Control4=IDC_NETBIOS_STATION,edit,1350631552
Control5=IDOK,button,1342242817
Control6=IDCANCEL,button,1342242816
Control7=IDX_HELP,button,1342242816

[DLG:IDD_ADV_COMM]
Type=1
Class=CAdvComDlg
ControlCount=5
Control1=65535,static,1342308354
Control2=IDC_DIRECT_PORT,combobox,1344340227
Control3=IDOK,button,1342242817
Control4=IDCANCEL,button,1342242816
Control5=IDX_HELP,button,1342242816

[DLG:IDD_ADV_MODEM]
Type=1
Class=CAdvMdmDlg
ControlCount=8
Control1=IDC_STATIC,static,1342308354
Control2=IDC_MODEM_PORT,combobox,1344340227
Control3=IDC_STATIC,static,1342308354
Control4=IDC_MODEM_NUMBER,edit,1350631552
Control5=IDC_MODEM_ADV,button,1342242816
Control6=IDX_HELP,button,1342242816
Control7=IDOK,button,1342242817
Control8=IDCANCEL,button,1342242816

[DLG:IDD_ADV_MODEM_SETTINGS]
Type=1
Class=CAdvMdmSetting
ControlCount=11
Control1=IDC_STATIC,static,1342308354
Control2=IDC_CLIENT_INIT,edit,1350631552
Control3=IDC_STATIC,static,1342308354
Control4=IDC_SERVER_INIT,edit,1350631552
Control5=IDC_STATIC,static,1342308354
Control6=IDC_DIAL_PREFIX,edit,1350631552
Control7=IDC_STATIC,static,1342308354
Control8=IDC_DIAL_SUFFIX,edit,1350762624
Control9=IDOK,button,1342242817
Control10=IDCANCEL,button,1342242816
Control11=IDX_HELP,button,1342242816

[DLG:IDD_COMM_STAT]
Type=1
Class=CComStatDlg
ControlCount=5
Control1=IDCANCEL,button,1342242816
Control2=IDX_HELP,button,1342242816
Control3=IDC_STATIC,static,1342308354
Control4=IDC_COMM_STATUS,edit,1350568064
Control5=IDC_COMM_TRACE,listbox,1352728833

[CLS:CAdvComDlg]
Type=0
HeaderFile=advdlg.h
ImplementationFile=advdlg.cpp
LastObject=65535
Filter=D
VirtualFilter=dWC

[CLS:CAdvIpxDdlg]
Type=0
HeaderFile=advdlg.h
ImplementationFile=advdlg.cpp
Filter=D
VirtualFilter=dWC
LastObject=IDX_HELP

[CLS:CAdvMdmDlg]
Type=0
HeaderFile=advdlg.h
ImplementationFile=advdlg.cpp
Filter=D
VirtualFilter=dWC
LastObject=IDOK

[CLS:CAdvMdmSetting]
Type=0
HeaderFile=advdlg.h
ImplementationFile=advdlg.cpp
Filter=D
VirtualFilter=dWC
LastObject=IDX_HELP

[CLS:CAdvNbDlg]
Type=0
HeaderFile=advdlg.h
ImplementationFile=advdlg.cpp
Filter=D
VirtualFilter=dWC
LastObject=IDX_HELP

[CLS:CAdvTcpDlg]
Type=0
HeaderFile=advdlg.h
ImplementationFile=advdlg.cpp
Filter=D
VirtualFilter=dWC
LastObject=IDX_HELP

[CLS:CComStatDlg]
Type=0
HeaderFile=comstatd.h
ImplementationFile=comstatd.cpp
Filter=D
LastObject=CComStatDlg
VirtualFilter=dWC
BaseClass=CDialog

[DLG:IDD_ADV_TAPI]
Type=1
ControlCount=7
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDX_HELP,button,1342242816
Control4=IDC_TAPI_DEVICE,combobox,1344339971
Control5=IDC_STATIC,static,1342308354
Control6=IDC_TAPI_NUMBER,edit,1350631552
Control7=IDC_TAPI_SETTINGS,button,1342242816

[DLG:IDD_ADV_COMM (French (France))]
Type=1
Class=CAdvComDlg
ControlCount=5
Control1=65535,static,1342308354
Control2=IDC_DIRECT_PORT,combobox,1344340227
Control3=IDOK,button,1342242817
Control4=IDCANCEL,button,1342242816
Control5=IDX_HELP,button,1342242816

[DLG:IDD_ADV_TCPIP (French (France))]
Type=1
ControlCount=7
Control1=IDC_STATIC,static,1342308354
Control2=IDC_IP_SERVER_ADDR,edit,1350631552
Control3=IDC_STATIC,static,1342308354
Control4=IDC_IP_SOCKET,edit,1350631552
Control5=IDOK,button,1342242817
Control6=IDCANCEL,button,1342242816
Control7=IDX_HELP,button,1342242816

[DLG:IDD_ADV_IPX (French (France))]
Type=1
ControlCount=9
Control1=IDC_STATIC,static,1342308354
Control2=IDC_IPX_NET_NUM,edit,1350631552
Control3=IDC_STATIC,static,1342308354
Control4=IDC_IPX_NODE_NUM,edit,1350631552
Control5=IDC_STATIC,static,1342308354
Control6=IDC_IPX_SOCKET,edit,1350631552
Control7=IDOK,button,1342242817
Control8=IDCANCEL,button,1342242816
Control9=IDX_HELP,button,1342242816

[DLG:IDD_ADV_NETBIOS (French (France))]
Type=1
ControlCount=7
Control1=65535,static,1342308354
Control2=IDC_NETBIOS_LANA,combobox,1344340227
Control3=65535,static,1342308354
Control4=IDC_NETBIOS_STATION,edit,1350631552
Control5=IDOK,button,1342242817
Control6=IDCANCEL,button,1342242816
Control7=IDX_HELP,button,1342242816

[DLG:IDD_ADV_MODEM (French (France))]
Type=1
ControlCount=8
Control1=IDC_STATIC,static,1342308354
Control2=IDC_MODEM_PORT,combobox,1344340227
Control3=IDC_STATIC,static,1342308354
Control4=IDC_MODEM_NUMBER,edit,1350631552
Control5=IDC_MODEM_ADV,button,1342242816
Control6=IDX_HELP,button,1342242816
Control7=IDOK,button,1342242817
Control8=IDCANCEL,button,1342242816

[DLG:IDD_ADV_MODEM_SETTINGS (French (France))]
Type=1
ControlCount=11
Control1=IDC_STATIC,static,1342308354
Control2=IDC_CLIENT_INIT,edit,1350631552
Control3=IDC_STATIC,static,1342308354
Control4=IDC_SERVER_INIT,edit,1350631552
Control5=IDC_STATIC,static,1342308354
Control6=IDC_DIAL_PREFIX,edit,1350631552
Control7=IDC_STATIC,static,1342308354
Control8=IDC_DIAL_SUFFIX,edit,1350762624
Control9=IDOK,button,1342242817
Control10=IDCANCEL,button,1342242816
Control11=IDX_HELP,button,1342242816

[DLG:IDD_COMM_STAT (French (France))]
Type=1
ControlCount=5
Control1=IDCANCEL,button,1342242816
Control2=IDX_HELP,button,1342242816
Control3=IDC_STATIC,static,1342308354
Control4=IDC_COMM_STATUS,edit,1350568064
Control5=IDC_COMM_TRACE,listbox,1352728833

[DLG:IDD_ADV_TAPI (French (France))]
Type=1
ControlCount=7
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDX_HELP,button,1342242816
Control4=IDC_TAPI_DEVICE,combobox,1344339971
Control5=IDC_STATIC,static,1342308354
Control6=IDC_TAPI_NUMBER,edit,1350631552
Control7=IDC_TAPI_SETTINGS,button,1342242816

[DLG:IDD_ADV_DP (French (France))]
Type=1
ControlCount=3
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_DP_PROVIDER,listbox,1352728835

[DLG:IDD_ADV_DP]
Type=1
ControlCount=3
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_DP_PROVIDER,listbox,1352728577

