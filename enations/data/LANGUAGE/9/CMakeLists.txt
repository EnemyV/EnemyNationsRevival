set(voice11_wav
        11/bld02.wav
        11/bld03.wav
        11/bld04.wav
        11/bld05.wav
        11/bld06.wav
        11/com01.wav
        11/com02.wav
        11/com04.wav
        11/com05.wav
        11/com06.wav
        11/com09.wav
        11/com10.wav
        11/com11.wav
        11/com12.wav
        11/com13.wav
        11/com14.wav
        11/com15.wav
        11/com16.wav
        11/com17.wav
        11/com18.wav
        11/com19.wav
        11/com20.wav
        11/cut01.wav
        11/cut02.wav
        11/cut03.wav
        11/cut04.wav
        11/cut05.wav
        11/cut06.wav
        11/cut07.wav
        11/cut08.wav
        11/cut09.wav
        11/cut10.wav
        11/cut11.wav
        11/cut12.wav
        11/cut13.wav
        11/fac01.wav
        11/fac03.wav
        11/fac04.wav
        11/fac05.wav
        11/fac06.wav
        11/rec01.wav
        11/rec02.wav
        11/rec03.wav
        11/rec04.wav
        11/sci01.wav
        11/sci02.wav
        11/sci03.wav
        11/sci04.wav
        11/SYS01.wav
        11/SYS02.wav
        11/SYS03.wav
        11/SYS04.wav
        11/SYS05.wav
        11/SYS06.wav
        11/SYS07.wav
        11/SYS08.wav
        11/SYS09.wav
        11/SYS10.wav
        11/SYS11.wav
        11/SYS12.wav
        11/SYS13.wav
        11/SYS14.wav
        11/tem01.wav
        11/tem02.wav
        11/tem03.wav
        11/tem04.wav
        11/tem05.wav
        11/tem06.wav
        11/tem07.wav
        11/tem08.wav
        11/tem09.wav
        11/tem10.wav
        11/tem11.wav
        11/tem12.wav
        11/tem13.wav
        11/tem14.wav
        11/tem15.wav
        11/tem16.wav
        11/tem17.wav
        11/tem18.wav
        )

set(voice22_wav
        22/bld02.wav
        22/bld03.wav
        22/bld04.wav
        22/bld05.wav
        22/bld06.wav
        22/com01.wav
        22/com02.wav
        22/com04.wav
        22/com05.wav
        22/com06.wav
        22/com09.wav
        22/com10.wav
        22/com11.wav
        22/com12.wav
        22/com13.wav
        22/com14.wav
        22/com15.wav
        22/com16.wav
        22/com17.wav
        22/com18.wav
        22/com19.wav
        22/com20.wav
        22/cut01.wav
        22/cut02.wav
        22/cut03.wav
        22/cut04.wav
        22/cut05.wav
        22/cut06.wav
        22/cut07.wav
        22/cut08.wav
        22/cut09.wav
        22/cut10.wav
        22/cut11.wav
        22/cut12.wav
        22/cut13.wav
        22/fac01.wav
        22/fac03.wav
        22/fac04.wav
        22/fac05.wav
        22/fac06.wav
        22/rec01.wav
        22/rec02.wav
        22/rec03.wav
        22/rec04.wav
        22/sci01.wav
        22/sci02.wav
        22/sci03.wav
        22/sci04.wav
        22/sys01.wav
        22/sys02.wav
        22/sys03.wav
        22/sys04.wav
        22/sys05.wav
        22/sys07.wav
        22/sys08.wav
        22/sys14.wav
        22/tem01.wav
        22/tem02.wav
        22/tem03.wav
        22/tem04.wav
        22/tem05.wav
        22/tem06.wav
        22/tem07.wav
        22/tem08.wav
        22/tem09.wav
        22/tem10.wav
        22/tem11.wav
        22/tem12.wav
        22/tem13.wav
        22/tem14.wav
        22/tem15.wav
        22/tem16.wav
        22/tem17.wav
        22/tem18.wav
        )

# replace .wav with .raw
string(REGEX REPLACE "[.]wav" ".raw" voice11_raw ${voice11_wav})
string(REGEX REPLACE "[.]wav" ".raw" voice22_raw ${voice22_wav})

# replace .raw with .cmp
string(REGEX REPLACE "[.]raw" ".cmp" voice11_cmp ${voice11_raw})
string(REGEX REPLACE "[.]raw" ".cmp" voice22_cmp ${voice22_raw})

foreach (wav raw IN ZIP_LISTS voice11_wav voice11_raw)
    add_custom_command(OUTPUT ${raw}
            COMMAND vtor ARGS ${wav} ${raw})
endforeach ()

foreach (wav raw IN ZIP_LISTS voice22_wav voice22_raw)
    add_custom_command(OUTPUT ${raw}
            COMMAND vtor ARGS ${wav} ${raw})
endforeach ()

foreach (raw cmp IN ZIP_LISTS voice11_raw voice11_cmp)
    add_custom_command(OUTPUT ${cmp}
            COMMAND compit ARGS 9 ${raw} ${cmp})
endforeach ()

foreach (raw cmp IN ZIP_LISTS voice22_raw voice22_cmp)
    add_custom_command(OUTPUT ${cmp}
            COMMAND compit ARGS 9 ${raw} ${cmp})
endforeach ()

add_custom_command(OUTPUT 9.rif
        COMMAND makeriff ARGS 9.mif)
