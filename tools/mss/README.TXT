*****************************************************************************
Miles Sound System 3.6                          RAD Game Tools 801-322-4300
Release 3.6B of 7-Mar-97
*****************************************************************************

                        ***************************
                        *** Development History ***
                        ***************************

V3.6B of 7-Mar-97
-----------------
Fixed bug in waveOut mixing code that could cause all sounds to stop under
  very high sound loads.

Fixed bug in red book volume control for CD devices that allowed stereo
  volume resolution.

Fixed some debug stuff that was left in the Quick example application.

Switched the newest Causeway (which works around a bug in the Ensoniq VIVO
  drivers).

Fixed a bug if you called AIL_start_sample or AIL_load_sample_buffer from
  an EOB or SOB callback (EOS and timer callbacks were fine).

MSS doesn't look for the application hwnd in AIL_digital_handle_reacquire()
  again - it just uses the old one.

V3.6A of 25-Jan-97
------------------
Added a new high-level streaming API - stream disk files in 4 calls!
  Streams work just like samples - you can change the playback rate,
  volume, pan, loop count, etc.

Updated the timer system to work around the dreaded Microsoft VC debugger
  problem under Windows 95 and Windows NT (slowdowns and crashes when
  sitting on a breakpoint for a long time).

Added volume control functions for redbook audio (AIL_redbook_volume and
  AIL_redbook_set_volume).  Uses MSCDEX for DOS, the mixer API for Win32,
  and the aux device calls for Win16.

Fixed a bug in the DOS redbook control functions that would sometimes
  cause a redbook track not to play (good catch - BullFrog!)

Integrated the Fatman's General MIDI FM patches directly into the library.

Added a new function to duplicate an existing quick handle (AIL_quick_copy).
  This function doesn't duplicate the audio memory - only the handle info.

Added two new functions to control the speed and volume of a quick handle
  (AIL_quick_set_speed and AIL_quick_set_volume).

Added a new function to return the digital and MIDI driver handles when
  you used the Quick API to init the system (AIL_quick_handles).  This
  is handy because you can now use AIL_quick_startup to simplify
  initializing MSS, and still use the full API for everything else.

Fixed a bug in AIL_strnicmp that was causing a crash when loading data
  out of a 32-bit resource DLL file.

The MSS DLLs now check to see if they have been incorrectly installed in
  Windows or Windows system directory.

Added the AIL_get_preference function to the DOS API.

Took out the virtual memory feature of VTOR - use the Smacker utilities
  to convert huge wave files to RAWs.

Fixed a bug in AIL_file_read when an pointer was passed and the read
  failed.

Fixed a bug in AIL_file_write that would free the pointer if the write
  failed.

Fixed a bug in the quick API where the MIDI system occasionally fail to
  load under NT.

Various minor clean-ups in the Delphi interface units.


V3.50F of 22-Oct-96
-------------------

Adjusted the method that the 16-bit DLL locks its memory for extreme 
  low memory machines (Win3.x and Win32s).

Fixed potential crash when completely out of DOS memory in the 16-bit
  DLL (Win3.x and Win32s).

Preceded the system functions with "AIL_" to fix function name collisions
  with VFX from Miles Design.  The old function names are still defined
  to match the new names, so no source should have to be changed.  The
  changed functions are as follows (old name first, then new name):

        MEM_alloc_lock   AIL_mem_alloc_lock
        MEM_free_lock    AIL_mem_free_lock
        FILE_error       AIL_file_error
        FILE_size        AIL_file_size
        FILE_read        AIL_file_read
        FILE_write       AIL_file_write
        FILE_append      AIL_file_append
        MEM_alloc        AIL_mem_alloc
        MEM_free         AIL_mem_free
        MEM_use_malloc   AIL_mem_use_malloc
        MEM_use_free     AIL_mem_use_free
        MEM_alloc_DOS    AIL_mem_alloc_DOS
        MEM_free_DOS     AIL_mem_free_DOS
        VMM_lock_range   AIL_vmm_lock_range
        VMM_unlock_range AIL_vmm_unlock_range
        VMM_lock         AIL_vmm_lock_range
        VMM_unlock       AIL_vmm_unlock_range

Because of these function name changes and because Smacker dynamically 
  links to MSS, you must update to at least Smacker version 2.2B for
  MSS 3.50F.


V3.50E of 18-Sep-96
-------------------

Fixed bug in release/reacquire code for DirectSound.

Added new ESS drivers - now run under Win95 DOS box.


V3.50D of 14-Sep-96
-------------------

Fixed unusual problem of DirectSound dropping samples when many samples are
  played at once.

Improved method that the background task uses for CPU-sharing, so that it
  is slightly faster.

Fixed bug in MIDI driver loading code - seen with Gravis driver & MidiForm.


V3.50C of 22-Aug-96 (mostly Windows)
------------------------------------

Added locking calls for volume and pan setting in the DirectSound code.

Added AIL_digital_handle_release and AIL_digital_handle_reacquire for
  focus control under Windows.

Added AIL_MIDI_handle_release and AIL_MIDI_handle_reacquire for
  focus control under Windows.

Added SMP_PLAYINGBUTRELEASED and SEQ_PLAYINGBUTRELEASED flags for HSAMPLE
  and HSEQUENCE status flags.

