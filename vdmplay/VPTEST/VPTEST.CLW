; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CJoinDialog
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "vptest.h"
LastPage=0

ClassCount=7
Class1=CVptestApp
Class2=CVptestDoc
Class3=CVptestView
Class4=CMainFrame

ResourceCount=5
Resource1=IDD_ABOUTBOX
Resource2=IDR_MAINFRAME
Resource3=IDR_VPTESTTYPE
Class5=CAboutDlg
Resource4=IDD_SENDMSGDIALOG
Class6=CSendMsgDlg
Class7=CJoinDialog
Resource5=IDD_JOINDIALOG

[CLS:CVptestApp]
Type=0
HeaderFile=vptest.h
ImplementationFile=vptest.cpp
Filter=N

[CLS:CVptestDoc]
Type=0
HeaderFile=vptesdoc.h
ImplementationFile=vptesdoc.cpp
Filter=N

[CLS:CVptestView]
Type=0
HeaderFile=vptesvw.h
ImplementationFile=vptesvw.cpp
Filter=C
VirtualFilter=VWC
LastObject=ID_VDMPLAY_STARTUP

[CLS:CMainFrame]
Type=0
HeaderFile=mainfrm.h
ImplementationFile=mainfrm.cpp
Filter=T



[CLS:CAboutDlg]
Type=0
HeaderFile=vptest.cpp
ImplementationFile=vptest.cpp
Filter=D

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308352
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[MNU:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_MRU_FILE1
Command4=ID_APP_EXIT
Command5=ID_APP_ABOUT
CommandCount=5

[MNU:IDR_VPTESTTYPE]
Type=1
Class=CVptestView
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_CLOSE
Command4=ID_FILE_SAVE
Command5=ID_FILE_SAVE_AS
Command6=ID_FILE_MRU_FILE1
Command7=ID_APP_EXIT
Command8=ID_EDIT_UNDO
Command9=ID_EDIT_CUT
Command10=ID_EDIT_COPY
Command11=ID_EDIT_PASTE
Command12=ID_WINDOW_NEW
Command13=ID_WINDOW_CASCADE
Command14=ID_WINDOW_TILE_HORZ
Command15=ID_WINDOW_ARRANGE
Command16=ID_VDMPLAY_STARTUP
Command17=ID_VDMPLAY_CLEANUP
Command18=ID_VDMPLAY_CREATESESSION
Command19=ID_VDMPLAY_JOINSESSION
Command20=ID_VDMPLAY_ENUMSESSIONS
Command21=ID_VDMPLAY_SENDMESSAGE
Command22=ID_VDMPLAY_CLOSESESSION
Command23=ID_APP_ABOUT
CommandCount=23

[ACL:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_EDIT_UNDO
Command5=ID_EDIT_CUT
Command6=ID_EDIT_COPY
Command7=ID_EDIT_PASTE
Command8=ID_EDIT_UNDO
Command9=ID_EDIT_CUT
Command10=ID_EDIT_COPY
Command11=ID_EDIT_PASTE
Command12=ID_NEXT_PANE
Command13=ID_PREV_PANE
CommandCount=13

[DLG:IDD_SENDMSGDIALOG]
Type=1
ControlCount=5
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_MESSAGETEXT,edit,1350631556
Control4=IDC_STATIC,static,1342308352
Control5=IDC_PLAYERID,edit,1350631552
Class=CSendMsgDlg

[CLS:CSendMsgDlg]
Type=0
HeaderFile=sendmsgd.h
ImplementationFile=sendmsgd.cpp
Filter=D
VirtualFilter=dWC

[DLG:IDD_JOINDIALOG]
Type=1
Class=CJoinDialog
ControlCount=6
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_STATIC,static,1342308352
Control4=IDC_STATIC,static,1342308352
Control5=IDC_PLAYERNAME,edit,1350631552
Control6=IDC_SESSIONNAME,edit,1350631552

[CLS:CJoinDialog]
Type=0
HeaderFile=joindial.h
ImplementationFile=joindial.cpp
Filter=D
VirtualFilter=dWC

