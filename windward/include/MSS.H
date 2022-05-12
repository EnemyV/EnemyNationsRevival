//############################################################################
//##                                                                        ##
//##  Miles Sound System                                                    ##
//##                                                                        ##
//##  AIL.H: API function prototypes                                        ##
//##                                                                        ##
//##  Flat-model source compatible with IBM 32-bit ANSI C/C++               ##
//##                                                                        ##
//##  Version 3.00 of 11-Apr-94: Initial version for MSS 3.0                ##
//##          3.01 of 21-Oct-94: Added Watcom/FlashTek support              ##
//##                             AIL_restore_interrupts() changed to void   ##
//##          3.02 of  3-Jan-95: VDI_HDR updated to DDK rev 1.12            ##
//##                             Added AIL_INI structure                    ##
//##                             Added AIL_install_DIG/MDI_INI() functions  ##
//##          3.03 of  9-Jan-95: Added various functions for V3.03 release  ##
//##                             Borland/Zortech support added              ##
//##          3.50 of 20-Feb-96: Added various functions for V3.50 release  ##
//##                                                                        ##
//##  Author: John Miles                                                    ##
//##                                                                        ##
//############################################################################
//##                                                                        ##
//##  Contact RAD Game Tools at 801-322-4300 for technical support.         ##
//##                                                                        ##
//############################################################################

#ifndef AIL_VERSION

#define AIL_VERSION      "3.6B"
#define AIL_VERSION_DATE "8-Mar-97"

#define MSS_VERSION AIL_VERSION
#define MSS_COPYRIGHT "Copyright (C) 1991-97 RAD Game Tools, Inc."

#endif

#ifndef MSS_VERSION_ONLY

//
// Watcom/FlashTek support (for library build)
//

#ifdef INT21
  #ifdef __WATCOMC__
    #include "x32.h"
  #endif
  #ifdef __ZTC__
    #include "x32.h"
  #endif
#endif

#ifndef AIL_H
#define AIL_H

#ifdef __WATCOMC__
  #ifdef _PUSHPOP_SUPPORTED
    #pragma pack(push,1)
  #else
    #pragma pack(1)
  #endif
#endif


//
// MetaWare support
//

#ifdef __HIGHC__

#define cdecl _CC(_REVERSE_PARMS | _NEAR_CALL)
#pragma Global_aliasing_convention("_%r");
#pragma Align_members(1)

#ifndef FAR_PTR_DEF
#define FAR_PTR_DEF

typedef struct   // Union used to build far pointers under MetaWare
{
   unsigned off;
   unsigned short seg;
}
fp_ovl;

typedef union
{
   _Far void *ptr;
   fp_ovl part;
}
FARPTR;

#endif
#endif

//
// End of MetaWare conditionals
//

#ifdef __cplusplus
extern "C" {
#endif

//
// Misc. constant definitions  
//

#define MAX_DRVRS                16       // Max. # of simultaneous drivers
#define MAX_TIMERS               16       // Max. # of simultaneous timers
#define MAX_NOTES                32       // Max # of notes "on" 
#define FOR_NEST                 4        // # of nested XMIDI FOR loops
#define NUM_CHANS                16       // # of possible MIDI channels
#define MAX_W_VOICES             16       // Max virtual wave synth voice cnt
#define MAX_W_ENTRIES            512      // 512 wave library entries max.

#define MIN_CHAN                 ( 1-1)   // Min channel recognized (0-based)
#define MAX_CHAN                 (16-1)   // Max channel recognized
#define MIN_LOCK_CHAN            ( 2-1)   // Min channel available for locking
#define MAX_LOCK_CHAN            ( 9-1)   // Max channel available for locking
#define PERCUSS_CHAN             (10-1)   // Percussion channel (no locking)

#define DIG_F_MONO_8             0        // PCM data formats
#define DIG_F_MONO_16            1
#define DIG_F_STEREO_8           2
#define DIG_F_STEREO_16          3

#define DIG_F_16BITS_MASK        1
#define DIG_F_STEREO_MASK        2

#define DIG_PCM_SIGN             0x0001   // PCM flags
#define DIG_PCM_ORDER            0x0002

#define DIG_PCM_POLARITY         0x0004   // PCM flags used by driver hardware
#define DIG_PCM_SPLIT            0x0008
#define DIG_BUFFER_SERVICE       0x0010
#define DIG_DUAL_DMA             0x0020   
#define DIG_RECORDING_SUPPORTED  0x8000

#define AIL3DIG                  0        // .DIG driver
#define AIL3MDI                  1        // .MDI driver

#define DIG_DETECT_8_BIT_ONLY    0x0001   // Detect 8-bit DMA only
#define DIG_DETECT_16_BIT_ONLY   0x0002   // Detect 16-bit DMA only
#define DIG_DETECT_8_AND_16_BITS 0x0003   // Detect both 8- and 16-bit DMA

#define DRV_INIT          0x300     // Functions common to .MDI and .DIG 
#define DRV_GET_INFO      0x301     // drivers
#define DRV_SERVE         0x302
#define DRV_PARSE_ENV     0x303
#define DRV_VERIFY_IO     0x304
#define DRV_INIT_DEV      0x305
#define DRV_SHUTDOWN_DEV  0x306

#define DIG_HW_VOLUME     0x400     // .DIG driver functions
#define DIG_START_P_CMD   0x401
#define DIG_STOP_P_REQ    0x402
#define DIG_START_R_CMD   0x403
#define DIG_STOP_R_REQ    0x404
#define DIG_VSE           0x405

#define MDI_HW_VOLUME     0x500     // .MDI driver functions
#define MDI_INIT_INS_MGR  0x501
#define MDI_MIDI_XMIT     0x502
#define MDI_INSTALL_T_SET 0x503
#define MDI_GET_T_STATUS  0x504
#define MDI_PROT_UNPROT_T 0x505
#define MDI_VSE           0x506

//
// Non-specific XMIDI/MIDI controllers and event types
//

#define CHAN_MUTE         107
#define CALLBACK_PFX      108
#define SEQ_BRANCH        109
#define CHAN_LOCK         110
#define CHAN_PROTECT      111
#define VOICE_PROTECT     112
#define TIMBRE_PROTECT    113
#define PATCH_BANK_SEL    114
#define INDIRECT_C_PFX    115
#define FOR_LOOP          116
#define NEXT_LOOP         117
#define CLEAR_BEAT_BAR    118
#define CALLBACK_TRIG     119
#define SEQ_INDEX         120

#define MODULATION        1
#define DATA_MSB          6
#define PART_VOLUME       7
#define PANPOT            10
#define EXPRESSION        11
#define DATA_LSB          38
#define SUSTAIN           64
#define REVERB            91
#define CHORUS            93
#define RPN_LSB           100
#define RPN_MSB           101
#define ALL_NOTES_OFF     123

#define EV_NOTE_OFF       0x80
#define EV_NOTE_ON        0x90
#define EV_POLY_PRESS     0xa0
#define EV_CONTROL        0xb0
#define EV_PROGRAM        0xc0
#define EV_CHAN_PRESS     0xd0
#define EV_PITCH          0xe0
#define EV_SYSEX          0xf0
#define EV_ESC            0xf7
#define EV_META           0xff

#define META_EOT          0x2f
#define META_TEMPO        0x51
#define META_TIME_SIG     0x58

//
// SAMPLE.system_data[] usage
//

#define SSD_EOD_CALLBACK  0  // Application end-of-data callback if not NULL
#define VOC_BLK_PTR       1  // Pointer to current block                                   
#define VOC_REP_BLK       2  // Pointer to beginning of repeat loop block                  
#define VOC_N_REPS        3  // # of iterations left in repeat loop
#define VOC_MARKER        4  // Marker to search for, or -1 if all
#define VOC_MARKER_FOUND  5  // Desired marker found if 1, else 0
#define SSD_RELEASE       6  // Release sample handle upon termination if >0
#define SSD_TEMP          7  // Temporary storage location for general use

//
// SAMPLE.status flag values
//

#define SMP_FREE          0x0001    // Sample is available for allocation

#define SMP_DONE          0x0002    // Sample has finished playing, or has 
                                    // never been started

#define SMP_PLAYING       0x0004    // Sample is playing

#define SMP_STOPPED       0x0008    // Sample has been stopped

//
// SEQUENCE.status flag values
//

#define SEQ_FREE          0x0001    // Sequence is available for allocation

#define SEQ_DONE          0x0002    // Sequence has finished playing, or has 
                                    // never been started

#define SEQ_PLAYING       0x0004    // Sequence is playing

#define SEQ_STOPPED       0x0008    // Sequence has been stopped

//
// MIDI driver types
//

#define MDIDRVRTYPE_GM    0         // General MIDI driver (Roland-compatible)
#define MDIDRVRTYPE_FM_2  1         // 2-operator FM MIDI driver (OPL2)
#define MDIDRVRTYPE_FM_4  2         // 4-operator FM MIDI driver (OPL3)
#define MDIDRVRTYPE_SPKR  3         // Tandy or PC speaker "beep" driver

//
// .INI installation result codes
// 

#define AIL_INIT_SUCCESS  0         // Driver installed successfully
#define AIL_NO_INI_FILE   1         // No MDI.INI or DIG.INI file exists
#define AIL_INIT_FAILURE  2         // Driver could not be initialized

//
// General type definitions for portability
// 

#ifndef U8
#define U8 unsigned char
#endif

#ifndef S8
#define S8 signed char
#endif

#ifndef U16
#define U16 unsigned short
#endif

#ifndef S16
#define S16 signed short
#endif

#ifndef U32
#define U32 unsigned long
#endif

#ifndef S32
#define S32 signed long
#endif


//
// Type definitions
//

struct _DIG_DRIVER;

struct _MDI_DRIVER;

typedef struct _DIG_DRIVER * HDIGDRIVER;    // Handle to digital driver

typedef struct _MDI_DRIVER * HMDIDRIVER;    // Handle to XMIDI driver

typedef struct _SAMPLE * HSAMPLE;           // Handle to sample

typedef struct _SEQUENCE * HSEQUENCE;       // Handle to sequence

typedef S32 HTIMER;                         // Handle to timer


//
// Function pointer types
//

#define AILCALLBACK __pascal

typedef void (AILCALLBACK* AILTIMERCB)    (U32 user);

typedef void (AILCALLBACK* AILSAMPLECB)   (HSAMPLE sample);

typedef S32  (AILCALLBACK* AILEVENTCB)    (HMDIDRIVER hmi,HSEQUENCE seq,S32 status,S32 data_1,S32 data_2);

typedef S32  (AILCALLBACK* AILTIMBRECB)   (HMDIDRIVER hmi,S32 bank,S32 patch);

typedef S32  (AILCALLBACK* AILPREFIXCB)   (HSEQUENCE seq,S32 log,S32 data);

typedef void (AILCALLBACK* AILTRIGGERCB)  (HSEQUENCE seq,S32 log,S32 data);

typedef void (AILCALLBACK* AILBEATCB)     (HMDIDRIVER hmi,HSEQUENCE seq,S32 beat,S32 measure);

typedef void (AILCALLBACK* AILSEQUENCECB) (HSEQUENCE seq);

typedef U32 REALFAR;              // Real-mode far pointer type

#ifdef _X32_H_INCLUDED              // Macro to access low mem via seg:off ptr

 #ifndef REALPTR
 #define REALPTR(x) ((void *) (U32) ((((U32) (x))>>16<<4) + ((x) & 0xffff) \
                    + (U32) _x32_zero_base_ptr))
 #endif