Added new macro, "AIL_DLL_version()", to return the current DLL version
  (doesn't actually call into the DLL, so it is always safe).

Fixed weirdness with the background task complaining about Bad Code Pointers.

Changed the way AIL_redbook_status() works to speed it up.

Added AIL_serve() calls in the redbook API to avoid stutters on slow CDs.

Changed standard types (S8, U8, etc) slightly (to defines)

Under DirectSound, you can now pass NULL for the lpFormat paramater - this
  tells MSS not to adjust the primary sound buffer.


V3.50B of 7-Jul-96
------------------

Minor clean up release for Windows.  Major update for DOS version.

DOS version now has all of the features of the Windows version (red book
support, quick-integration API, sub-block looping, etc).

Fixed error in linked list code of the quick API for Windows.

Fixed error in AIL_send_channel_voice_message() when the driver handle is
NULL.

Changed AIL_ms_count() to call QueryPerformanceCounter() under NT.

AIL_ms_count() under DOS inside a Windows DOS box can read the Windows
timer for millisecond accuracy.

Added "FILE_READ_WITH_SIZE" flag to FILE_READ function, so that it can
return the size of the data read as the first four bytes of the returned
pointer.

AIL.LIB can be used for register or stack based programs (there is no
need to recompile for stack based programs anymore).

MSS can now be used with Watcom 9.5 to 10.6.  RAD recommends version 10.6.

Added new AIL_set_driver_directory() function to load drivers, .INI
files, and patch files from alternate directory (MSS still always searchs
the current directory first).

You must now call AIL_startup() before any other MSS calls (including
FILE_read, MEM_alloc, etc).

Callbacks are now typed and must use the AILCALLBACK calling convention.

Added two new redbook functions.  AIL_redbook_track() returns the current
playing track number.  AIL_redbook_retract() closes the CD drive door.

Timer callbacks now synchronize the callback frequency to AIL_ms_count().
This improves MIDI playback drastically under DOS inside a Windows DOS box.
It also prevents long-term timer counting drift if you use MSS timer to
track time (use AIL_ms_count() directly for best accuracy).

DDK has been integrated into the SDK for convience.

All example programs and tools have been bound with the Causeway DOS
extender.

Added new DOS drivers for the AMD Interwave and the NVidia multimedia card.

The VBL flag no longer used for delaying (in either the library or the
drivers).

SetSound can display the current Windows 95 configuration when running
in a Windows 95 DOS box.

Built-in Borland C support for both Windows and DOS PowerPack.

SoundFont support now supplied directly with MSS.

AILEXPORT has been renamed to AILCALLBACK for consistency with DOS.

Renamed main libraries and header to use MSS instead of AIL.

VTOR uses virtual memory so that large .WAVs or .VOCs can be converted.
Under Windows and Windows 95 DOS boxes, virtual memory is limited to 16 MB,
so you may have to run VTOR under normal DOS to convert really huge files.

Added 16- and 32-bit Delphi interface units.

      
V3.50A of 11-Apr-96 (Windows only)
----------------------------------

Major new version with complete built-in DirectSound support.  There are
several new DirectSound specific functions - see the docs for details.

AIL_startup() can be called multiple times - but calling AIL_shutdown()
closes everything down immediately.  This allows you to call AIL_startup()
several times in your application, and call AIL_shutdown() once in your
closing procedure.

Fixed a harmless thread handle leak.

Also fixed is AIL_minimum_sample_buffer_size() which was
under-reporting by a factor of 4.

Added the new Quick Integration API.  See the docs for details.

Added new AIL_digital_configuration() call that returns info about the 
current sound driver.

Added new sub-block looping feaure - can loop internal pieces of a buffer.
Use the AIL_set_sample_loop_block() function to set the loop points.

Callbacks now work under Win32s - but are not called at interrupt time.
Rather, they are distributed from the Windows messaging system, so you
must yield to Windows to receive callbacks

Added new Red-Book audio interfaces - play CD audio tracks quickly and 
easily.

Multiple 16-bit and Win32s apps can be loaded at the same time.

Fixed crashes when running 16-bit DLL with other 32-bit apps or libraries.

Fixed crashes when memory would move.

Added new millisecond count call - AIL_ms_count().  Allows accurate timing
without having to create absurdly fast timers.

Added new starvation call - AIL_samaple_buffer_info() for video players.

AIL_serve() is no longer needed in your message loop.  Under Win32, a
background thread handles servicing.  Under Win16 and Win32s, a background
task takes care of it.

MSS now works under Windows NT - (the NT MIDI drivers are still pretty
buggy however).


V3.04A of 4-Feb-96 (Windows only)
---------------------------------

This minor release integrates Win32s support directly into WAIL32.DLL.
This means you can create one executeable that supports Win 95, Win NT,
and Win32s.  To support Win32s, you must distribute both WAIL32.DLL and
WAIL16.DLL.

                         ** Win 32s Issues **

You must use MEM_alloc_lock for memory that you intend to use for sample 
or sequence data (with AIL_set_sample_address, AIL_load_sample_buffer, 
AIL_init_sequence, etc).  MEM_alloc_lock returns 32-bit pointers
that have been locked and fixed for Win32s.  Under Win 95 or Win NT, 
MEM_alloc_lock just returns GlobalAlloc-ed memory, so you can use this 
function for all environments.  If you pass in a pointer that isn't 
created with MEM_alloc_lock, MSS will display a error message box.
      
HDIGDRIVER, HSAMPLE, HSEQUENCE, etc, are all handles under Win32s -
they are not pointers, so you can't dereference them.
      
Internal MSS calls are not captured in the debug trace file when running
under Win32s.

Callbacks aren't supported yet.

                            ** Bug Fixes **

Fixes segment wrap around in WAIL16 on sound cards that need large Windows
buffers (>64K).

Marks several variables as volatile, so that WAIL can be compiled with 
optimizations on.
 
Pointers are checked for validity as they are freed.


V3.03D of 22-Nov-95 (Windows only)
----------------------------------

This maintenance release uses a new thread synchronization technique
which should fix deadlocking problems previously encountered by some 
32-bit MSS applications.  

Previous versions of the 32-bit MSS library could crash if an attempt was
made to start more than one instance of a MSS application.  This has been
fixed.

AIL_branch_index() could fail to work properly in previous versions of MSS.
This problem has been fixed for this release.


V3.03C of 7-Nov-95
-----------------

MSS version 3.03C is a maintenance release which includes several minor 
compatibility improvements and bug fixes.

                            ** New Features **

SETSOUND now includes explicit support for the new Yamaha DB50-XG 
WaveForce(TM) daughterboard via the MPU401.MDI driver.

MSS for Win32 now uses thread synchronization.

                             ** Bug Fixes **

If all playing samples had their volume set to 127, it was possible for the
AIL_set_digital_master_volume() command to fail to control the overall
digital output level.  This has been fixed.

The SETSOUND installation program did not uninstall drivers after successful
detection.  This caused no reported problems; however, the message "Out of
timer handles" could appear if the user were to select and install any driver
more than 16 times.  SETSOUND now uninstalls drivers after detection to avoid
the possibility of this error message.

The AIL_MDI_driver_type() function caused premature termination of debug
scripts when the AIL_DEBUG feature was in use.  This has been fixed.

Memory past the end of XMIDI files could be accessed by AIL_init_sequence(),
in cases where XMIDI files smaller than 1024 bytes were played.  This caused
occasional page faults in the Windows MSS system, but no problems were ever
reported in the DOS MSS system.  This bug has been fixed, however, in both
versions of MSS.

At the end of an unlooped XMIDI sequence, the sequence loop count is now
reset to 1.  This allows a sequence to be restarted without undesirable
looping.

Under certain rare circumstances, unwanted pauses occurred during XMIDI
playback when using the FOR/NEXT XMIDI controllers or the AIL_branch_index()
function.  These pauses have been eliminated in this release.

The AIL_end_sequence() function previously called the end-of-sequence (EOS)
callback function, even if the sequence was already done.  This behavior has
been corrected.

The Microsoft Windows Sound System driver (SNDSYS.DIG) has been patched to
fix a bug that caused a detection problem on some systems, particularly 
Compaq machines.

The standard Sound Blaster digital driver (SBLASTER.DIG) could cause lockups
or "Transfer Stack Overflow" messages during detection when running under 
Windows 95.  This was due to the use of an extremely short DMA buffer to 
check for auto-init DMA capabilities.  In this release, the DMA verification 
buffer in SBLASTER.DIG has been increased in size to avoid problems.  No other
drivers, including those for the Sound Blaster Pro and Sound Blaster 16, were
affected by this problem.


V3.03B of 29-Jun-95
-------------------

Changing the digital master volume level had no effect on samples whose 
volume level was set to 127.  This has been fixed.

A VMM page-locking problem with the FlashTek DOS extender version of MSS
has been fixed.


V3.03A of 25-Jun-95
-------------------

Sporadic crashes at shutdown time may occur in earlier MSS releases if a 
sequence handle is allocated with AIL_allocate_sequence_handle() but never 
actually initialized with AIL_init_sequence().  This patch corrects this
problem, which affected only the MS-DOS versions of MSS.


V3.03 of 18-Jun-95
------------------

This is a major update release which includes many new features added in 
response to client requests.

                            ** New Features **

Two new drivers, SNDSYS.DIG and ESFM.MDI, provide support for the Microsoft
Windows Sound System and ESS Technology ES1688, ES1788, and ES1888 chipsets,
respectively.  The SETSOUND program recognizes these new drivers by means of
an updated AILDRVR.LST file.

Support for the Borland Power Pack and Zortech/Symantec compiler packages is
now available.  Please note that by default, the MSS system is built for the 
more popular Watcom/Rational package.  To use MSS with any other compiler/DOS
extender package, it is necessary to delete all .OBJ, .LIB, and .EXE files and
recompile the desired library version.  The .MDI and .DIG drivers, as well as
the files in the TOOLS subdirectory, do not need to be rebuilt.

Four new functions provide a "master volume" control for all digital samples 
and XMIDI sequences:

  AIL_set_digital_master_volume()
  AIL_set_XMIDI_master_volume()
  AIL_digital_master_volume()
  AIL_XMIDI_master_volume()

Please note that like the AIL_set_sequence_volume() function, 
AIL_set_XMIDI_master_volume() will not have the desired effect on sequences
which do not include MIDI Part Volume controllers (7) at the beginning of each
track.  Refer to the updated documentation for further information on these
functions.

The function AIL_register_beat_callback() has been added to allow applications
to request callback service at sequence beat transition points.  Refer to the
description of this function AND the AIL_sequence_position() function in 
the documentation before attempting to use either function!

The function AIL_MDI_driver_type() has been added to help applications
identify the synthesis technology provided by various MSS drivers.  This
function may be used to select different XMIDI sequences for various sound
synthesis platforms.  Refer to the documentation for details.

A new XMIDI controller has been implemented.  The XMIDI Channel Mute 
controller (107) can be used to selectively mute channels in sequences.  Refer
to the updated documentation for information on the XMIDI Channel Mute 
controller.

The prototypes of the AIL_install_DIG_INI() and AIL_install_MDI_INI() functions 
have been changed to provide enhanced error-reporting capabilities.  It is
now possible to distinguish between failure due to a missing .INI file and
failure due to driver initialization or hardware detection problems.  Refer
to the updated documentation file for details.

Previous versions of MSS truncated 16-bit sample data to 8-bit sample data 
when panpot or volume scaling was used (panpot != 64 or volume < 127).  With
this release, 16-bit sample data is scaled with a multiply/shift sequence that
preserves full 16-bit sample precision.

For consistency with the new Windows MSS package, the DIG_DRIVER * and
MDI_DRIVER * data types have been replaced with the HDIGDRIVER and HMDIDRIVER
handle types, respectively.  The old pointer types may still be used, but
new applications should be written with the new handle types for compatibility
with the Windows MSS API.

Previous versions of MSS shut down interrupts during the digital mixing 
process, which caused problems with high-speed serial communications.  By
default, MSS V3.03 no longer disables interrupts at any time during digital
audio processing.  A new preference name, DIG_SS_LOCK, has been added to 
allow applications to restore the former behavior of shutting down 
interrupts while updating the DMA buffer.  Few applications should need 
to change the DIG_SS_LOCK preference value.

When playing a stereo digital audio sample on a monaural sound card, previous
versions of MSS truncated the right channel source data and played only the
left channel data.  MSS V3.03 allows the AIL_set_sample_pan() function to
fade linearly between the left and right source channel in this situation.

                             ** Bug Fixes **

In previous versions of MSS, it was possible under exceptionally rare 
conditions for a nested INT 8 interrupt to cause crashes due to stack 
corruption.  Under Windows 95, however, the probability of protection faults
and crashes is greatly increased.  This bug has been fixed for this release 
of MSS; additionally, Windows 95 itself has been patched to correct the bug
in legacy applications by automatically modifying the faulty instructions 
in the MSS INT 8 ISR.  Customers with the retail release of Windows 95 should
not be affected; however, early beta versions (pre-490) can exhibit problems.

A minor bug in the digital mixer code caused clipping tests to be performed 
even with only one actively-playing sample.  This bug has been fixed for
this release, resulting in faster digital audio performance.

The AIL_resume_sample() function could fail to operate properly if no other
samples were playing at the time it was called.  This has been fixed.

Reports have been received of lockups with Sound Blaster 16 daughterboards
(such as those from Roland and Creative Labs) which occasionally occur 
during extremely intense bursts of MIDI traffic while digital audio is in
use.  The MSS MPU401 driver has been patched to provide a partial workaround 
for this problem; however, momentary digital audio glitches may still be noted
during fast volume fades and other periods of high MIDI activity.  
Because this problem is due to a hardware bug in the Creative chipset, no 
completely reliable software workaround is currently known.  However, 
difficulties arising from the bug are quite rare and should not be a cause 
for concern with most applications.

AIL_start_sequence() formerly reset the sequence loop count (if any) to 1,
effectively disabling any loop count previously established by 
AIL_set_sequence_loop_count().  This has been fixed in version 3.03 --
AIL_set_sequence_loop_count() may now be called either before or after 
playback begins.


V3.02 of 18-Jan-95
------------------

The DRIVERS directory now contains a complete MSS driver configuration
program (SETSOUND.EXE) which may be included in your application.  SETSOUND 
must be executed before any of the MSS example programs in the DRIVERS 
directory can be run.  Consequently, the example programs no longer require
driver filenames to be specified directly on their command lines.

SETSOUND is very easy to use and can shave days or weeks of programming time
from an application's development cycle.  However, you should refer to 
SETSOUND's complete description in the Tools Reference section of the manual 
to help you get the most out of this powerful new utility.

The contents of the old MIDI and DIGITAL directories have been merged into
the outer DRIVERS directory.  Minor revisions have been made to the example
programs and makefiles to accomodate this change.

SBLASTER.MDI, SBPRO1.MDI, and SBPRO2.MDI now check the BLASTER environment
variable to permit FM sound autodetection at addresses other than 220h.
Previously, only the Sound Blaster-series .DIG drivers checked the BLASTER
variable.

DMA channel 0 is now available for use with Sound Blaster Pro adapters.  
In earlier releases, DMA channel 0 worked only with Sound Blaster 16 boards.

During autodetection, RAP10.DIG now checks addresses 330H and 320H, using DMA
channels 5 and 6 and IRQ 9.  Formerly, IRQ 5 was tested with address 320H.

The AIL_minimum_sample_buffer_size() function could occasionally return 
buffer sizes below the actual minimum size when used with the SBPRO.DIG
driver.  This bug has been fixed.

An error-checking problem in the SSTEST example program could cause GP faults
under rare circumstances.  This bug has been fixed.

Updated Gravis UltraSound drivers are now available with this release.  These
"native mode" drivers no longer require the ULTRAMID emulation program.

Two new drivers, ADRV688.DIG and PROAUDIO.DIG, are supplied for use with
ESS Technology's ES688 chipset and Media Vision's Pro Audio Spectrum line,
respectively.  Many newer systems from Compaq and other vendors include the
ES688 device, which can support 16-bit stereo sound at up to 44 kHz through 
the use of the new ADRV688.DIG driver.  PROAUDIO.DIG has been tested and
found to give satisfactory performance on most systems; however, the Media
Vision cards' "native mode" has never been among the most reliable of sound
options, so users should be informed that the SBLASTER.DIG driver can be 
used with most Pro Audio Spectrum boards in the event problems occur.

First version of MSS for Windows is released as version 3.02 with full
MSS API support for 16-bit Windows 3.1, Windows 95, and Windows NT.

V3.01 of 22-Nov-94
-------------------

This maintenance release includes a number of new features and bug fixes.

                           ** New Features **

The AIL_background() function allows an application function to tell if it
is being executed from within an MSS timer, XMIDI, or callback function.

The new AIL_set_sample_file() function is similar to the existing
AIL_allocate_file_sample() function, except that it operates on an existing
sample handle.  .VOC or .WAV files may be played using an application's
own sample handles through the use of the AIL_set_sample_file() function.
See the API Reference for further information.

A powerful debugging feature has been added to the MSS API library.  If an
AIL_DEBUG environment variable is created with a command of the form 
SET AIL_DEBUG=filename, a timestamped log of virtually every MSS API call
made by any subsequently-executed MSS application will be recorded to the 
text file or device "filename".  The logfile may be examined to help 
diagnose and fix application errors.  MSS functions called from within 
callback functions will not be logged, along with the functions 
AIL_switch_stack(), AIL_restore_stack(), AIL_unlock(), AIL_lock(), and 
AIL_background().

Digital support for the Roland RAP-10 device is now available (RAP10.DIG).  
This board produces extremely high-quality audio in either 8- or 16-bit, 
mono or stereo formats.  Use the MPU401.MDI driver for MIDI support with 
the RAP-10.  

Digital support for the New Media WaveJammer PCMCIA audio adapter is now
available (JAMMER.ZIP).  Use the OPL2.MDI driver for MIDI support with
the WaveJammer.

                             ** Bug Fixes **
 
Under certain circumstances, double-buffered samples could be terminated
prematurely in previous releases of MSS.  This problem has been fixed.

"Stuck" notes were occasionally reported when shutting down applications
using the Roland RAP-10 and Roland SB16 MIDI daughterboards.  This MIDI
hardware overrun problem has been corrected by introducing a slight delay 
into functions which flush the XMIDI note queue, such as AIL_end_sequence() 
and AIL_stop_sequence().  This delay should not normally be perceptible 
at the application level.

Revised guidelines for audio-video synchronization have been issued with
this release.  The AIL_minimum_sample_buffer_size() function returns the 
size of the smallest memory block which may be used with the 
double-buffering calls.  Typically this value will range from 2 to 8KB;
applications must ensure that their video playback engines can provide 
sound blocks of this size or greater.  See the API Reference for further
information.  The DBTEST.C example program now uses 
AIL_minimum_sample_buffer_size() to allocate its memory buffers.  Note that
the "Guidelines for Audio/Video Synchronization" section that appeared in 
earlier manual revisions is no longer applicable.

Under rare circumstances, MSS's built-in VMM locking calls could fail to
lock the last few bytes in a module.  This problem was caused by Watcom's
code optimizer; a workaround is now in place.

The AIL_restore_interrupts() function is now prototyped to return void.

Calls to the AIL_channel_lock() function could result in crashes if any
sequence handles were allocated with AIL_allocate_sequence_handle() but 
not yet initialized with AIL_init_sequence().  This problem has been fixed.

AIL_destroy_wave_synthesizer() now releases sample handles allocated by
AIL_create_wave_synthesizer().  Previously, these allocated handles were lost
upon destruction of the HWAVE instance.


V3.00A of 24-Oct-94
-------------------

This maintenance release includes the following new features:

  - FlashTek X32 support with the Watcom compiler
  - Sound Blaster AWE32 XMIDI support (SoundFont support coming soon!)
  - Ensoniq SoundScape native-mode digital and XMIDI support
  - Updated Gravis UltraSound support

The AIL_set_sequence_volume() function can now be used prior to calling
AIL_start_sequence().  Previously, AIL_start_sequence() reset the sequence
volume to its default value.  The default sequence volume is now established
by the AIL_init_sequence() function, based on the MDI_DEFAULT_VOLUME
preference.


V3.00 of 19-Sep-94
------------------

* The complete digital audio system, with digital mixing,
  mono/stereo support, double-buffering, and individual control over sample
  pitch, volume, panning, and stereo balance.  Any sample, regardless of 
  its resolution, rate, channel count, or sign, may be played with any 
  MSS 3.0 digital driver -- present or future!

* Full General MIDI support, including 16-channel response for all Yamaha
  drivers, adjustable pitch-bend range, and information on The Fat Man's
  acclaimed Yamaha General MIDI instrument set.
 
* Over 100 API functions for interactive music and sound effects control.

* Complete documentation of all tools and MSS API functions.

* Support for Watcom C32/C++32 V10.0.  Versions 9.5 and earlier may also be
  used by recompiling the library's source code.


                           *********************
                           *** List of files ***
                           *********************


Main Directory:

MSS.H        ........... MSS for DOS header file 
MSS.LIB      ........... MSS for DOS library file for Watcom register or stack
MSSBOR.LIB   ........... MSS for DOS library file for Borland PowerPack
README.TXT   ........... This file
MSSW.H       ........... MSS for Windows header file
MSS16.DLL    ........... MSS for Windows 3.x and Win32s DLL
MSS32.DLL    ........... MSS for Windows 95, Windows NT and Win32s DLL
MSS16.LIB    ........... Import library for Wail16.DLL
MSS32.LIB    ........... Import library for Wail32.DLL
MSS32BC.DEF  ........... DEF file for Wail32.DLL (Use with Borland C)
MSSB16.TSK   ........... MSS for Windows 3.x and Win32s background task app


Directory DOSEXAM:       MSS for DOS example programs

DEMO.BAT     ........... Runs all of the examples once
AUDIOCD.EXE  ........... Simple redbook audio CD player
XDEMO.BAT    ........... Runs WAVELIB, XDIGPLAY for XMIDI-triggered PCM demo
XMIPLAY.EXE  ........... Basic XMIDI file playback example
XDIGPLAY.EXE ........... Virtual wave synthesis example (run XDEMO.BAT)
DBTEST.EXE   ........... Double-buffering demonstration program
DIGPLAY.EXE  ........... .VOC / .WAV file playback demonstration program
PLAY.EXE     ........... Quick API example - plays .VOC / .WAV / .XMI
SSTEST.EXE   ........... Multichannel digital audio demonstration program
WAVEDEMO.TXT ........... Example wave library list file used by XDEMO.BAT


Directory WINEXAM:       MSS for Windows example programs

DEMO16.EXE   ........... MSS Demo program for 16-bit Windows
DEMO32.EXE   ........... MSS Demo program for Win32
QUICK16.EXE  ........... Quick API example program for 16-bit Windows
QUICK32.EXE  ........... Quick API example program for Win32s
WXMIPLAY.EXE ........... Command-line XMI player (for .XMI file association)


Directory DOSDRV:        Redistributable sound drivers for DOS

ADRV688.DIG  ........... ESS ES688 AudioDrive digital driver
IWAV.DIG     ........... AMD Interwave digital driver
JAMMER.DIG   ........... New Media WaveJammer digital driver
NVDIG.DIG    ........... NVidia digital driver
PROAUDIO.DIG ........... Pro Audio Spectrum digital driver
RAP10.DIG    ........... Roland RAP-10 digital driver
SB16.DIG     ........... Sound Blaster 16 / AWE32 digital driver
SBLASTER.DIG ........... Sound Blaster Basic digital driver
SBPRO.DIG    ........... Sound Blaster Pro digital driver
SNDSCAPE.DIG ........... Ensoniq SoundScape digital driver
SNDSYS.DIG   ........... Microsoft Windows Sound System digital driver
ULTRA.DIG    ........... Gravis UltraSound digital driver
ADLIB.MDI    ........... Standard Ad Lib (generic OPL-2) XMIDI driver
ADLIBG.MDI   ........... Ad Lib Gold XMIDI driver
ESFM.MDI     ........... ESS ES1688, ES1788, ES1888 Enhanced FM XMIDI driver
IWAV.MDI     ........... AMD Interwave MIDI driver
MPU401.MDI   ........... MPU-401 General MIDI driver
MT32MPU.MDI  ........... MPU-401 MT-32 driver
NULL.MDI     ........... Null MIDI driver (e.g., for animation sequencing)
NVMPU.MDI    ........... NVidia MIDI driver
OPL3.MDI     ........... Generic OPL-3 XMIDI driver
PAS.MDI      ........... Pro Audio Spectrum (old) XMIDI driver
PASPLUS.MDI  ........... Pro Audio Spectrum (plus / 16) XMIDI driver
PCSPKR.MDI   ........... IBM PC speaker XMIDI driver (channel 2 only)
SBAWE32.MDI  ........... Sound Blaster AWE32 General MIDI driver
SBLASTER.MDI ........... Generic Sound Blaster XMIDI driver
SBPRO1.MDI   ........... Sound Blaster Pro I (dual-OPL2) XMIDI driver
SBPRO2.MDI   ........... Sound Blaster Pro II/SB16/AWE32 (OPL-3) XMIDI driver
SNDSCAPE.MDI ........... Ensoniq SoundScape XMIDI driver
TANDY.MDI    ........... Tandy 3-voice XMIDI driver (channels 2-4 only)
ULTRA.MDI    ........... Gravis UltraSound XMIDI driver
MSSDRVR.LST  ........... Driver description file used by SETSOUND.EXE
SETSOUND.EXE ........... MSS Sound Configuration Utility
MSSW95.EXE   ........... MSS Windows 95 configuration display
SAMPLE.AD    ........... Example OPL2 GTL file (MT-32 emulation)
SAMPLE.OPL   ........... Example OPL3 GTL file (MT-32 emulation)


Directory TOOLS:         Tools used for DOS and Windows MSS

CLAD.EXE     ........... Ad Lib / Creative Labs instrument file translator
GLIB.EXE     ........... Global Timbre Librarian
MIDIECHO.EXE ........... MPU-401-compatible MIDI repeater for MSS drivers
MIDIFORM.EXE ........... MIDI-to-XMIDI sequence compiler
MIDILOG.EXE  ........... MIDI file/event filter
MIDIREC.EXE  ........... MIDI Format 0 file recorder
MLIST.EXE    ........... MIDI file disassembler / Roland Musician's Reference
VTOR.EXE     ........... .VOC/.WAV-to-raw PCM data file converter
WAVELIB.EXE  ........... Wave file librarian for virtual wave synthesis


Directory DOSSRC:        MSS for DOS source code

DOMAKE.BAT   ........... Batch file to make Watcom.Mak
WATCOM.MAK   ........... Makefile for Watcom C / MASM 6.1 
DOMAKEB.BAT  ........... Batch file to make Borland.Mak
BORLAND.MAK  ........... Makefile for Borland Power Pack / MASM 6.1
AILA.ASM     ........... Process Services source
AILSSA.ASM   ........... Digital mixer source
AIL.C        ........... Main MSS library source
AILCD.C      ........... Redbook CD audio source
AILDEBUG.C   ........... Debugging API shell
AILDEBUG.INC ........... Debugging API shell include file
AILQAPI.C    ........... Source for quick integration API
AILSFILE.C   ........... Digital file playback source
AILSR.C      ........... Source to hide register vs. stack issues
AILSS.C      ........... Digital API source
AILXDIG.C    ........... Virtual wave synthesizer source
AILXMIDI.C   ........... XMIDI interpreter and API source
MEMFILE.C    ........... General-purpose memory and file routines
AIL.INC      ........... Include file for MSS MASM sources


Directory WINSRC:        MSS for Windows source code

DOMAKE.BAT   ........... Batch file to make WAIL16.MAK and WAIL32.MAK
WAIL16.MAK   ........... Makefile for WAIL16.DLL
WAIL32.MAK   ........... Makefile for WAIL32.DLL
DODEFS.EXE   ........... Parser to create import file from an export dump
SI.BAT       ........... Make SoftIce debug files and copy to WinExam
WAILSS16.ASM ........... Digital mixer source for 16-bit code
WAILSS32.ASM ........... Digital mixer source for 32-bit code
WAIL.C       ........... Main MSS library source
WAILCD.C     ........... Redbook CD audio source
WAILDBG.C    ........... Debugging API shell
WAILDBG.INC  ........... Debugging API shell include file
WAILQAPI.C   ........... Source for quick integration API
WAILSFIL.C   ........... Digital file playback source
WAILDS.C     ........... Digital API source for DirectSound
WAILSS.C     ........... Digital API source for waveOut drivers
WAILXDIG.C   ........... Virtual wave synthesizer source
WAILXMIDI.C  ........... XMIDI interpreter and API source
WAILSYS.C    ........... General-purpose routines source
SSMACROS.INC ........... Include file for MSS MASM sources
WAIL16.DEF   ........... WAIL16.DLL .DEF file
WAIL16.RSP   ........... WAIL16.DLL linker response file
WAIL32.RSP   ........... WAIL32.DLL linker response file
WAILB16.C    ........... Background task application source code
WAILB16.DEF  ........... Background task application .DEF file


Directory TOOLS\SRC:     Source code for the MSS tools

DOMAKE.BAT   ........... Batch file to make Watcom.Mak
WATCOM.MAK   ........... Makefile for Watcom C / MASM 6.1-based utilities
TIMER.ASM    ........... MIDIREC assembly routines
REAL_ISR.ASM ........... MIDIECHO assembly routines
REAL_ISR.BIN ........... MIDIECHO assembly routines
CLAD.C       ........... Ad Lib / Creative Labs instrument translator source
GLIB.C       ........... Global Timbre Librarian source
MIDIECHO.C   ........... MPU-401-compatible MIDI repeater source
MIDIFORM.C   ........... MIDI-to-XMIDI sequence compiler source 
MIDILOG.C    ........... MIDI file/event filter source
MIDIREC.C    ........... MIDI Format 0 file recorder source
MLIST.C      ........... MIDI file disassembler source
MSSW95.C     ........... Source for Windows 95 configuration viewer
SETSOUND.C   ........... Source for SetSound utility
VTOR.C       ........... .VOC/.WAV-to-raw PCM data file converter source
WAVELIB.C    ........... Wave file librarian for virtual wave synthesis
GEN.H        ........... General-purpose real-mode resource header
TIMER.H      ........... MIDIREC support header file
GEN.ASM      ........... General-purpose real-mode assembly functions
GEN.C        ........... General-purpose real-mode C functions


Directory DOSEXAM\SRC:   MSS for DOS example program source code

DOMAKE.BAT   ........... Batch file to make Watcom.Mak
WATCOM.MAK   ........... Makefile for example programs
AUDIOCD.C    ........... Simple redbook audio CD player source
DBTEST.C     ........... Double-buffering demonstration program
DIGPLAY.C    ........... .VOC / .WAV file playback demonstration program
PLAY.C       ........... Quick API example source - plays .VOC / .WAV / .XMI
SSTEST.C     ........... Multichannel digital audio demonstration program
XDIGPLAY.C   ........... Virtual wave synthesis example (run XDEMO.BAT)
XMIPLAY.C    ........... Basic XMIDI file playback example


Directory WINEXAM\DEMOSRC: MSS for Windows Demo program source

DOMAKE.BAT   ........... Batch file to make Demo16.MAK and Demo32.MAK
DEMO16.MAK   ........... Make file for Demo16.EXE
DEMO32.MAK   ........... Make file for Demo32.EXE
DEMO.C       ........... Source for the demo programs
DEMO.ICO     ........... MSS icon file
DEMO.RC      ........... Resource file for the demo programs
DEMO16.DEF   ........... .DEF file for Demo16.EXE
DEMO32.RSP   ........... Demo32.EXE linker response file


Directory WINEXAM\QUICKSRC: MSS for Windows Quick API program source

DOMAKE.BAT   ........... Batch file to make Quick16.MAK and Quick32.MAK
QUICK16.MAK  ........... Make file for Quick16.EXE
QUICK32.MAK  ........... Make file for Quick32.EXE
QUICK.C      ........... Source for the quick API example programs
DEMO.ICO     ........... MSS icon file
QUICK.RC     ........... Resource file for the quick API example programs
QUICK16.DEF  ........... .DEF file for Quick16.EXE
QUICK32.RSP  ........... Qucik32.EXE linker response file


Directory WINEXAM\WXMISRC: MSS for Windows WXMIPlay program source

DOMAKE.BAT   ........... Batch file to make WXMIPlay.MAK
WXMIPLAY.MAK ........... Make file for WXMIPlay.EXE
WXMIPLAY.C   ........... Source for the Windows XMI player
DEMO.ICO     ........... MSS icon file
WXMIPLAY.RC  ........... Resource file for the Windows XMI player
WXMIPLAY.DEF ........... .DEF file for WXMIPlay.EXE


Directory DELPHI:        MSS for Windows WXMIPlay program source

MSS16.PAS    ........... 16-bit Delphi interface unit source code
MSS32.PAS    ........... 32-bit Delphi interface unit source code
MSS16.DCU    ........... Compiled 16-bit Delphi interface unit 
MSS32.DCU    ........... Compiled 32-bit Delphi interface unit


Directory DOSDDK\DIG:    MSS for DOS digital drivers source code

DOMAKE.BAT   ........... Batch file to make Watcom.Mak
WATCOM.MAK   ........... Watcom make file
DDK.ASM      ........... General assembly support functions
DDK.H        ........... Header file for DDK.ASM
VDI_DIG.INC  ........... Standard driver include file from VDI DDK
DIGPAS.ASM   ........... Pro Audio Spectrum digital driver source
DIGRAP10.ASM ........... Roland RAP-10 digital driver source
DIGSB.ASM    ........... SoundBlaster digital driver source
DIGSB16.ASM  ........... SoundBlaster 16 digital driver source
DIGSBPRO.ASM ........... SoundBlaster Pro digital driver source
DIGSSYS.ASM  ........... Microsoft Sound System digital driver source
MDITEST.EXE  ........... Program to test newly created digital drivers
MDITEST.C    ........... Source to digital driver test program


Directory DOSDDK\MDI:    MSS for DOS MIDI drivers source code

DOMAKE.BAT   ........... Batch file to make Watcom.Mak
WATCOM.MAK   ........... Watcom make file
DDKXMIDI.BIN ........... XMIDI playback routines for MDITEST
MDI_DDK.H    ........... Header file for DDKXMIDI.BIN
VDI_MDI.INC  ........... General support routines for driver development
MDIAIL2X.ASM ........... Generic compatibility shell for MSS 2.X XMIDI drivers
MDIMPU.ASM   ........... MPU-401 General MIDI driver source
MDINULL.ASM  ........... Null MIDI driver (no synthesizer required) source
MPU401.INC   ........... Roland MPU-401 communications source for MT32MPU.MDI
MT32.INC     ........... MT32MPU.MDI driver source 
SPKR.INC     ........... IBM PC speaker XMIDI driver source
YAMAHA.INC   ........... Yamaha OPL-2 and OPL-3 driver source
SAMPLE.BNK   ........... Example Ad Lib Instrument Maker bankfile
SAMPLE.CAT   ........... Instrument catalog file used to build SAMPLE.AD
MDITEST.EXE  ........... Program to test newly created MIDI drivers
MDITEST.C    ........... Source to MIDI driver test program


Directory DOSDDK\SNDFONT: SB AWE32 Sound Font example code and .MDI driver

DOMAKE.BAT   ........... Batch file to make Watcom.Mak
WATCOM.MAK   ........... Watcom make file
SBAWE-SF.MDI ........... MSS .MDI driver for SoundFont programs
SNDFONT.EXE  ........... SoundFont example program
SNDFONT.C    ........... SoundFont example source code
DPMI.C       ........... DPMI module for the example program
DPMI.H       ........... Header file for DPMI.C
AWE32VSE.C   ........... High-level functions for using SoundFonts
AWE32VSE.H   ........... Header file for AWE32VSE.H
SAGA.XMI     ........... .XMI file used by the example program
SAGA.SBK     ........... SoundFound file used by the example program


Directory DOSDDK\OEM:    MSS for DOS OEM-supplied driver source and notes
 
GRAVIS   ZIP ........... Gravis UltraSound XMIDI and digital drivers
NVIDIA   ZIP ........... NVidia sound card driver source
SBAWE32  ZIP ........... Creative AWE32 XMIDI (asm with or without soundfont)
ENSONIQ  ZIP ........... Ensoniq SoundScape XMIDI and digital drivers


Directory MEDIA:         Example MSS .WAV, .RAW, and .XMI files

*.RAW        ........... 44 Khz, 8-bit, Mono .RAW digital sound files
*.WAV        ........... Sample digital sound files
*.XMI        ........... Sample MIDI files


                        *****************************
                        *** Important Information ***
                        *****************************

Please note that all Watcom C/C++ MSS applications that include any type of
callback function (timer, XMIDI, or digital) MUST be compiled with the -s 
(disable stack overflow testing) option.  Otherwise, crashes and/or lockups
may occur since MSS switches to its own internal stack at interrupt time.
Omission of the -s option is a frequent cause of timer-related problems.


Microsoft's documentation indicates that the multimedia functions
in the Win32 API should not be called from more than one application thread.
Because the 32-bit MSS library uses the Microsoft multimedia calls
internally, it is important that MSS applications adhere to this guideline
as well.  If you have a multithreaded application (including one with no
explicit threads other than callback functions), you should ensure that you do
not make calls such as AIL_start_sample(), AIL_load_sample_buffer(),
AIL_wave/midiOutOpen/Close(), or AIL_send_channel_voice_message()
from more than one thread in your program.  It is best to restrict all MSS
calls to a single thread if possible.  When in doubt about which MSS 
functions may be called from multiple threads, contact RAD Game Tools for
assistance.

The MSS .ASM files cannot be successfully assembled with MASM V6.0,
due to bugs in early versions of the assembler.  MASM V6.1A or later is
needed in order to build the entire MSS library.


MSS for Windows and MSS for DOS are separate products, and require
separate licensing arrangements.  Please contact RAD Game Tools at
801-322-4300 (or mitchs@radgametools.com) with any licensing inquiries,
comments, technical-support questions, or feature requests!

*****************************************************************************
Copyright (C) 1991-1997  RAD Game Tools, Inc.
*****************************************************************************


RAD Game Tools
850 South Main Street
Salt Lake City, UT
84101

801-322-4300
FAX: 801-359-6169

Internet:  jeffr@radgametools.com   or   mitchs@radgametools.com
Compuserve:  73237,75
*****************************************************************************
