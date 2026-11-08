#ifndef ___ERROR_H__
#define ___ERROR_H__


//---------------------------------------------------------------------------
//
// Copyright (c) 1995, 1996. Windward Studios, Inc.
// All Rights Reserved.
//
//---------------------------------------------------------------------------


enum Error {
    ERR_BASE_USER_ERROR = (1 << 29),
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

static constexpr const char* GetWind22ErrString(int iErr) {
    if ( ( iErr < ERR_APP_START ) && ( iErr > ERR_BASE_USER_ERROR ) ) {
        switch(iErr){
            case ERR_MMIO_OPEN:
                return "ERR_MMIO_OPEN";
            case ERR_MMIO_DESCEND_RIFF:
                return "ERR_MMIO_DESCEND_RIFF";
            case ERR_MMIO_DESCEND_LIST:
                return "ERR_MMIO_DESCEND_LIST";
            case ERR_MMIO_DESCEND_CHUNK:
                return "ERR_MMIO_DESCEND_CHUNK";
            case ERR_MMIO_ASCEND_LIST:
                return "ERR_MMIO_ASCEND_LIST";
            case ERR_MMIO_ASCEND_CHUNK:
                return "ERR_MMIO_ASCEND_CHUNK";
            case ERR_MMIO_READ:
                return "ERR_MMIO_READ";
            case ERR_MMIO_REWIND:
                return "ERR_MMIO_REWIND";
            case ERR_DATAFILE_OPEN:
                return "ERR_DATAFILE_OPEN";
            case ERR_DATAFILE_BAD_MAGIC:
                return "ERR_DATAFILE_BAD_MAGIC";
            case ERR_DATAFILE_NO_ENTRY:
                return "ERR_DATAFILE_NO_ENTRY";
            case ERR_DATAFILE_READ:
                return "ERR_DATAFILE_READ";
            case ERR_CACHE_READ:
                return "ERR_CACHE_READ";
            case ERR_PATCHFILE_OPEN:
                return "ERR_PATCHFILE_OPEN";
            case ERR_FLC_READ_FRAME_HDR:
                return "ERR_FLC_READ_FRAME_HDR";
            case ERR_FLC_FRAME_HDR_SIZE:
                return "ERR_FLC_FRAME_HDR_SIZE";
            case ERR_FLC_READ_FRAME:
                return "ERR_FLC_READ_FRAME";
            case ERR_FLC_READ_HDR:
                return "ERR_FLC_READ_HDR";
            case ERR_FLC_TYPE:
                return "ERR_FLC_TYPE";
            case ERR_FLC_COLOR_DEPTH:
                return "ERR_FLC_COLOR_DEPTH";
            case ERR_FLC_CHUNK_TYPE:
                return "ERR_FLC_CHUNK_TYPE";
            case ERR_FLC_OVERFLOW:
                return "ERR_FLC_OVERFLOW";
            case ERR_FLC_DIB_SIZE:
                return "ERR_FLC_DIB_SIZE";
            case ERR_SUBCLASS_DLG_CREATE:
                return "ERR_SUBCLASS_DLG_CREATE";
            case ERR_OUT_OF_MEMORY:
                return "ERR_OUT_OF_MEMORY";
            case ERR_RES_CREATE_WND:
                return "ERR_RES_CREATE_WND";
            case ERR_RES_NO_ACCEL:
                return "ERR_RES_NO_ACCEL";
            case ERR_NO_THREAD_LIB:
                return "ERR_NO_THREAD_LIB";
            case ERR_ADPCM:
                return "ERR_ADPCM";
            default:
                return "";
        }
    } else{
        return "";
    }
}

_inline void ThrowError(int iErr) {

    const char* errStr = GetWind22ErrString( iErr );
    char buf[256];
    if ( errStr == NULL || errStr[0] == '\0' ) {
        _snprintf_s( buf, sizeof(buf), _TRUNCATE,
            "Application error thrown, error number: %d\n", iErr );
    } else {
        _snprintf_s( buf, sizeof(buf), _TRUNCATE,
            "Wind22 error thrown, error: %s, number: %d\n", errStr, iErr );
    }
    OutputDebugString( buf );

    throw ( iErr );
}


#endif
