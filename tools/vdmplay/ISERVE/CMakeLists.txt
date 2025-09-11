
###############################
# iserve.exe
###############################

set(iserve_sources
        STDAFX.CPP
        ISERVE.CPP
        ISERVDLG.CPP
        ISERVE.RC
        )

add_executable(iserve WIN32 ${iserve_sources})

target_include_directories(iserve PUBLIC
        ../
        ../Release
        )