#else
#ifdef __BORLANDC__

 #ifndef REALPTR       
 #define REALPTR(x) ((void *) (U32) ((((U32) (x))>>16<<4) + ((x) & 0xffff) \
                    - AIL_sel_base(_DS)))
 #endif

#else

 #ifndef REALPTR
 #define REALPTR(x) ((void *) (U32) ((((U32) (x))>>16<<4) + ((x) & 0xffff)))
 #endif

#endif
#endif

#define MIN_VAL 0
#define NOM_VAL 1
#define MAX_VAL 2

#ifndef YES
#define YES 1
#endif

#ifndef NO
#define NO  0
#endif


//
// Preference names and default values
//

#define DIG_SERVICE_RATE          0
#define DEFAULT_DSR               200     // DMA buffer-polling rate = 200 Hz
                                  
#define DIG_HARDWARE_SAMPLE_RATE  1
#define DEFAULT_DHSR              NOM_VAL // Use nominal sample rate by default

#define DIG_DMA_RESERVE           2
#define DEFAULT_DDR               32768   // Reserve 32K real-mode mem for DMA

#define DIG_LATENCY               3
#define DEFAULT_DL                100     // Half-buffer size in ms = 100

#define DIG_MIXER_CHANNELS        4     
#define DEFAULT_DMC               16      // 16 allocatable SAMPLE structures

#define DIG_DEFAULT_VOLUME        5   
#define DEFAULT_DDV               100     // Default sample volume = 100 (0-127)

#define DIG_RESAMPLING_TOLERANCE  6
#define DEFAULT_DRT               655     // Resampling triggered at +/- 1%
                                  
#define DIG_USE_STEREO            7
#define DEFAULT_DUS               NO      // Use mono output only
                                  
#define DIG_USE_16_BITS           8
#define DEFAULT_DU16              NO      // Use 8-bit output by default
                                  
#define DIG_ALLOW_16_BIT_DMA      9
#define DEFAULT_DA16DMA           YES     // OK to use 16-bit DMA if necessary

#define DIG_SS_LOCK               10   
#define DEFAULT_DSL               NO      // Don't disable IRQs while mixing

#define MDI_SERVICE_RATE          11
#define DEFAULT_MSR               120     // XMIDI sequencer timing = 120 Hz

#define MDI_SEQUENCES             12
#define DEFAULT_MS                8       // 8 sequence handles/driver

#define MDI_DEFAULT_VOLUME        13
#define DEFAULT_MDV               127     // Default sequence volume = 127 (0-127)

#define MDI_QUANT_ADVANCE         14      
#define DEFAULT_MQA               1       // Beat/bar count +1 interval

#define MDI_ALLOW_LOOP_BRANCHING  15      
#define DEFAULT_ALB               NO      // Branches cancel XMIDI FOR loops

#define MDI_DEFAULT_BEND_RANGE    16
#define DEFAULT_MDBR              2       // Default pitch-bend range = 2

#define AIL_SCAN_FOR_HARDWARE     17
#define DEFAULT_ASH               YES     // Scan for I/O settings if necessary

#define AIL_ALLOW_VDM_EXECUTION   18
#define DEFAULT_AVE               YES     // Allow Windows "DOS box" execution

#define MDI_DOUBLE_NOTE_OFF       19
#define DEFAULT_MDNO              NO      // For stuck notes on SB daughterboards

#define N_PREFS 20                        // # of preference types

//
// Type definitions
//

typedef struct                      // I/O parameters structure
{
   S16 IO;
   S16 IRQ;
   S16 DMA_8_bit;
   S16 DMA_16_bit;
   S32 IO_reserved[4];
}
IO_PARMS;

