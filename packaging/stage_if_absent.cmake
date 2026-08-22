# Copy SRC -> DST only when DST does not already exist. Used for runtime config
# the game rewrites (vdmplay.ini): copy_if_different would reset the player's
# saved settings on every rebuild.
if (NOT EXISTS "${DST}")
    configure_file("${SRC}" "${DST}" COPYONLY)
endif ()
