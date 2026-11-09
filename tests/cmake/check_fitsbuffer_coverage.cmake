# check_fitsbuffer_coverage.cmake - fail if any network message type lacks an
# explicit packet-size rule in CNetCmd::FitsBuffer, or if the switch default
# accepts unknown types. Adding a message type without a validation rule must
# fail this check (plan 2.1). Run via: cmake -DEN_SOURCE_DIR=<repo> -P <this file>
if (NOT DEFINED EN_SOURCE_DIR)
    message(FATAL_ERROR "EN_SOURCE_DIR not set")
endif ()

file(READ "${EN_SOURCE_DIR}/enations_latest/src/netcmd.h" _h)
file(READ "${EN_SOURCE_DIR}/enations_latest/src/netcmd.cpp" _cpp)

# Enum values between cmd_ready and last_message (inclusive).
string(FIND "${_h}" "cmd_ready," _begin)
string(FIND "${_h}" "last_message" _end)
if (_begin EQUAL -1 OR _end EQUAL -1 OR NOT _end GREATER _begin)
    message(FATAL_ERROR "could not locate the message enum in netcmd.h")
endif ()
math(EXPR _len "${_end} - ${_begin} + 12")
string(SUBSTRING "${_h}" ${_begin} ${_len} _enum)
# Strip // comments, then split on commas.
string(REGEX REPLACE "//[^\n]*" "" _enum "${_enum}")
string(REPLACE "," ";" _enum "${_enum}")
set(_values "")
foreach (_tok IN LISTS _enum)
    string(STRIP "${_tok}" _tok)
    if (_tok MATCHES "^[A-Za-z_][A-Za-z0-9_]*$")
        list(APPEND _values "${_tok}")
    endif ()
endforeach ()

# FitsBuffer cases.
string(FIND "${_cpp}" "BOOL CNetCmd::FitsBuffer" _fb)
if (_fb EQUAL -1)
    message(FATAL_ERROR "could not locate CNetCmd::FitsBuffer in netcmd.cpp")
endif ()
string(SUBSTRING "${_cpp}" ${_fb} -1 _fb)
string(REGEX MATCHALL "case [A-Za-z_][A-Za-z0-9_]*[ \t]*:" _cases "${_fb}")
set(_covered "")
foreach (_c IN LISTS _cases)
    string(REGEX REPLACE "^case ([A-Za-z_][A-Za-z0-9_]*)[^:]*:$" "\\1" _c "${_c}")
    list(APPEND _covered "${_c}")
endforeach ()

# Intentionally denied: never sent and no handler/struct exists (dead enum slot).
set(_allow_denied cmd_game_data last_message)

set(_missing "")
foreach (_v IN LISTS _values)
    if (NOT _v IN_LIST _covered AND NOT _v IN_LIST _allow_denied)
        list(APPEND _missing "${_v}")
    endif ()
endforeach ()
if (_missing)
    message(FATAL_ERROR "FitsBuffer has no size rule for message type(s): ${_missing}. Add an explicit case (or document the type as dead in tests/cmake/check_fitsbuffer_coverage.cmake).")
endif ()

# The default arm must deny, not accept at header size. Strip // comments
# first so prose mentioning the old default cannot false-positive.
string(REGEX REPLACE "//[^\n]*" "" _fb_nocomment "${_fb}")
if (_fb_nocomment MATCHES "default:[^}]*cbNeed = sizeof\\( CNetCmd \\)")
    message(FATAL_ERROR "FitsBuffer default still accepts unknown message types at sizeof(CNetCmd); it must return FALSE.")
endif ()
if (NOT _fb_nocomment MATCHES "default:[ \t\r\n]*return FALSE")
    message(FATAL_ERROR "FitsBuffer default does not return FALSE for unknown message types.")
endif ()

list(LENGTH _values _n)
message(STATUS "FitsBuffer coverage OK: ${_n} message types, all sized or explicitly denied; default denies.")
