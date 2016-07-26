//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		key bindings container
//
// $NoKeywords: $osukey
//===============================================================================//

#include "OsuKeyBindings.h"

#include "Keyboard.h"

ConVar OsuKeyBindings::LEFT_CLICK("osu_key_left_click", (float)KEY_R);
ConVar OsuKeyBindings::RIGHT_CLICK("osu_key_right_click", (float)KEY_T);

ConVar OsuKeyBindings::INCREASE_VOLUME("osu_key_increase_volume", (float)KEY_UP);
ConVar OsuKeyBindings::DECREASE_VOLUME("osu_key_decrease_volume", (float)KEY_DOWN);

ConVar OsuKeyBindings::INCREASE_LOCAL_OFFSET("osu_key_increase_local_offset", (float)KEY_ADD);
ConVar OsuKeyBindings::DECREASE_LOCAL_OFFSET("osu_key_decrease_local_offset", (float)KEY_SUBTRACT);

ConVar OsuKeyBindings::GAME_PAUSE("osu_key_game_pause", (float)KEY_ESCAPE);
ConVar OsuKeyBindings::SKIP_CUTSCENE("osu_key_skip_cutscene", (float)KEY_SPACE);
ConVar OsuKeyBindings::QUICK_RETRY("osu_key_quick_retry", (float)KEY_BACKSPACE);