typedef struct                      // Standard MSS 3.X VDI driver header
{                                    
   S8     ID[8];                  // "AIL3xxx" ID string, followed by ^Z

   U32    driver_version;

   REALFAR  common_IO_configurations;
   U16    num_IO_configurations;

   REALFAR  environment_string;
   
   IO_PARMS IO;

   S16     service_rate;

   U16    busy;

   U16    driver_num;             // Driver number

   U16    this_ISR;               // Offset of INT 66H dispatcher
   REALFAR  prev_ISR;               // Pointer to previous INT 66H ISR

   S8     scratch[128];           // Shared scratch workspace
   
   S8     dev_name[80];           // Device name (VDI version >= 1.12 only)
}  
VDI_HDR;

typedef struct
{
   U16 minimum_physical_sample_rate;
   U16 nominal_physical_sample_rate;
   U16 maximum_physical_sample_rate;

   U16 minimum_DMA_half_buffer_size;
   U16 maximum_DMA_half_buffer_size;

   U32 flags;
}
DIG_MODE;

typedef struct
{
   U8    format_supported[16];
   DIG_MODE format_data[16];
}
DIG_DDT;

typedef struct
{
   REALFAR  DMA_buffer_A;
   REALFAR  DMA_buffer_B;
   S16     active_buffer;
}
DIG_DST;

typedef struct
{
   REALFAR  library_environment;
   REALFAR  GTL_suffix;

   U16    num_voices;

   U16    max_melodic_channel;
   U16    min_melodic_channel;
   U16    percussion_channel;
}
MDI_DDT;

typedef struct
{
   S8     library_directory[128];
   S8     GTL_filename[128];

   S8     MIDI_data[512];
}
MDI_DST;

typedef struct                   // Initialization file structure
{
   char     device_name[128];    // Device name
   char     driver_name[128];    // Driver filename
   IO_PARMS IO;                  // I/O parameters for driver
}
AIL_INI;

typedef struct                   // Handle to driver
{
   REALFAR  seg;                 // Seg:off pointer to driver (off=0)
   U32    sel;                 // Selector for driver (off=0)
   void    *buf;                 // Protected-mode pointer to driver
   U32    size;                // Size of driver image
   VDI_HDR *VHDR;                // Pointer to driver header (same as buf)
   S32     type;                // AIL3DIG or AIL3MDI (see below)

   S32     initialized;         // 1 if hardware successfully init'd, else 0

   S32     PM_ISR;              // -1 if no PM ISR hooked, else IRQ #

   HTIMER   server;              // DRV_SERVE periodic timer, if requested

                                 // Vector to high-level destructor, if any

   void (cdecl *destructor)(void *); 

                                 // High-level descriptor (DIG_ or MDI_DRIVER)
   void        *descriptor;          
}
AIL_DRIVER;

struct _DIG_DRIVER;

typedef struct _SAMPLE           // Sample instance
{
   S8     tag[4];              // HSAM

   struct _DIG_DRIVER *driver;   // Driver for playback

   U32    status;              // SMP_ flags: _FREE, _DONE, _PLAYING

   void    *start[2];            // Sample buffer address (W)
   U32    len  [2];            // Sample buffer size in bytes (W)
   U32    pos  [2];            // Index to next byte (R/W)
   U32    done [2];            // Nonzero if buffer with len=0 sent by app

   S32     current_buffer;      // Buffer # active (0/1)
   S32     last_buffer;         // Last active buffer (for double-buffering)
   S32     starved;             // Buffer stream has run out of data
          
   S32     loop_count;          // # of cycles-1 (1=one-shot, 0=indefinite)
   S32     loop_start;          // Starting offset of loop block (0=SOF)
   S32     loop_end;            // End offset of loop block (-1=EOF)
          
   S32     format;              // DIG_F format (8/16 bits, mono/stereo)
   U32    flags;               // DIG_PCM_SIGN / DIG_PCM_ORDER (stereo only)
                                 
   S32     playback_rate;       // Playback rate in hertz
                                 
   S32     volume;              // Sample volume 0-127
   S32     pan;                 // Mono panpot/stereo balance (0=L ... 127=R)

   S32     vol_scale[2][256];   // [left/mono=0,right=1][256] channel volume

   S32     service_type;        // 1 if single-buffered; 2 if streamed

   AILSAMPLECB SOB;             // Start-of-block callback function
   AILSAMPLECB EOB;             // End-of-buffer callback function
   AILSAMPLECB EOS;             // End-of-sample callback function

   S32     user_data  [8];      // Miscellaneous user data
   S32     system_data[8];      // Miscellaneous system data
}
SAMPLE;

typedef struct _DIG_DRIVER          // Handle to digital audio driver
{
   S8        tag[4];              // HDIG

   AIL_DRIVER *drvr;                // Base driver descriptor

   DIG_DDT    *DDT;                 // Protected-mode pointer to DDT
   DIG_DST    *DST;                 // Protected-mode pointer to DST

   HTIMER      timer;               // Buffer-polling timer

   S32        half_buffer_size;    // Size of DMA half-buffer
   S32        DMA_rate;            // Hardware sample rate
   S32        hw_format;           // DIG_F code in use
   U32       hw_mode_flags;       // DIG_PCM_ flags for mode in use

   REALFAR     DMA_seg;             // Seg:off pointer to DMA buffers (off=0)
   U32       DMA_sel;             // Selector for DMA buffers (off=0)
   void       *DMA_buf;             // Protected-mode pointer to DMA buffers

   void       *DMA[2];              // Protected-mode pointers to half-buffers
                                    // (note that DMA[0] may != DMA_buf)

   S16       *buffer_flag;         // Protected-mode pointer to buffer flag
   S32        last_buffer;         // Last active buffer flag value in driver

   S32        channels_per_sample; // # of channels per sample (1 or 2)
   S32        bytes_per_channel;   // # of bytes per channel (1 or 2)
   S32        channels_per_buffer; // # of channels per half-buffer
   S32        samples_per_buffer;  // # of samples per half-buffer

   S32        build_size;          // # of bytes in build buffer
   S32       *build_buffer;        // Build buffer (4 * n_samples bytes)

   S32        playing;             // Playback active if non-zero
   S32        quiet;               // # of consecutive quiet sample periods

   SAMPLE     *samples;             // Pointer to list of SAMPLEs
   S32        n_samples;           // # of SAMPLEs

   S32        n_active_samples;    // # of samples being processed

   S32        master_volume;       // Master sample volume 0-127

   S32        system_data[8];      // Miscellaneous system data
}
DIG_DRIVER;

typedef struct                      // MIDI status log structure    
   {
   S32     program   [NUM_CHANS];  // Program Change
   S32     pitch_l   [NUM_CHANS];  // Pitch Bend LSB
   S32     pitch_h   [NUM_CHANS];  // Pitch Bend MSB

   S32     c_lock    [NUM_CHANS];  // Channel Lock
   S32     c_prot    [NUM_CHANS];  // Channel Lock Protection
   S32     c_mute    [NUM_CHANS];  // Channel Mute
   S32     c_v_prot  [NUM_CHANS];  // Voice Protection
   S32     bank      [NUM_CHANS];  // Patch Bank Select
   S32     indirect  [NUM_CHANS];  // ICA indirect controller value
   S32     callback  [NUM_CHANS];  // Callback Trigger

   S32     mod       [NUM_CHANS];  // Modulation
   S32     vol       [NUM_CHANS];  // Volume
   S32     pan       [NUM_CHANS];  // Panpot
   S32     exp       [NUM_CHANS];  // Expression
   S32     sus       [NUM_CHANS];  // Sustain
   S32     reverb    [NUM_CHANS];  // Reverb
   S32     chorus    [NUM_CHANS];  // Chorus

   S32     bend_range[NUM_CHANS];  // Bender Range (data MSB, RPN 0 assumed)
   }
CTRL_LOG;

struct _MDI_DRIVER;

