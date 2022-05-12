#ifndef ___ERROR_H__
#define ___ERROR_H__


//---------------------------------------------------------------------------
//
//	Copyright (c) 1995, 1996. Windward Studios, Inc.
//	All Rights Reserved.
//
//---------------------------------------------------------------------------


enum 
{
	ERR_BASE_USER_ERROR = ( 1 << 29 ),
	ERR_MMIO_OPEN,
	ERR_MMIO_DESCEND_RIFF,
	ERR_MMIO_DESCEND_LIST,
	ERR_MMIO_DESCEND_CHUNK,
	ERR_MMIO_ASCEND_LIST,
	ERR_MMIO_ASCEND_CHUNK,
	ERR_MMIO_READ,
	ERR_MMIO_REWIND,
	ERR_DATAFILE_OPEN,
	ERR_DATAFILE_BAD_MAGIC,
	ERR_DATAFILE_NO_ENTRY,
	ERR_DATAFILE_READ,
	ERR_CACHE_READ,
	ERR_PATCHFILE_OPEN,
	ERR_FLC_READ_FRAME_HDR,
	ERR_FLC_FRAME_HDR_SIZE,
	ERR_FLC_READ_FRAME,
	ERR_FLC_READ_HDR,
	ERR_FLC_TYPE,
	ERR_FLC_COLOR_DEPTH,
	ERR_FLC_CHUNK_TYPE,
	ERR_FLC_OVERFLOW,
	ERR_FLC_DIB_SIZE,

	ERR_SUBCLASS_DLG_CREATE,

	ERR_OUT_OF_MEMORY,

	ERR_RES_CREATE_WND,
	ERR_RES_NO_ACCEL,

	ERR_NO_THREAD_LIB,

	ERR_ADPCM,

	ERR_APP_START
};


_inline void ThrowError (int iErr)
{

	throw (iErr);
}


#endif
