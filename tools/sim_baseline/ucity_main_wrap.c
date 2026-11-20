// SPDX-License-Identifier: GPL-3.0-only
//
// Reuse the game's platform helpers without defining the interactive entrypoint
// for the standalone simulation baseline executable.

#define main UCity_Interactive_Main
#include "../../source/main.c"
#undef main