typedef struct _SEQUENCE                 // XMIDI sequence state table
{
   S8     tag[4];                        // HSEQ

   struct _MDI_DRIVER *driver;           // Driver for playback

   U32    status;                        // SEQ_ flags

   void    *TIMB;                        // XMIDI IFF chunk pointers
   void    *RBRN;           
   void    *EVNT;           

   U8   *EVNT_ptr;                       // Current event pointer

   U8   *ICA;                            // Indirect Controller Array

   AILPREFIXCB prefix_callback;          // XMIDI Callback Prefix handler
   AILTRIGGERCB trigger_callback;        // XMIDI Callback Trigger handler
   AILBEATCB beat_callback;              // XMIDI beat/bar change handler
   AILSEQUENCECB EOS;                    // End-of-sequence callback function

   S32     loop_count;                   // 0=one-shot, -1=indefinite, ...

   S32     interval_count;               // # of intervals until next event
   S32     interval_num;                 // # of intervals since start

   S32     volume;                       // Sequence volume 0-127
   S32     volume_target;                // Target sequence volume 0-127
   S32     volume_accum;                 // Accumulated volume period
   S32     volume_period;                // Period for volume stepping

   S32     tempo_percent;                // Relative tempo percentage 0-100
   S32     tempo_target;                 // Target tempo 0-100
   S32     tempo_accum;                  // Accumulated tempo period
   S32     tempo_period;                 // Period for tempo stepping
   S32     tempo_error;                  // Error counter for tempo DDA

   S32     beat_count;                   // Sequence playback position
   S32     measure_count;  

   S32     time_numerator;               // Sequence timing data
   S32     time_fraction;  
   S32     beat_fraction;  
   S32     time_per_beat;

   void    *FOR_ptrs       [FOR_NEST];    // Loop stack
   S32     FOR_loop_count [FOR_NEST];

   S32     chan_map       [NUM_CHANS];   // Physical channel map for sequence

   CTRL_LOG shadow;                       // Controller values for sequence

   S32     note_count;                   // # of notes "on"
                          
   S32     note_chan      [MAX_NOTES];   // Channel for queued note (-1=free)
   S32     note_num       [MAX_NOTES];   // Note # for queued note
   S32     note_time      [MAX_NOTES];   // Remaining duration in intervals

   S32     user_data  [8];               // Miscellaneous user data
   S32     system_data[8];               // Miscellaneous system data
}
SEQUENCE;

typedef struct _MDI_DRIVER         // Handle to XMIDI driver
{
   S8        tag[4];               // HMDI

   AIL_DRIVER *drvr;               // Base driver descriptor

   MDI_DDT    *DDT;                // Protected-mode pointer to DDT
   MDI_DST    *DST;                // Protected-mode pointer to DST

   HTIMER      timer;              // XMIDI quantization timer
   S32        interval_time;       // XMIDI quantization timer interval in uS

   S32        disable;             // > 0 to disable XMIDI service

   SEQUENCE   *sequences;          // Pointer to list of SEQUENCEs
   S32        n_sequences;         // # of SEQUENCEs

   S32        lock  [NUM_CHANS];   // 1 if locked, 2 if protected, else 0
   HSEQUENCE   locker[NUM_CHANS];  // HSEQUENCE which locked channel
   HSEQUENCE   owner [NUM_CHANS];  // HSEQUENCE which owned locked channel
   HSEQUENCE   user  [NUM_CHANS];  // Last sequence to use channel
   S32        state [NUM_CHANS];   // Lock state prior to being locked

   S32        notes [NUM_CHANS];   // # of active notes in channel

   AILEVENTCB  event_trap;         // MIDI event trap callback function
   AILTIMBRECB timbre_trap;        // MIDI timbre request callback function

   S32        message_count;       // MIDI message count
   S32        offset;              // MIDI buffer offset

   S32        master_volume;       // Master XMIDI note volume 0-127

   S32        system_data[8];      // Miscellaneous system data
}
MDI_DRIVER;

typedef struct                      // XMIDI TIMB IFF chunk
   {
   S8  name[4];

   U8 msb;
   U8 lsb;
   U8 lsb2;
   U8 lsb3;

   U16 n_entries;

   U16 timbre[1];
   }
TIMB_chunk;

typedef struct                      // XMIDI RBRN IFF entry
   {
   S16  bnum;
   U32 offset;
   }
RBRN_entry;

typedef struct                      // Wave library entry
{
   S32  bank;                      // XMIDI bank, MIDI patch for sample
   S32  patch;                     

   S32  root_key;                  // Root MIDI note # for sample (or -1)

   U32 file_offset;               // Offset of wave data from start-of-file
   U32 size;                      // Size of wave sample in bytes

   S32  format;                    // DIG_F format (8/16 bits, mono/stereo)
   U32 flags;                     // DIG_PCM_SIGN / DIG_PCM_ORDER (stereo)
   S32  playback_rate;             // Playback rate in hertz
}
WAVE_ENTRY;

typedef struct                      // Virtual "wave synthesizer" descriptor
{
   HMDIDRIVER  mdi;                 // MIDI driver for use with synthesizer
   HDIGDRIVER  dig;                 // Digital driver for use with synthesizer

   WAVE_ENTRY *library;             // Pointer to wave library

   AILEVENTCB  prev_event_fn;      // Previous MIDI event trap function
   AILTIMBRECB prev_timb_fn;       // Previous timbre request trap function

   CTRL_LOG    controls;            // MIDI controller states

   WAVE_ENTRY *wave [NUM_CHANS];    // Pointer to WAVE_ENTRY for each channel

   HSAMPLE     S    [MAX_W_VOICES]; // List of HSAMPLE voices
   S32        n_voices;             // Actual # of voices allocated to synth
                                    
   S32        chan [MAX_W_VOICES]; // MIDI channel for each voice, or -1
   S32        note [MAX_W_VOICES]; // MIDI note number for voice
   S32        root [MAX_W_VOICES]; // MIDI root note for voice
   S32        rate [MAX_W_VOICES]; // Playback rate for voice
   S32        vel  [MAX_W_VOICES]; // MIDI note velocity for voice
   U32       time [MAX_W_VOICES];  // Timestamp for voice

   U32       event;                // Event counter for LRU timestamps
}
WAVE_SYNTH;

typedef WAVE_SYNTH * HWAVESYNTH;   // Handle to virtual wave synthesizer

typedef struct                     // VDI interface register structure
{
   S16 AX;
   S16 BX;
   S16 CX;
   S16 DX;
   S16 SI;
   S16 DI;
}
VDI_CALL;

//
// Global preference array
//

extern S32           AIL_preference   [N_PREFS];

//
// AIL fatal error type string
//
                                      
extern S8           AIL_error        [256];

//
// AIL alternate driver directory
//
                                      
extern S8           AIL_driver_directory [256];

//
// Last IO_PARMS structure used to attempt device detection
//

extern IO_PARMS       AIL_last_IO_attempt;

//
// Quick-integration service functions and data types
//

typedef struct
{
   U32* data;

   S32  type;
   void *handle;

   S32  status;

   void* next;

   S32  speed;
   S32  volume;
   S32  extravol;
}
AUDIO_TYPE;

#define QSTAT_DONE          1       // Data has finished playing
#define QSTAT_LOADED        2       // Data has been loaded, but not yet played
#define QSTAT_PLAYING       3       // Data is currently playing

typedef AUDIO_TYPE * HAUDIO;        // Generic handle to any audio data type

extern S32    cdecl  AIL_quick_startup_with_start   (void* startup,
                                                     S32        use_digital,
                                                     S32        use_MIDI,
                                                     U32        output_rate,
                                                     S32        output_bits,
                                                     S32        output_channels);
