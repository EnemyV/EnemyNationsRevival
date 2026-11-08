; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CDlgSocket
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "iserve.h"
VbxHeaderFile=iserve.h
VbxImplementationFile=iserve.cpp
LastPage=0

ClassCount=4
Class1=CAboutDlg
Class2=CIserveDlg
Class3=CIserveApp

ResourceCount=4
Resource1=IDD_ABOUTBOX
Resource2=IDD_SOCKET
Resource3=IDD_ISERVE_DIALOG
Class4=CDlgSocket
Resource4=IDR_MAIN_MENU

[CLS:CAboutDlg]
Type=0
HeaderFile=iservdlg.cpp
ImplementationFile=iservdlg.cpp

[CLS:CIserveDlg]
Type=0
HeaderFile=iservdlg.h
ImplementationFile=iservdlg.cpp
Filter=D
VirtualFilter=dWC
LastObject=CIserveDlg

[CLS:CIserveApp]
Type=0
HeaderFile=iserve.h
ImplementationFile=iserve.cpp

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308352
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[DLG:IDD_ISERVE_DIALOG]
Type=1
Class=CIserveDlg
ControlCount=7
Control1=IDC_ServerList,listbox,1352728833
Control2=IDC_STATIC,static,1342308354
Control3=IDC_STATIC,static,1342308354
Control4=IDC_STATIC,static,1342308354
Control5=IDC_EDIT_NAME,edit,1350568064
Control6=IDC_EDIT_ADDR,edit,1350568064
Control7=IDC_EDIT_SOCKET,edit,1350568064

[MNU:IDR_MAIN_MENU]
Type=1
Class=?
Command1=IDC_START
Command2=IDC_STOP
Command3=IDC_EXIT
Command4=IDC_OPTIONS
Command5=IDC_HELP
Command6=IDC_ABOUT
CommandCount=6

[DLG:IDD_SOCKET]
Type=1
Class=CDlgSocket
ControlCount=3
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_EDIT_SOCKET,edit,1350631552

[CLS:CDlgSocket]
Type=0
HeaderFile=iservdlg.h
ImplementationFile=iservdlg.cpp
Filter=D
VirtualFilter=dWC
LastObject=CDlgSocket

