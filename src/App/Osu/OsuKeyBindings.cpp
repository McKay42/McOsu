//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		key bindings container
//
// $NoKeywords: $osukey
//===============================================================================//

#include "OsuKeyBindings.h"

#include "Keyboard.h"

ConVar OsuKeyBindings::LEFT_CLICK("osu_key_left_click", (int)KEY_R);
ConVar OsuKeyBindings::RIGHT_CLICK("osu_key_right_click", (int)KEY_T);

ConVar OsuKeyBindings::INCREASE_VOLUME("osu_key_increase_volume", (int)KEY_UP);
ConVar OsuKeyBindings::DECREASE_VOLUME("osu_key_decrease_volume", (int)KEY_DOWN);

ConVar OsuKeyBindings::INCREASE_LOCAL_OFFSET("osu_key_increase_local_offset", (int)KEY_ADD);
ConVar OsuKeyBindings::DECREASE_LOCAL_OFFSET("osu_key_decrease_local_offset", (int)KEY_SUBTRACT);

ConVar OsuKeyBindings::GAME_PAUSE("osu_key_game_pause", (int)KEY_ESCAPE);
ConVar OsuKeyBindings::SKIP_CUTSCENE("osu_key_skip_cutscene", (int)KEY_SPACE);
ConVar OsuKeyBindings::QUICK_RETRY("osu_key_quick_retry", (int)KEY_BACKSPACE);
ConVar OsuKeyBindings::SAVE_SCREENSHOT("osu_key_save_screenshot", (int)KEY_F12);
ConVar OsuKeyBindings::DISABLE_MOUSE_BUTTONS("osu_key_disable_mouse_buttons", (int)KEY_F10);

ConVar OsuKeyBindings::MOD_EASY("osu_key_mod_easy", (int)KEY_Q);
ConVar OsuKeyBindings::MOD_NOFAIL("osu_key_mod_nofail", (int)KEY_W);
ConVar OsuKeyBindings::MOD_HALFTIME("osu_key_mod_halftime", (int)KEY_E);
ConVar OsuKeyBindings::MOD_HARDROCK("osu_key_mod_hardrock", (int)KEY_A);
ConVar OsuKeyBindings::MOD_SUDDENDEATH("osu_key_mod_suddendeath", (int)KEY_S);
ConVar OsuKeyBindings::MOD_DOUBLETIME("osu_key_mod_doubletime", (int)KEY_D);
ConVar OsuKeyBindings::MOD_HIDDEN("osu_key_mod_hidden", (int)KEY_F);
ConVar OsuKeyBindings::MOD_FLASHLIGHT("osu_key_mod_flashlight", (int)KEY_G);
ConVar OsuKeyBindings::MOD_RELAX("osu_key_mod_relax", (int)KEY_Z);
ConVar OsuKeyBindings::MOD_AUTOPILOT("osu_key_mod_autopilot", (int)KEY_X);
ConVar OsuKeyBindings::MOD_SPUNOUT("osu_key_mod_spunout", (int)KEY_C);
ConVar OsuKeyBindings::MOD_AUTO("osu_key_mod_auto", (int)KEY_V);