extern void    cdecl  AIL_quick_shutdown            (void);

extern void    cdecl  AIL_quick_handles  (HDIGDRIVER* pdig, HMDIDRIVER* pmdi);

extern HAUDIO  cdecl  AIL_quick_load                (char       *filename);

extern HAUDIO  cdecl  AIL_quick_copy                (HAUDIO      audio);

extern void    cdecl  AIL_quick_unload              (HAUDIO      audio);

extern S32    cdecl  AIL_quick_play                (HAUDIO      audio,
                                                     S32        loop_count);

extern void    cdecl  AIL_quick_halt                (HAUDIO      audio);

extern S32    cdecl  AIL_quick_status              (HAUDIO      audio);

extern HAUDIO  cdecl  AIL_quick_load_and_play       (char      *filename, 
                                                     S32        loop_count,
                                                     S32        wait_request);

extern void    cdecl  AIL_quick_set_speed      (HAUDIO      audio, S32 speed);

extern void    cdecl  AIL_quick_set_volume   (HAUDIO audio, S32 volume, S32 extravol);

//
// High-level support services
//

#ifdef __SW_3R
extern S32    cdecl  AIL_startup_reg               (void);
#define AIL_startup AIL_startup_reg
#else
extern S32    cdecl  AIL_startup_stack             (void);
#define AIL_startup AIL_startup_stack
#endif

#define AIL_quick_startup(ud,um,opr,opb,ops) AIL_quick_startup_with_start(&AIL_startup,ud,um,opr,opb,ops)

extern void    cdecl  AIL_shutdown                  (void);

extern char*  cdecl  AIL_set_driver_directory      (char  *dir);

#define AIL_get_preference(number) (AIL_preference[number])

extern S32    cdecl  AIL_set_preference            (U32       number, 
                                                     S32        value);

extern S32    cdecl  AIL_set_preference            (U32       number,
                                                     S32        value);

extern char*  cdecl  AIL_last_error                (void);

//
// Low-level file i/o routines
//

U32  cdecl AIL_fopen(char* dest,U32 acc);
U32  cdecl AIL_fcreate(char* dest);
U32  cdecl AIL_fappend(char* dest);
U32  cdecl AIL_fread(U32 hand,void* dest,U32 bytes);
U32  cdecl AIL_fwrite(U32 hand,void* dest,U32 bytes);
U32  cdecl AIL_fseekbegin(U32 hand,U32 pos);
U32  cdecl AIL_fseekcur(U32 hand,U32 pos);
U32  cdecl AIL_fseekend(U32 hand,U32 pos);
void cdecl AIL_fclose(U32 hand);

//
// Stack vs. register mapping routines
//

extern char* __pascal (*AIL_getenv) (char* vari);
extern void __pascal (*AIL_int386) (U32 intnum,void* inr, void* outr);
extern void __pascal (*AIL_sprintf) (char* dest, char* format, U32 num, U32*params);

//
// Low-level inline routines for watcom
//

#ifdef __WATCOMC__

S32 AIL_abs(S32 ab);
#pragma aux AIL_abs = "test eax,eax" "jge skip" "neg eax" "skip:" parm [eax];

void AIL_memset(void* dest,S8 value,U32 len);
#pragma aux AIL_memset = "mov ah,al" "mov bx,ax" "shl eax,16" "mov ax,bx" "mov bl,cl" "shr ecx,2" "and bl,3" "rep stosd" "mov cl,bl" "rep stosb" parm [EDI] [AL] [ECX] modify [EAX EDX EBX ECX EDI];

void AIL_memcpy(void* dest,const void* source,U32 size);
#pragma aux AIL_memcpy = "mov bl,cl" "shr ecx,2" "and bl,3" "rep movsd" "mov cl,bl" "rep movsb" parm [EDI] [ESI] [ECX] modify [EBX ECX EDI ESI];

char* AIL_strcpy(void* dest,const void* source);
#pragma aux AIL_strcpy = "mov edx,edi" "lp:" "mov al,[esi]" "inc esi" "mov [edi],al" "inc edi" "cmp al,0" "jne lp" parm [EDI] [ESI] modify [EAX EDX EDI ESI] value [EDX];

U32 AIL_strlen(const void* dest);
#pragma aux AIL_strlen = "mov ecx,0xffffffff" "xor eax,eax" "repne scasb" "not ecx" "dec ecx" parm [EDI] modify [EAX ECX EDI] value [ECX];
    
char* AIL_strcat(void* dest,const void* source);
#pragma aux AIL_strcat = "cld" "mov ecx,0xffffffff" "mov edx,edi" "xor eax,eax" "repne scasb" "dec edi" "lp:" "lodsb" "stosb" "test al,0xff" "jnz lp" \
  parm [EDI] [ESI] modify [EAX ECX EDI ESI] value [EDX];
    
S8 AIL_memcmp(const void* s1,const void* s2,U32 len);
#pragma aux AIL_memcmp = "cld" "rep cmpsb" "setne al" "jbe end" "neg al" "end:"  parm [EDI] [ESI] [ECX] modify [ECX EDI ESI];

S8 AIL_strcmp(const void* s1,const void* s2);
#pragma aux AIL_strcmp = "lp:" "mov al,[esi]" "mov ah,[edi]" "cmp al,ah" "jne set" "cmp al,0" "je set" "inc esi" "inc edi" "jmp lp" "set:" "setne al" "jbe end" "neg al" "end:" \
   parm [EDI] [ESI] modify [EAX EDI ESI];

S8 AIL_stricmp(const void* s1,const void* s2);                                                                       
#pragma aux AIL_stricmp = "lp:" "mov al,[esi]" "mov ah,[edi]" "cmp al,'a'" "jb c1" "cmp al,'z'" "ja c1" "sub al,32" "c1:" "cmp ah,'a'" "jb c2" "cmp ah,'z'" "ja c2" "sub ah,32" "c2:" "cmp al,ah" "jne set" "cmp al,0" "je set" \
   "inc esi" "inc edi" "jmp lp" "set:" "setne al" "jbe end" "neg al" "end:" \
   parm [EDI] [ESI] modify [EAX EDI ESI];

S8 AIL_strnicmp(const void* s1, const void* s2,U32 len);
#pragma aux AIL_strnicmp = "lp:" "mov al,[esi]" "mov ah,[edi]" "cmp al,'a'" "jb c1" "cmp al,'z'" "ja c1" "sub al,32" "c1:" "cmp ah,'a'" "jb c2" "cmp ah,'z'" "ja c2" "sub ah,32" "c2:" "cmp al,ah" "jne set" "cmp al,0" "je set" \
   "dec ecx" "jz set" "inc esi" "inc edi" "jmp lp" "set:" "setne al" "jbe end" "neg al" "end:" \
   parm [EDI] [ESI] [ECX] modify [EAX ECX EDI ESI];

U32 AIL_atoi(const void* dest);
 #pragma aux AIL_atoi = "cld" "xor ecx,ecx" "xor ebx,ebx" "xor edi,edi" "lodsb" "cmp al,45" "jne skip2" "mov edi,1" "jmp skip" "lp:" "mov eax,10" "mul ecx" "lea ecx,[eax+ebx]" \
 "skip:" "lodsb" "skip2:" "cmp al,0x39" "ja dne" "cmp al,0x30" "jb dne" "mov bl,al" "sub bl,0x30" "jmp lp" "dne:" "test edi,1" "jz pos" "neg ecx" "pos:" \
 parm [ESI] modify [EAX EBX EDX EDI ESI] value [ecx];

void MarkEBXAsChanged();
#pragma aux MarkEBXAsChanged = "" modify [ebx];

#else

#include "stdlib.h"
#include "string.h"

