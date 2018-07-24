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

ConVar OsuKeyBindings::INCREASE_SPEED("osu_key_mania_increase_speed", (int)KEY_RIGHT);
ConVar OsuKeyBindings::DECREASE_SPEED("osu_key_mania_decrease_speed", (int)KEY_LEFT);

ConVar OsuKeyBindings::INCREASE_VOLUME("osu_key_increase_volume", (int)KEY_UP);
ConVar OsuKeyBindings::DECREASE_VOLUME("osu_key_decrease_volume", (int)KEY_DOWN);

ConVar OsuKeyBindings::INCREASE_LOCAL_OFFSET("osu_key_increase_local_offset", (int)KEY_ADD);
ConVar OsuKeyBindings::DECREASE_LOCAL_OFFSET("osu_key_decrease_local_offset", (int)KEY_SUBTRACT);

ConVar OsuKeyBindings::GAME_PAUSE("osu_key_game_pause", (int)KEY_ESCAPE);
ConVar OsuKeyBindings::SKIP_CUTSCENE("osu_key_skip_cutscene", (int)KEY_SPACE);
ConVar OsuKeyBindings::TOGGLE_SCOREBOARD("osu_key_toggle_scoreboard", (int)KEY_TAB);
ConVar OsuKeyBindings::SEEK_TIME("osu_key_seek_time", (int)KEY_SHIFT);
ConVar OsuKeyBindings::QUICK_RETRY("osu_key_quick_retry", (int)KEY_BACKSPACE);
ConVar OsuKeyBindings::QUICK_SAVE("osu_key_quick_save", (int)KEY_F6);
ConVar OsuKeyBindings::QUICK_LOAD("osu_key_quick_load", (int)KEY_F7);
ConVar OsuKeyBindings::SAVE_SCREENSHOT("osu_key_save_screenshot", (int)KEY_F12);
ConVar OsuKeyBindings::DISABLE_MOUSE_BUTTONS("osu_key_disable_mouse_buttons", (int)KEY_F10);
ConVar OsuKeyBindings::BOSS_KEY("osu_key_boss", (int)KEY_INSERT);

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

std::vector<std::vector<ConVar*>> OsuKeyBindings::MANIA = OsuKeyBindings::createManiaConVarSets();

std::vector<ConVar*> OsuKeyBindings::createManiaConVarSet(int k)
{
	std::vector<ConVar*> convars;
	for (int i=1; i<=k; i++)
	{
		convars.push_back(new ConVar(UString::format("osu_key_mania_%ik_%i", k, i), 0));
	}
	return convars;
}

std::vector<std::vector<ConVar*>> OsuKeyBindings::createManiaConVarSets()
{
	std::vector<std::vector<ConVar*>> sets;
	for (int i=1; i<=10; i++)
	{
		sets.push_back(createManiaConVarSet(i));
	}
	setDefaultManiaKeys(sets);
	return sets;
}

void OsuKeyBindings::setDefaultManiaKeys(std::vector<std::vector<ConVar*>> mania)
{
	mania[0][0]->setValue((int)KEY_F);

	mania[1][0]->setValue((int)KEY_F);
	mania[1][1]->setValue((int)KEY_J);

	mania[2][0]->setValue((int)KEY_F);
	mania[2][1]->setValue((int)KEY_SPACE);
	mania[2][2]->setValue((int)KEY_J);

	mania[3][0]->setValue((int)KEY_D);
	mania[3][1]->setValue((int)KEY_F);
	mania[3][2]->setValue((int)KEY_J);
	mania[3][3]->setValue((int)KEY_K);

	mania[4][0]->setValue((int)KEY_D);
	mania[4][1]->setValue((int)KEY_F);
	mania[4][2]->setValue((int)KEY_SPACE);
	mania[4][3]->setValue((int)KEY_J);
	mania[4][4]->setValue((int)KEY_K);

	mania[5][0]->setValue((int)KEY_S);
	mania[5][1]->setValue((int)KEY_D);
	mania[5][2]->setValue((int)KEY_F);
	mania[5][3]->setValue((int)KEY_J);
	mania[5][4]->setValue((int)KEY_K);
	mania[5][5]->setValue((int)KEY_L);

	mania[6][0]->setValue((int)KEY_S);
	mania[6][1]->setValue((int)KEY_D);
	mania[6][2]->setValue((int)KEY_F);
	mania[6][3]->setValue((int)KEY_SPACE);
	mania[6][4]->setValue((int)KEY_J);
	mania[6][5]->setValue((int)KEY_K);
	mania[6][6]->setValue((int)KEY_L);

	mania[7][0]->setValue((int)KEY_A);
	mania[7][1]->setValue((int)KEY_S);
	mania[7][2]->setValue((int)KEY_D);
	mania[7][3]->setValue((int)KEY_F);
	mania[7][4]->setValue((int)KEY_J);
	mania[7][5]->setValue((int)KEY_K);
	mania[7][6]->setValue((int)KEY_L);
	mania[7][7]->setValue((int)KEY_N); // TODO

	mania[8][0]->setValue((int)KEY_A);
	mania[8][1]->setValue((int)KEY_S);
	mania[8][2]->setValue((int)KEY_D);
	mania[8][3]->setValue((int)KEY_F);
	mania[8][4]->setValue((int)KEY_SPACE);
	mania[8][5]->setValue((int)KEY_J);
	mania[8][6]->setValue((int)KEY_K);
	mania[8][7]->setValue((int)KEY_L);
	mania[8][8]->setValue((int)KEY_N); // TODO

	mania[9][0]->setValue((int)KEY_A);
	mania[9][1]->setValue((int)KEY_S);
	mania[9][2]->setValue((int)KEY_D);
	mania[9][3]->setValue((int)KEY_F);
	mania[9][4]->setValue((int)KEY_SPACE);
	mania[9][5]->setValue((int)KEY_N); // TODO
	mania[9][6]->setValue((int)KEY_O);
	mania[9][7]->setValue((int)KEY_P);
	mania[9][8]->setValue((int)KEY_N); // TODO
	mania[9][9]->setValue((int)KEY_N); // TODO
}
