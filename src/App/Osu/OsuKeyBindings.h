//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		key bindings container
//
// $NoKeywords: $osukey
//===============================================================================//

#ifndef OSUKEYBINDINGS_H
#define OSUKEYBINDINGS_H

#include "ConVar.h"

class OsuKeyBindings
{
public:
	static ConVar LEFT_CLICK;
	static ConVar RIGHT_CLICK;

	static ConVar INCREASE_VOLUME;
	static ConVar DECREASE_VOLUME;

	static ConVar INCREASE_LOCAL_OFFSET;
	static ConVar DECREASE_LOCAL_OFFSET;

	static ConVar GAME_PAUSE;
	static ConVar SKIP_CUTSCENE;
	static ConVar QUICK_RETRY;
	static ConVar SAVE_SCREENSHOT;
	static ConVar DISABLE_MOUSE_BUTTONS;
};

#endif