#define AIL_abs abs
#define AIL_memset memset
#define AIL_memcpy memcpy
#define AIL_strcpy strcpy 
#define AIL_strlen strlen
#define AIL_strcat strcat
#define AIL_memcmp memcmp
#define AIL_strcmp strcmp
#define AIL_stricmp stricmp
#define AIL_strnicmp strnicmp
#define AIL_atoi(str) ((U32)atoi(str))
#define MarkEBXAsChanged()

#endif


//
// Low-level support services
//

extern void    cdecl  AIL_lock                      (void);
extern void    cdecl  AIL_unlock                    (void);


extern REALFAR cdecl  AIL_get_real_vect             (U32       vectnum);
extern void    cdecl  AIL_set_real_vect             (U32       vectnum, 
                                                     REALFAR     real_ptr);

extern void    cdecl  AIL_set_USE16_ISR             (S32        IRQ,
                                                     REALFAR     real_base,
                                                     U32       ISR_offset);

extern void    cdecl  AIL_restore_USE16_ISR         (S32        IRQ);

extern U32   cdecl  AIL_disable_interrupts        (void);
extern void    cdecl  AIL_restore_interrupts        (U32       FD_register);

extern void    cdecl  AIL_switch_stack              (void       *stack, 
                                                     U32       size, 
                                                     U32      *SS, 
                                                     void      **ESP, 
                                                     void      **EBP);

extern void    cdecl  AIL_restore_stack             (U32       SS, 
                                                     void       *ESP, 
                                                     void       *EBP);

extern S32    cdecl  AIL_call_driver               (AIL_DRIVER *drvr, 
                                                     S32        fn,
                                                     VDI_CALL   *in, 
                                                     VDI_CALL   *out);

extern void    cdecl  AIL_delay                     (S32        intervals);

extern S32    cdecl  AIL_background                (void);

extern S32    cdecl  AIL_read_INI                  (AIL_INI    *INI,
                                                     char      *filename);

//                 
// Process services
//

extern U32   cdecl AIL_ms_count(void);

extern HTIMER  cdecl  AIL_register_timer            (AILTIMERCB  callback_fn);

extern U32   cdecl  AIL_set_timer_user            (HTIMER      timer, 
                                                     U32       user);

extern void    cdecl  AIL_set_timer_period          (HTIMER      timer, 
                                                     U32       microseconds);

extern void    cdecl  AIL_set_timer_frequency       (HTIMER      timer, 
                                                     U32       hertz);

extern void    cdecl  AIL_set_timer_divisor         (HTIMER      timer, 
                                                     U32       PIT_divisor);

extern U32   cdecl  AIL_interrupt_divisor         (void);

extern void    cdecl  AIL_start_timer               (HTIMER      timer);
extern void    cdecl  AIL_start_all_timers          (void);

extern void    cdecl  AIL_stop_timer                (HTIMER      timer);
extern void    cdecl  AIL_stop_all_timers           (void);

extern void    cdecl  AIL_release_timer_handle      (HTIMER      timer);
extern void    cdecl  AIL_release_all_timers        (void);

//
// Low-level installation services
//

extern IO_PARMS *   cdecl AIL_get_IO_environment    (AIL_DRIVER *drvr);

extern AIL_DRIVER * cdecl AIL_install_driver        (U8      *driver_image, 
                                                     U32       n_bytes);

extern void         cdecl AIL_uninstall_driver      (AIL_DRIVER *drvr);

//
// High-level digital services
//

extern S32         cdecl AIL_install_DIG_INI       (HDIGDRIVER *dig);

extern HDIGDRIVER   cdecl AIL_install_DIG_driver_file 
                                                    (char       *filename, 
                                                     IO_PARMS   *IO);

extern void         cdecl AIL_uninstall_DIG_driver  (HDIGDRIVER  dig);

extern HSAMPLE      cdecl AIL_allocate_sample_handle 
                                                    (HDIGDRIVER  dig);

extern HSAMPLE      cdecl AIL_allocate_file_sample  (HDIGDRIVER  dig, 
                                                     void       *file_image, 
                                                     S32        block);

extern void         cdecl AIL_release_sample_handle (HSAMPLE S);
                                                 
extern void         cdecl AIL_init_sample           (HSAMPLE S);

extern S32         cdecl AIL_set_sample_file       (HSAMPLE S,
                                                     void   *file_image,
                                                     S32    block);

extern void         cdecl AIL_set_sample_address    (HSAMPLE S, 
                                                     void   *start, 
                                                     U32   len);
                                                 
extern void         cdecl AIL_set_sample_type       (HSAMPLE S, 
                                                     S32    format, 
                                                     U32   flags);

extern void         cdecl AIL_start_sample          (HSAMPLE S);
extern void         cdecl AIL_stop_sample           (HSAMPLE S);
extern void         cdecl AIL_resume_sample         (HSAMPLE S);
extern void         cdecl AIL_end_sample            (HSAMPLE S);

extern void         cdecl AIL_set_sample_playback_rate 
                                                    (HSAMPLE S, 
                                                     S32    playback_rate);

extern void         cdecl AIL_set_sample_volume     (HSAMPLE S, 
                                                     S32    volume);

extern void         cdecl AIL_set_sample_pan        (HSAMPLE S, 
                                                     S32    pan);

extern void         cdecl AIL_set_sample_loop_count (HSAMPLE S, 
                                                     S32    loop_count);

extern void         cdecl AIL_set_sample_loop_block (HSAMPLE S,
                                                     S32    loop_start_offset,
                                                     S32    loop_end_offset);

extern U32        cdecl AIL_sample_status         (HSAMPLE S);
extern S32         cdecl AIL_sample_playback_rate  (HSAMPLE S);
extern S32         cdecl AIL_sample_volume         (HSAMPLE S);
extern S32         cdecl AIL_sample_pan            (HSAMPLE S);
extern S32         cdecl AIL_sample_loop_count     (HSAMPLE S);

extern void         cdecl AIL_set_digital_master_volume
                                                    (HDIGDRIVER dig, 
                                                     S32       master_volume);

extern S32         cdecl AIL_digital_master_volume (HDIGDRIVER dig);

//
// SoundStream(TM) low-level digital services
//

extern HDIGDRIVER   cdecl AIL_install_DIG_driver_image 
                                                    (void     *driver_image,
                                                     U32     size,
                                                     IO_PARMS *IO);

extern S32     cdecl AIL_minimum_sample_buffer_size(HDIGDRIVER  dig,
                                                     S32        playback_rate,
                                                     S32        format);       

extern S32     cdecl AIL_sample_buffer_ready       (HSAMPLE S);

extern void     cdecl AIL_load_sample_buffer        (HSAMPLE S,
                                                     U32   buff_num,
                                                     void   *buffer,
                                                     U32   len);

extern S32     cdecl AIL_sample_buffer_info        (HSAMPLE  S,
                                                     U32   *pos0,
                                                     U32   *len0,
                                                     U32   *pos1,
                                                     U32   *len1);

extern void     cdecl AIL_set_sample_position       (HSAMPLE S, 
                                                     U32   pos);

extern U32    cdecl AIL_sample_position             (HSAMPLE S);

extern AILSAMPLECB cdecl AIL_register_SOB_callback  (HSAMPLE S, 
                                                     AILSAMPLECB SOB);

extern AILSAMPLECB cdecl AIL_register_EOB_callback  (HSAMPLE S, 
                                                     AILSAMPLECB EOB);

extern AILSAMPLECB cdecl AIL_register_EOS_callback  (HSAMPLE S, 
                                                     AILSAMPLECB EOS);

extern AILSAMPLECB cdecl AIL_register_EOF_callback  (HSAMPLE S, 
                                                     AILSAMPLECB EOFILE);

