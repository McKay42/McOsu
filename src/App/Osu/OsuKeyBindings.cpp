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
