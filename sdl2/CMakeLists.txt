# SPDX-License-Identifier: MIT
#
# Copyright (c) 2020-2022 Antonio Niño Díaz

ugba_toolchain_sdl2()

include(compiler_flags.cmake)

# Define library target
# ---------------------

add_executable(ucity-advance)

compiler_flags_sdl2(ucity-advance)
linker_flags_sdl2(ucity-advance)

target_link_libraries(ucity-advance libugba)
target_link_libraries(ucity-advance umod_player)

# Source code, include directories and global definitions
# -------------------------------------------------------

target_sources(ucity-advance PRIVATE ${ALL_FILES_SOURCE})
target_include_directories(ucity-advance PRIVATE ${INCLUDE_PATHS})

# Deterministic simulation baseline runner. It reuses the game and simulation
# sources but initializes the SDL2 backend without opening a window or loading
# the interactive executable's SRAM save file.
set(SIM_BASELINE_SOURCES ${ALL_FILES_SOURCE})
list(REMOVE_ITEM SIM_BASELINE_SOURCES "${CMAKE_SOURCE_DIR}/source/main.c")
add_executable(ucity-sim-baseline
    ${SIM_BASELINE_SOURCES}
    ${CMAKE_SOURCE_DIR}/tools/sim_baseline/main.c
    ${CMAKE_SOURCE_DIR}/tools/sim_baseline/ucity_main_wrap.c
)
compiler_flags_sdl2(ucity-sim-baseline)
linker_flags_sdl2(ucity-sim-baseline)
target_link_libraries(ucity-sim-baseline libugba)
target_link_libraries(ucity-sim-baseline umod_player)
target_include_directories(ucity-sim-baseline PRIVATE ${INCLUDE_PATHS})

install(
    TARGETS
        ucity-advance libugba
    DESTINATION
        .
)