extern void     cdecl AIL_set_sample_user_data      (HSAMPLE S,
                                                     U32   index,
                                                     S32    value);

extern S32     cdecl AIL_sample_user_data          (HSAMPLE S,
                                                     U32   index);

extern S32     cdecl AIL_active_sample_count       (HDIGDRIVER  dig);

extern void     cdecl AIL_digital_configuration     (HDIGDRIVER dig,
                                                     S32      *rate,
                                                     S32      *format,
                                                     char     *name);

//
// High-level XMIDI services
//

extern S32         cdecl AIL_install_MDI_INI       (HMDIDRIVER *mdi);

extern HMDIDRIVER   cdecl AIL_install_MDI_driver_file 
                                                    (char       *filename, 
                                                     IO_PARMS   *IO);

extern void         cdecl AIL_uninstall_MDI_driver  (HMDIDRIVER  mdi);

extern HSEQUENCE    cdecl AIL_allocate_sequence_handle 
                                                    (HMDIDRIVER  mdi);

extern void         cdecl AIL_release_sequence_handle 
                                                    (HSEQUENCE S);
                                                 
extern S32         cdecl AIL_init_sequence         (HSEQUENCE S,
                                                     void     *start,
                                                     S32      sequence_num);

extern void         cdecl AIL_start_sequence        (HSEQUENCE S);
extern void         cdecl AIL_stop_sequence         (HSEQUENCE S);
extern void         cdecl AIL_resume_sequence       (HSEQUENCE S);
extern void         cdecl AIL_end_sequence          (HSEQUENCE S);

extern void         cdecl AIL_set_sequence_tempo    (HSEQUENCE S, 
                                                     S32      tempo,
                                                     S32      milliseconds);

extern void         cdecl AIL_set_sequence_volume   (HSEQUENCE S, 
                                                     S32      volume,
                                                     S32      milliseconds);

extern void         cdecl AIL_set_sequence_loop_count
                                                    (HSEQUENCE S, 
                                                     S32      loop_count);

extern U32        cdecl AIL_sequence_status       (HSEQUENCE S);
                                                   
extern S32         cdecl AIL_sequence_tempo        (HSEQUENCE S);
extern S32         cdecl AIL_sequence_volume       (HSEQUENCE S);
extern S32         cdecl AIL_sequence_loop_count   (HSEQUENCE S);

extern void         cdecl AIL_set_XMIDI_master_volume
                                                    (HMDIDRIVER mdi, 
                                                     S32       master_volume);

extern S32         cdecl AIL_XMIDI_master_volume   (HMDIDRIVER mdi);


//
// Low-level XMIDI services
//

extern HMDIDRIVER   cdecl AIL_install_MDI_driver_image 
                                                    (void       *driver_image,
                                                     U32       size,
                                                     IO_PARMS   *IO);

extern S32     cdecl AIL_MDI_driver_type           (HMDIDRIVER  mdi);

extern void     cdecl AIL_set_GTL_filename_prefix   (char      *prefix);

extern S32     cdecl AIL_timbre_status             (HMDIDRIVER  mdi,
                                                     S32        bank,
                                                     S32        patch);

extern S32     cdecl AIL_install_timbre            (HMDIDRIVER  mdi,
                                                     S32        bank,
                                                     S32        patch);

extern void     cdecl AIL_protect_timbre            (HMDIDRIVER  mdi,
                                                     S32        bank,
                                                     S32        patch);

extern void     cdecl AIL_unprotect_timbre          (HMDIDRIVER  mdi,
                                                     S32        bank,
                                                     S32        patch);

extern S32     cdecl AIL_active_sequence_count     (HMDIDRIVER  mdi);

extern S32     cdecl AIL_controller_value          (HSEQUENCE S,
                                                     S32      channel,
                                                     S32      controller_num);

extern S32     cdecl AIL_channel_notes             (HSEQUENCE S,
                                                     S32      channel);

extern void     cdecl AIL_sequence_position         (HSEQUENCE S,
                                                     S32     *beat,
                                                     S32     *measure);

extern void     cdecl AIL_branch_index              (HSEQUENCE S,
                                                     U32     marker);

extern AILPREFIXCB cdecl AIL_register_prefix_callback(HSEQUENCE S,
                                                      AILPREFIXCB  callback);

extern AILTRIGGERCB cdecl AIL_register_trigger_callback (HSEQUENCE S,
                                                     AILTRIGGERCB  callback);

extern AILSEQUENCECB cdecl AIL_register_sequence_callback
                                                    (HSEQUENCE S,
                                                     AILSEQUENCECB  callback);

extern AILBEATCB cdecl AIL_register_beat_callback   (HSEQUENCE   S,
                                                     AILBEATCB    callback);

extern AILEVENTCB cdecl AIL_register_event_callback (HMDIDRIVER  mdi,
                                                     AILEVENTCB   callback);

extern AILTIMBRECB cdecl AIL_register_timbre_callback  (HMDIDRIVER  mdi,
                                                     AILTIMBRECB    callback);

extern void     cdecl AIL_set_sequence_user_data    (HSEQUENCE S,
                                                     U32     index,
                                                     S32      value);

extern S32     cdecl AIL_sequence_user_data        (HSEQUENCE S,
                                                     U32     index);

extern void     cdecl AIL_register_ICA_array        (HSEQUENCE S,
                                                     U8     *array);

extern S32     cdecl AIL_lock_channel              (HMDIDRIVER  mdi);

extern void     cdecl AIL_release_channel           (HMDIDRIVER  mdi,
                                                     S32        channel);

extern void     cdecl AIL_map_sequence_channel      (HSEQUENCE S,
                                                     S32      seq_channel,
                                                     S32      new_channel);

extern S32     cdecl AIL_true_sequence_channel     (HSEQUENCE S,
                                                     S32      seq_channel);

extern void     cdecl AIL_send_channel_voice_message
                                                    (HMDIDRIVER  mdi,
                                                     HSEQUENCE   S,
                                                     S32        status,
                                                     S32        data_1,
                                                     S32        data_2);

extern void     cdecl AIL_send_sysex_message        (HMDIDRIVER  mdi,
                                                     void       *buffer);

extern HWAVESYNTH cdecl AIL_create_wave_synthesizer   (HDIGDRIVER  dig,
                                                     HMDIDRIVER  mdi,
                                                     void       *wave_lib,
                                                     S32        polyphony);

extern void     cdecl AIL_destroy_wave_synthesizer  (HWAVESYNTH W);

typedef struct _REDBOOKTRACKINFO {
  U32 tracks;
  U32 trackstarts[100];
} REDBOOKTRACKINFO;

typedef struct _REDBOOK {
  U32 DeviceID;
  U32 paused;
  U32 pausedsec;
  U32 lastendsec;
  U32 readcontents;
  REDBOOKTRACKINFO info;
} REDBOOK;

typedef struct _REDBOOK* HREDBOOK;

#define REDBOOK_ERROR    0
#define REDBOOK_PLAYING  1
#define REDBOOK_PAUSED   2
#define REDBOOK_STOPPED  3


extern  HREDBOOK  cdecl AIL_redbook_open(U32 which);

extern  void      cdecl AIL_redbook_close(HREDBOOK hand);

extern  void      cdecl AIL_redbook_eject(HREDBOOK hand);

extern  void      cdecl AIL_redbook_retract(HREDBOOK hand);

extern  U32       cdecl AIL_redbook_status(HREDBOOK hand);

extern  U32       cdecl AIL_redbook_tracks(HREDBOOK hand);

extern  void      cdecl AIL_redbook_track_info(HREDBOOK hand,U32 tracknum,
                                                     U32* startmsec,U32* endmsec);

extern  U32       cdecl AIL_redbook_id(HREDBOOK hand);

