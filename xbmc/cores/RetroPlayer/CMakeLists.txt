set(SOURCES PixelConverter.cpp
            RetroPlayer.cpp
            RetroPlayerAudio.cpp
            RetroPlayerAutoSave.cpp
            RetroPlayerClock.cpp
            RetroPlayerVideo.cpp)

set(HEADERS IPixelConverter.h
            PixelConverter.h
            RetroPlayer.h
            RetroPlayerAudio.h
            RetroPlayerAutoSave.h
            RetroPlayerClock.h
            RetroPlayerDefines.h
            RetroPlayerVideo.h)

if(CORE_SYSTEM_NAME STREQUAL rbpi)
  list(APPEND SOURCES PixelConverterRBP.cpp)
  list(APPEND HEADERS PixelConverterRBP.h)
endif()

core_add_library(retroplayer)
