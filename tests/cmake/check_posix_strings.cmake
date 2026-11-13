# check_posix_strings.cmake - fail if any STRINGTABLE id in lastplnt.rc has no
# entry in the POSIX string table (win32_resources_linux.cpp). A missing entry
# degrades silently to a blank string at runtime (plan 1.5).
# Run via: cmake -DEN_SOURCE_DIR=<repo> -P <this file>
cmake_policy(SET CMP0057 NEW)
if (NOT DEFINED EN_SOURCE_DIR)
    message(FATAL_ERROR "EN_SOURCE_DIR not set")
endif ()

file(READ "${EN_SOURCE_DIR}/enations_latest/src/lastplnt.rc" _rc)
file(READ "${EN_SOURCE_DIR}/enations_latest/src/win32_resources_linux.cpp" _posix)

# STRINGTABLE entries start a line (optionally indented): IDS_xxx "text" or
# IDS_xxx alone with the text on the next line.
string(REGEX MATCHALL "\n[ \t]*IDS_[A-Za-z0-9_]+" _rc_ids "${_rc}")
set(_want "")
foreach (_i IN LISTS _rc_ids)
    string(STRIP "${_i}" _i)
    list(APPEND _want "${_i}")
endforeach ()
list(REMOVE_DUPLICATES _want)

# Map entries look like: { IDS_xxx, "text" },
string(REGEX MATCHALL "\\{[ \t]*IDS_[A-Za-z0-9_]+[ \t]*," _entries "${_posix}")
set(_have "")
foreach (_e IN LISTS _entries)
    string(REGEX REPLACE "^\\{[ \t]*(IDS_[A-Za-z0-9_]+)[ \t]*,$" "\\1" _e "${_e}")
    list(APPEND _have "${_e}")
endforeach ()
list(REMOVE_DUPLICATES _have)

set(_missing "")
foreach (_w IN LISTS _want)
    if (NOT _w IN_LIST _have)
        list(APPEND _missing "${_w}")
    endif ()
endforeach ()
if (_missing)
    message(FATAL_ERROR "POSIX string table is missing STRINGTABLE id(s): ${_missing}. Copy the text verbatim from lastplnt.rc into win32_resources_linux.cpp.")
endif ()

list(LENGTH _want _n)
message(STATUS "POSIX strings OK: all ${_n} STRINGTABLE ids present.")
