# SPDX-License-Identifier: MIT
#
# Copyright (c) 2020-2022 Antonio Niño Díaz

ugba_toolchain_sdl2()

include(compiler_flags.cmake)

# Define library target
# ---------------------

add_executable(ucity-advance)

# The legacy game and simulation baseline do not compile the OpenGL renderer.
set(GAME_FILES_SOURCE ${ALL_FILES_SOURCE})
list(REMOVE_ITEM GAME_FILES_SOURCE
    "${CMAKE_SOURCE_DIR}/source/renderer3d/city_renderer.c")

compiler_flags_sdl2(ucity-advance)
linker_flags_sdl2(ucity-advance)

target_link_libraries(ucity-advance libugba)
target_link_libraries(ucity-advance umod_player)

# Source code, include directories and global definitions
# -------------------------------------------------------

target_sources(ucity-advance PRIVATE ${GAME_FILES_SOURCE})
target_include_directories(ucity-advance PRIVATE ${INCLUDE_PATHS})

# Deterministic simulation baseline runner. It reuses the game and simulation
# sources but initializes the SDL2 backend without opening a window or loading
# the interactive executable's SRAM save file.
set(SIM_BASELINE_SOURCES ${GAME_FILES_SOURCE})
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

# Standalone OpenGL 2.1 prototype that reads the platform-neutral city view.
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
set(PROTOTYPE_SOURCES ${GAME_FILES_SOURCE})
list(REMOVE_ITEM PROTOTYPE_SOURCES "${CMAKE_SOURCE_DIR}/source/main.c")
add_executable(ucity-3d-prototype
    ${PROTOTYPE_SOURCES}
    ${CMAKE_SOURCE_DIR}/source/renderer3d/city_renderer.c
    ${CMAKE_SOURCE_DIR}/tools/city3d_prototype_main.c
    ${CMAKE_SOURCE_DIR}/tools/sim_baseline/ucity_main_wrap.c
)
compiler_flags_sdl2(ucity-3d-prototype)
linker_flags_sdl2(ucity-3d-prototype)
target_link_libraries(ucity-3d-prototype libugba umod_player)
target_include_directories(ucity-3d-prototype PRIVATE ${INCLUDE_PATHS})
if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    target_link_libraries(ucity-3d-prototype SDL2::SDL2 SDL2::SDL2main)
else()
    target_include_directories(ucity-3d-prototype PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(ucity-3d-prototype ${SDL2_LIBRARIES})
endif()
target_include_directories(ucity-3d-prototype PRIVATE ${OPENGL_INCLUDE_DIRS})
target_link_libraries(ucity-3d-prototype ${OPENGL_LIBRARIES})
if(UNIX)
    target_link_libraries(ucity-3d-prototype m)
endif()

install(
    TARGETS
        ucity-advance libugba
    DESTINATION
        .
)
