; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CStatusDlg
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "sprite.h"
LastPage=0

ClassCount=20
Class1=CBD
Class2=cdirdlg
Class3=CHoldTime
Class4=CMainFrame
Class5=CMultiProgress
Class6=CNamingDlg
Class7=CPalDoc
Class8=CPalView
Class9=CProgressDlg
Class10=CSplitWnd
Class11=CSPProp
Class12=CSprite
Class13=CSpriteApp
Class14=CAboutDlg
Class15=CSpriteView
Class16=SpriteTreeCtrl
Class17=CTD
Class18=CTView
Class19=CVD

ResourceCount=9
Resource1=IDD_SPPROP_PAGE
Resource2=IDD_PROGRESS2
Resource3=IDR_MAINFRAME
Resource4=IDD_ABOUTBOX
Resource5=IDD_PROGRESSDLG
Resource6=IDD_DIRDIALOG
Resource7=IDD_NAMING
Resource8=IDD_HOLDTIME
Class20=CStatusDlg
Resource9=IDD_DLG_ERR

[CLS:CBD]
Type=0
BaseClass=CSprite
HeaderFile=BD.H
ImplementationFile=BD.CPP

[CLS:cdirdlg]
Type=0
BaseClass=CDialog
HeaderFile=CDIRDLG.H
ImplementationFile=CDIRDLG.CPP

[CLS:CHoldTime]
Type=0
BaseClass=CDialog
HeaderFile=HOLDTIME.H
ImplementationFile=HOLDTIME.CPP

[CLS:CMainFrame]
Type=0
BaseClass=CMDIFrameWnd
HeaderFile=MAINFRM.H
ImplementationFile=MAINFRM.CPP

[CLS:CMultiProgress]
Type=0
BaseClass=CDialog
HeaderFile=MULTPROG.H
ImplementationFile=Multprog.cpp

[CLS:CNamingDlg]
Type=0
BaseClass=CDialog
HeaderFile=NAMNGDLG.H
ImplementationFile=NAMNGDLG.CPP

[CLS:CPalDoc]
Type=0
BaseClass=CDocument
HeaderFile=PALDOC.H
ImplementationFile=PALDOC.CPP

[CLS:CPalView]
Type=0
BaseClass=CView
HeaderFile=PALVIEW.H
ImplementationFile=PALVIEW.CPP

[CLS:CProgressDlg]
Type=0
BaseClass=CDialog
HeaderFile=PROGDLG.H
ImplementationFile=PROGDLG.CPP

[CLS:CSplitWnd]
Type=0
BaseClass=CMDIChildWnd
HeaderFile=SPLITWND.H
ImplementationFile=SPLITWND.CPP

[CLS:CSPProp]
Type=0
BaseClass=CPropertyPage
HeaderFile=SPPROP.H
ImplementationFile=SPPROP.CPP

[CLS:CSprite]
Type=0
BaseClass=CDocument
HeaderFile=SPRD.H
ImplementationFile=SPRD.CPP

[CLS:CSpriteApp]
Type=0
BaseClass=CWinApp
HeaderFile=SPRITE.H
ImplementationFile=SPRITE.CPP

[CLS:CAboutDlg]
Type=0
BaseClass=CDialog
HeaderFile=SPRITE.CPP
ImplementationFile=SPRITE.CPP
LastObject=IDC_LIST_ERR

[CLS:CSpriteView]
Type=0
BaseClass=CScrollView
HeaderFile=SPRITVW.H
ImplementationFile=SPRITVW.CPP

[CLS:SpriteTreeCtrl]
Type=0
BaseClass=CTreeCtrl
HeaderFile=SPTRCTRL.H
ImplementationFile=SPTRCTRL.CPP

[CLS:CTD]
Type=0
BaseClass=CSprite
HeaderFile=TD.H
ImplementationFile=TD.CPP

[CLS:CTView]
Type=0
BaseClass=CScrollView
HeaderFile=TREEVIEW.H
ImplementationFile=TREEVIEW.CPP

[CLS:CVD]
Type=0
BaseClass=CSprite
HeaderFile=VD.H
ImplementationFile=VD.CPP

[DLG:IDD_DIRDIALOG]
Type=1
Class=cdirdlg
ControlCount=4
Control1=IDC_DIRLIST,listbox,1352728835
Control2=IDOK,button,1342242817
Control3=IDCANCEL,button,1342242816
Control4=IDC_DRIVELIST,combobox,1344340227

[DLG:IDD_HOLDTIME]
Type=1
Class=CHoldTime
ControlCount=4
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_HT_EDIT,edit,1350631552
Control4=IDC_STATIC,static,1342308352

[DLG:IDD_PROGRESS2]
Type=1
Class=CMultiProgress
ControlCount=7
Control1=IDCANCEL,button,1342242816
Control2=IDC_STATIC,static,1342308352
Control3=IDC_CURRENTDIR,edit,1350633600
Control4=IDC_STATIC,static,1342308352
Control5=IDC_CURRENTFILE,edit,1350633600
Control6=IDC_FILEPROGRESS,msctls_progress32,1350565888
Control7=IDC_DIRPROGRESS,msctls_progress32,1350565888