extern  U32       cdecl AIL_redbook_position(HREDBOOK hand);

extern  U32       cdecl AIL_redbook_track(HREDBOOK hand);

extern  U32       cdecl AIL_redbook_play(HREDBOOK hand,U32 startmsec, U32 endmsec);

extern  U32       cdecl AIL_redbook_stop(HREDBOOK hand);

extern  U32       cdecl AIL_redbook_pause(HREDBOOK hand);

extern  U32       cdecl AIL_redbook_resume(HREDBOOK hand);

extern  S32       cdecl AIL_redbook_volume(HREDBOOK hand);

extern  S32       cdecl AIL_redbook_set_volume(HREDBOOK hand, S32 volume);


#ifndef FILE_ERRS
#define FILE_ERRS

#define NO_ERROR        0
#define IO_ERROR        1
#define OUT_OF_MEMORY   2
#define FILE_NOT_FOUND  3
#define CANT_WRITE_FILE 4
#define CANT_READ_FILE  5
#define DISK_FULL       6

#endif

//
// malloc() / free() wrapper functions
// 

#ifndef NO_OLD_SYS_FUNCTIONS

#define MEM_alloc_lock   AIL_mem_alloc_lock
#define MEM_free_lock    AIL_mem_free_lock
#define FILE_error       AIL_file_error
#define FILE_size        AIL_file_size
#define FILE_read        AIL_file_read
#define FILE_write       AIL_file_write
#define FILE_append      AIL_file_append
#define MEM_alloc        AIL_mem_alloc
#define MEM_free         AIL_mem_free
#define MEM_use_malloc   AIL_mem_use_malloc
#define MEM_use_free     AIL_mem_use_free
#define MEM_alloc_DOS    AIL_mem_alloc_DOS
#define MEM_free_DOS     AIL_mem_free_DOS
#define VMM_lock_range   AIL_vmm_lock_range
#define VMM_unlock_range AIL_vmm_unlock_range
#define VMM_lock         AIL_vmm_lock
#define VMM_unlock       AIL_vmm_unlock

#endif
extern void * AILCALLBACK (*AIL_mem_alloc) (U32);
extern void   AILCALLBACK (*AIL_mem_free)  (void *);

void * cdecl AIL_mem_use_malloc(void * AILCALLBACK (*fn)(U32));
void * cdecl AIL_mem_use_free  (void AILCALLBACK (*fn)(void *));

//
// Other memory-management functions
//

S32   cdecl AIL_mem_alloc_DOS (U32  n_paras, 
                               void **protected_ptr, 
                               U32 *segment_far_ptr,
                               U32 *selector);

void   cdecl AIL_mem_free_DOS (void  *protected_ptr, 
                               U32  segment_far_ptr,
                               U32  selector);

void * cdecl AIL_mem_alloc_lock   (U32 size);

void   cdecl AIL_mem_free_lock    (void  *ptr); 

S32   cdecl AIL_vmm_lock_range   (void  *p1, void *p2);
S32   cdecl AIL_vmm_unlock_range (void  *p1, void *p2);

S32   cdecl AIL_vmm_lock         (void  *start, U32 size);
S32   cdecl AIL_vmm_unlock       (void  *start, U32 size);

U32  cdecl AIL_sel_base         (U32  sel);

void   cdecl AIL_sel_set_limit  (U32  sel,
                               U32  limit);

//
// File functions
//

#define FILE_READ_WITH_SIZE ((void*)(S32)-1)

S32   cdecl AIL_file_error (void);
S32   cdecl AIL_file_size  (char *filename);
void * cdecl AIL_file_read  (char *filename, void *dest);
S32   cdecl AIL_file_write (char *filename, void *buf, U32 len);
S32   cdecl AIL_file_append(char *filename, void *buf, U32 len);


typedef struct _STREAM * HSTREAM;           // Handle to stream

typedef void (AILCALLBACK* AILSTREAMCB)   (HSTREAM stream);

typedef struct _STREAM {

  HSAMPLE samp;    // the sample handle

  U32 fileh;       // the open file handle

  U8* bufs[3];     // the data buffers
  U32 bufsizes[3]; // the size of each buffer

  S32 bufsize;     // size of each buffer
  S32 readsize;    // size of each read block

  U32 buf1;        // 0,1,2 (current buffer that we are reading into)
  S32 size1;       // holds the current amount of data read

  U32 buf2;        // 0,1,2 (the next buffer that we are reading into)
  S32 size2;       // next buffer loaded up to

  U32 buf3;        // 0,1,2 (the next buffer that we are reading into)
  S32 size3;       // next buffer loaded up to

  U32 datarate;    // datarate in bytes per second
  S32 filerate;    // original datarate of the file
  S32 filetype;    // file format type
  U32 fileflags;   // file format flags (signed or unsigned)
  S32 totallen;    // total length of the sound data

  S32 loadedsize;  // size that has been loaded
  S32 loadedsome;  // have we done any loads?

  U32 startpos;    // point that the sound data begins
  U32 totalread;   // total bytes read from the disk

  U32 loopsleft;   // how many loops are left

  U32 error;       // read error has occurred

  S32 preload;     // preload the file into the first buffer
  S32 dontclose;   // don't close the file
  S32 noback;      // no background processing
  S32 alldone;     // alldone
  S32 primeamount; // amount to load after a seek
  S32 readatleast; // forced amount to read on next service

  S32 playcontrol; // control: 0=stopped, 1=started, |8=paused, |16=sample paused

  AILSTREAMCB callback;  // end of stream callback

  S32 user_data[8];      // Miscellaneous user data
  void * next;           // pointer to next stream

} STREAM_TYPE;


extern HSTREAM cdecl AIL_open_stream(HDIGDRIVER dig, char * filename, S32 stream_mem);

extern void cdecl AIL_close_stream(HSTREAM stream);

extern S32 cdecl AIL_service_stream(HSTREAM stream, S32 fillup);

extern void cdecl AIL_start_stream(HSTREAM stream);

extern void cdecl AIL_pause_stream(HSTREAM stream, S32 onoff);

extern void cdecl AIL_set_stream_volume(HSTREAM stream,S32 volume);

extern void cdecl AIL_set_stream_pan(HSTREAM stream,S32 pan);

extern S32 cdecl AIL_stream_volume(HSTREAM stream);

extern S32 cdecl AIL_stream_pan(HSTREAM stream);

extern void cdecl AIL_set_stream_playback_rate(HSTREAM stream, S32 rate);

extern S32 cdecl AIL_stream_playback_rate(HSTREAM stream);

extern S32 cdecl AIL_stream_loop_count(HSTREAM stream);

extern void cdecl AIL_set_stream_loop_count(HSTREAM stream, S32 count);

extern S32 cdecl AIL_stream_status(HSTREAM stream);

extern void cdecl AIL_set_stream_position(HSTREAM stream,S32 offset);

extern S32 cdecl AIL_stream_position(HSTREAM stream);

extern void cdecl AIL_stream_info(HSTREAM stream, S32* datarate, S32* sndtype, S32* length, S32* memory);

extern AILSTREAMCB cdecl AIL_register_stream_callback(HSTREAM stream, AILSTREAMCB callback);

extern void     cdecl AIL_set_stream_user_data      (HSAMPLE S,
                                                     U32   index,
                                                     S32    value);

extern S32     cdecl AIL_stream_user_data          (HSAMPLE S,
                                                     U32   index);


#ifdef __cplusplus
}
#endif
   
//
// MetaWare support
//

#ifdef __HIGHC__
#pragma Global_aliasing_convention();
#endif


#ifdef __WATCOMC__
  #ifdef _PUSHPOP_SUPPORTED
    #pragma pack(pop)
  #else
    #pragma pack()
  #endif
#endif


#endif

#endif

