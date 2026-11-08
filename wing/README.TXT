Microsoft WinG version 1.0
--------------------------
This file describes known bugs, gotchas, and helpful hints for the WinG
Version 1.0 final release.

ISVs may want to distribute portions of this readme file that describe
configuration bugs along with shipping products that use WinG.

WinG version 1.0 provides fast DIB-to-screen blts under Windows 3.1,
Windows for Workgroups 3.11, Windows 95, and Windows NT version 3.5.
WinG will not run on Windows NT version 3.1 or on earlier versions of
Windows.

WinG requires a 386 or better processor to run. WinG will not run on a
286.

If you have problems with WinG, please run the wingbug.exe file in
the bin directory of the SDK and send the generated report to
wingbug@microsoft.com.  Updated information on WinG is available
on CompuServe in the WINMM forum and on ftp.microsoft.com.


Known Bugs And Limitations
--------------------------
The following are known problems with or useful tidbits of information
about WinG version 1.0.

- On Windows 3.1, WinGBitmaps must be 8 bits per pixel and must be
  created with full 256 entry color tables.

- WinGDCs are NOT palette devices. You must change their color tables
  using WinGSetDIBColorTable, not SelectPalette.

- WinGBitBlt and WinGStretchBlt only support bltting from WinGDCs to the
  screen.

- Using BitBlt and StretchBlt to blt from one WinGDC to
  another can be very slow when a clipping region has been
  selected into the destination.

- WinGBitBlt and WinGStretchBlt may return different values than
  StretchDIBits for identical blts.

- A few GDI APIs do not work correctly with WinGDCs:

    StretchDIBits will not blt 24bpp and 16bpp DIBs into an 8bpp WinGDC.
    FloodFill with a NULL brush draws incorrectly
    FloodFill outside of the bounds of a WinGBitmap can flood the
      entire image
    Brushes created with CreatePatternBrush with a WinGBitmap fault when
      selected into a WinGDC on Win3x - use CreateBitmap(8,8,1,8,0)
    DrawIcon will crash
    WinGBitBlt and WinGStretchBlt will not always blt to the correct
      location when you have changed the Viewport and/or Window origins
      using SetViewportOrg or SetWindowOrg.

- You cannot change the origin of halftone brushes created by WinG.

- Noticeable timing differences have been found while running the
  WinG profiler on a computer connected to a network. For accurate
  results, disconnect your computer from the network the first time you
  run a WinG application. After the profile is complete, you can plug the
  net in again. NOTE: Microsoft is not liable for any damage you may
  incur by incorrect handling of your computer hardware.

- WinGBitBlt and WinGStretchBlt use a slightly different color matching
  algorithm than StretchDIBits when blting an 8-bit image to a 4-bit
  planar display such as a standard VGA. Mixing WinG and
  StretchDIBits on these displays may produce odd results.

- WinG relies on the mmsystem timer drivers to determine display
  performance. If mmsystem.dll and timer.drv are not installed
  correctly, the results of the performance test may be incorrect.
  mmsystem.dll should appear on the drivers= line of the [boot]
  section of system.ini, and timer=timer.drv should appear in the
  [drivers] section of system.ini.

- WinG version 1.0 does not yet use standard DCI because of time
  constraints.

- "Just In Time" debuggers install a fault handler in a chain along
  with the WinG display performance profiler. If your debugger
  reports a fault during the WinG display performance test, pass the
  fault on to Windows instead of invoking your debugger.


Driver-Specific Problems
------------------------
WinG depends on Windows display drivers written by independant
hardware manufacturers for much of its speed. Bugs or performance
problems in third-party display drivers may cause problems in WinG.
In many cases, the video card manufacturer has already fixed the bug,
and upgrading your display driver will often clear away problems.

There are some specific "bugs" in display drivers of which you should
be aware. The list below is not intended to slight the manufacturer
of any particular card or driver.

A list of stress-tested configurations is available on the CompuServe
WINMM forum and ftp.microsoft.com.

Some names in this list are trademarks of the respective
manufacturer.

- Early drivers for Diamond Viper cards included a "Power Palette"
  option that is no longer supported by Diamond. They recommend that
  you upgrade your drivers if you have this option. WinG may be
  slower when power palette is enabled.

- IBM no longer supports the IBM ThinkPad 720c. There are some
  problems using WinG with the ThinkPad 720c display drivers.

- Cirrus drivers before version 1.43 have many known bugs which
  have been fixed in the more recent drivers. Be sure to upgrade your
  drivers if you are still running with this version.

- Some ATI drivers offer a "Crystal Fonts" option. Turning Crystal
  Fonts on in 8-bit modes sets up a non-palettized driver that can
  slow WinG significantly.

- The ATI mach8 Radical drivers cause a number of problems in both
  WinG and in Windows with some versions of the ATI chipset. Be
  aware.

- The ATI VGA Wonder drivers (W31-*.drv) will crash during a call to
  StretchDIBits in the profiler.  Users can run the SVGA256.DRV driver
  that shipped with Windows.

- Many miro Crystal drivers have problems with StretchDIBits, so they
  crash during profiling.

- Early ATI Mach 32 PCI cards have a hardware timing problem and will
  hang while blting.  ATI will replace these cards for no cost.

- WinG is incompatible with the #9GXE "TurboCopy" mode.  Use the
  #9 control panel to disable TurboCopy (it is off by default).
  
- WinG uses a GetPixel to synchronize with display hardware when
  writing directly to the screen.  The ATI Mach 32 driver's GetPixel
  does not work properly, so it is possible to use GDI to draw to
  the screen, then use WinG to blt to the screen and have them overwrite
  each other.  Be careful mixing GDI drawing commands and WinG blts
  to the display.

- The Orchid mmtllo.drv driver for the Prodesigner IIs has duplicate
  system colors which prevents applications from getting an identity
  palette and greatly reduces the WinG blt speed.  A workaround is
  to set SYSPAL_NOSTATIC mode or use standard the Tseng ET4000 drivers
  instead of the mmtllo drivers.


A Note on Speed
---------------
WinG is designed to be the absolute fastest way to blt DIBs under
Windows.  The goal, achieved in many cases, is to blt at memory
bandwidth to the display device.

On most 8bpp devices, if you use the recommended DIB format (returned
by WinGRecommendDIBFormat) and set up correct identity palette, you
should get 1:1 blt speeds comparable to BitBlt, which blts device
dependent bitmaps (DDBs) to the display.  The timewing sample
application will show you various blt speeds on your display.

The WinGRecommendDIBFormat API will tell you whether to use top-down
or bottom-up DIBs for fastest unclipped 1:1 identity palette blts. If
you plan on using other types of blt (such as stretching or complex
clipping), your application may want to time top-down versus
bottom-up blts itself at run time. See the WinG help file for more
information.
