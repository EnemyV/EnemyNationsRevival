========================================================================
       MICROSOFT FOUNDATION CLASS LIBRARY : VPDEMO
========================================================================


AppWizard has created this VPDEMO application for you.  This application
not only demonstrates the basics of using the Microsoft Foundation classes
but is also a starting point for writing your application.

This file contains a summary of what you will find in each of the files that
make up your VPDEMO application.


VPDEMO.MAK
    This project file is compatible with the Visual C++ development
	environment. 

    It is also compatible with the NMAKE program provided with Visual C++. 

    To build a debug version of the program from the MS-DOS prompt, type
nmake /f VPDEMO.MAK CFG="Win32 Debug"
    or to build a release version of the program, type
nmake /f VPDEMO.MAK CFG="Win32 Release"

VPDEMO.H
    This is the main header file for the application.  It includes other
    project specific headers (including RESOURCE.H) and declares the
    CVpdemoApp application class.

VPDEMO.CPP
    This is the main application source file that contains the application
    class CVpdemoApp.

VPDEMO.RC
    This is a listing of all of the Microsoft Windows resources that the
    program uses.  It includes the icons, bitmaps, and cursors that are stored
    in the RES subdirectory.  This file can be directly edited in the
    Visual C++ development environment.

RES\VPDEMO.ICO
    This is an icon file, which is used as the application's icon.  This
    icon is included by the main resource file VPDEMO.RC.

RES\VPDEMO.RC2
    This file contains resources that are not edited by the Visual C++
	development environment.  You should place all resources not
	editable by the resource editor in this file.


VPDEMO.CLW
    This file contains information used by ClassWizard to edit existing
    classes or add new classes.  ClassWizard also uses this file to store
    information needed to create and edit message maps and dialog data
    maps and to create prototype member functions.


/////////////////////////////////////////////////////////////////////////////

AppWizard creates one dialog class:

vpdemdlg.H, vpdemdlg.CPP - the dialog
    These files contain your CVpdemoDlg class.  This class defines
    the behavior of your application's main dialog.  The dialog's
    template is in VPDEMO.RC, which can be edited in the Visual C++
	development environment. 


/////////////////////////////////////////////////////////////////////////////
Other standard files:

STDAFX.H, STDAFX.CPP
    These files are used to build a precompiled header (PCH) file
    named VPDEMO.PCH and a precompiled types file named STDAFX.OBJ.

RESOURCE.H
    This is the standard header file, which defines new resource IDs.
    Visual C++ reads and updates this file.

/////////////////////////////////////////////////////////////////////////////
Other notes:

AppWizard uses "TODO:" to indicate parts of the source code you
should add to or customize.

/////////////////////////////////////////////////////////////////////////////

Usage instructions

On server machine(s)
 1) Launch vpdemo
 2) Enter session name in the Session name field
 3) Click on Create Sesion button
 4) Enter player name int the player name field
 5) Click on Add Player Button
 6) Repeat steps 4 and 5 couple of times


On client machines
  1) Launch
  2) Select a session in the session listbox
  3) Enter your player name int the Player Name field
  4) Clik on Join Session button


To send messages:

  1) Select a player in the Players list box
  2) Enter a message in the message text field
  3) Select broadcast and/or garanteed delivery as you wish
     If on server machine the Player Name field defines the originator of the message 
  4) Click on Send Message button
  5) Observe messages in the listbox at the botton of the window
      
     

Notes:
  On Windows/NT  multiple instances of the program run fine on the same machine
  On Win95 there is problem when running multiple instances, I don't know
   if its my problem or compiler or RTL problem. I'm using VC 2.0 currently 
    Maybe i should switch to VC 2.2
 Session and player names in the demo program are limited to 16 characters...


Good luck 
  Vadim.


This version supports IPX and TCP

The selection is in the vdmplay.ini section VPDEMO

[VPDEMO]
Transport=TCP
; or
; Transport=IPX

;;; The default is TCP

There are some problems with IPX broadcasts, i'll have to look into it....



	

	

 