[DLG:IDD_NAMING]
Type=1
Class=CNamingDlg
ControlCount=46
Control1=IDC_STATIC,button,1342308359
Control2=IDC_NODAMAGE,button,1342177289
Control3=IDC_MINORDAMAGE,button,1342177289
Control4=IDC_HEAVYDAMAGE,button,1342177289
Control5=IDC_DESTROYED,button,1342177289
Control6=IDC_STATIC,button,1342308359
Control7=IDC_NORTHEAST,button,1342177289
Control8=IDC_EAST,button,1342177289
Control9=IDC_SOUTHEAST,button,1342177289
Control10=IDC_SOUTH,button,1342177289
Control11=IDC_SOUTHWEST,button,1342177289
Control12=IDC_WEST,button,1342177289
Control13=IDC_NORTHWEST,button,1342177289
Control14=IDC_NORTH,button,1342177289
Control15=IDC_STATIC,button,1342308359
Control16=IDC_FOUNDATION,button,1342177289
Control17=IDC_SKELETON,button,1342177289
Control18=IDC_CONSTRUCTION,button,1342177289
Control19=IDC_DONE,button,1342177289
Control20=IDC_STATIC,button,1342308359
Control21=IDC_OVERLAY,button,1342177289
Control22=IDC_BASE,button,1342177289
Control23=IDC_ANIM1,button,1342177289
Control24=IDC_ANIM2,button,1342177289
Control25=IDC_UNDERANIM1,button,1342177289
Control26=IDC_UNDERANIM2,button,1342177289
Control27=IDC_STATIC,static,1342308352
Control28=IDC_STATIC,static,1342308352
Control29=IDC_STATIC,static,1342308352
Control30=IDC_STATIC,static,1342308352
Control31=IDC_STATIC,static,1342308352
Control32=IDC_BASECNT,edit,1350631552
Control33=IDC_ANIM1CNT,edit,1350631552
Control34=IDC_ANIM2CNT,edit,1350635648
Control35=IDC_UNDERANIM1CNT,edit,1350635648
Control36=IDC_UNDERANIM2CNT,edit,1350635648
Control37=IDC_STATIC,button,1342308359
Control38=IDC_FILENAME,edit,1350635648
Control39=IDC_STATIC,button,1342308359
Control40=IDC_FLAT,button,1342177289
Control41=IDC_TILTTOWARD,button,1342177289
Control42=IDC_TILTAWAY,button,1342177289
Control43=IDC_STATIC,button,1342308359
Control44=IDC_TWOPIECEBACK,button,1342177289
Control45=IDC_TWOPIECEFORE,button,1342177289
Control46=IDC_ONEPIECE,button,1342177289

[DLG:IDD_PROGRESSDLG]
Type=1
Class=CProgressDlg
ControlCount=2
Control1=IDCANCEL,button,1342242816
Control2=IDC_PROGRESS,msctls_progress32,1350565888

[DLG:IDD_SPPROP_PAGE]
Type=1
Class=CSPProp
ControlCount=10
Control1=IDC_STATIC,static,1342308352
Control2=IDC_STATIC,static,1342308352
Control3=IDC_STATIC,static,1342308352
Control4=IDC_STATIC,static,1342308352
Control5=IDC_SPPROP_XOFF,edit,1350631552
Control6=IDC_SPPROP_WIDTH,edit,1350633600
Control7=IDC_SPPROP_HOLD,edit,1350631552
Control8=IDC_STATIC,static,1342308352
Control9=IDC_SPPROP_YOFF,edit,1350631552
Control10=IDC_SPPROP_HEIGHT,edit,1350633600

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342308352
Control2=IDC_STATIC,static,1342308352
Control3=IDOK,button,1342373889
Control4=IDC_STATIC,static,1342308352

[DLG:IDD_DLG_ERR]
Type=1
Class=CStatusDlg
ControlCount=2
Control1=IDC_LIST_ERR,listbox,1353777408
Control2=IDOK,button,1342242817

[MNU:IDR_MAINFRAME]
Type=1
Class=?
Command1=ID_FILE_CREATESPRITE
Command2=ID_FILE_CREATETERRAIN
Command3=ID_FILE_CREATEVEHICLE
Command4=ID_FILE_CREATEMULTIPLEBUILDINGS
Command5=ID_FILE_CREATEMULTIPLETERRAIN
Command6=ID_FILE_CREATEMULTIPLEVEHICLES
Command7=ID_FILE_CREATESINGLEDIB
Command8=ID_FILE_CREATEMULTIPLEDIBS
Command9=ID_FILE_CREATESHAREWAREBUILDINGS
Command10=ID_FILE_CREATESHAREWARETERRAIN
Command11=ID_FILE_CREATESHAREWAREVEHICLES
Command12=ID_FILE_LOADSPRITE
Command13=ID_FILE_SAVESPRITE
Command14=ID_FILE_CREATEPALETTE
Command15=ID_FILE_LOADPALETTE
Command16=ID_FILE_VIEWPALETTE
Command17=ID_NAMEDLG
Command18=ID_APP_EXIT
Command19=ID_VIEW_TOOLBAR
Command20=ID_VIEW_STATUS_BAR
Command21=ID_APP_ABOUT
CommandCount=21

[ACL:IDR_MAINFRAME]
Type=1
Class=?
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_FILE_PRINT
Command5=ID_EDIT_UNDO
Command6=ID_EDIT_CUT
Command7=ID_EDIT_COPY
Command8=ID_EDIT_PASTE
Command9=ID_EDIT_UNDO
Command10=ID_EDIT_CUT
Command11=ID_EDIT_COPY
Command12=ID_EDIT_PASTE
Command13=ID_NEXT_PANE
Command14=ID_PREV_PANE
CommandCount=14

[CLS:CStatusDlg]
Type=0
HeaderFile=StatusDlg.h
ImplementationFile=StatusDlg.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=IDOK

