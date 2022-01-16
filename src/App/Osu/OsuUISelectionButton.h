//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		song browser selection button (mode, mods, random, etc)
//
// $NoKeywords: $
//===============================================================================//

#ifndef OSUUISELECTIONBUTTON_H
#define OSUUISELECTIONBUTTON_H

#include "CBaseUIButton.h"

class OsuSkinImage;

class OsuUISelectionButton : public CBaseUIButton
{
public:
	OsuUISelectionButton(std::function<OsuSkinImage*()> getImageFunc, std::function<OsuSkinImage*()> getImageOverFunc, float xPos, float yPos, float xSize, float ySize, UString name);

	void draw(Graphics *g);

	virtual void onMouseInside();
	virtual void onMouseOutside();

	virtual void onResized();

	void keyboardPulse();

private:
	float m_fAnimation;

	std::function<OsuSkinImage*()> getImageFunc;
	std::function<OsuSkinImage*()> getImageOverFunc;
};

#endif
