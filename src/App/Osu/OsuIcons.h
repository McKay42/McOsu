//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		font awesome icon enum
//
// $NoKeywords: $
//===============================================================================//

#ifndef OSUICONS_H
#define OSUICONS_H

#include "cbase.h"

class OsuIcons
{
public:
	static std::vector<wchar_t> icons;

	static wchar_t addIcon(wchar_t character)
	{
		icons.push_back(character);
		return character;
	}

	static wchar_t Z_UNKNOWN_CHAR;
	static wchar_t Z_SPACE;

	static wchar_t GEAR;
	static wchar_t DESKTOP;
	static wchar_t CIRCLE;
	static wchar_t CUBE;
	static wchar_t VOLUME_UP;
	static wchar_t VOLUME_DOWN;
	static wchar_t VOLUME_OFF;
	static wchar_t PAINTBRUSH;
	static wchar_t GAMEPAD;
	static wchar_t WRENCH;
	static wchar_t EYE;
	static wchar_t ARROW_CIRCLE_UP;
	static wchar_t TROPHY;
	static wchar_t CARET_DOWN;
	static wchar_t ARROW_DOWN;
	static wchar_t GLOBE;
	static wchar_t USER;
	static wchar_t UNDO;
};

#endif
