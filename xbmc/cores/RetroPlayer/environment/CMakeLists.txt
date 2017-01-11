set(SOURCES Action.cpp
            Environment.cpp
            Goal.cpp
            Reward.cpp
            State.cpp)

set(HEADERS Action.h
            Environment.h
            Goal.h
            Reward.h
            State.h)

core_add_library(retroplayer_environment)
