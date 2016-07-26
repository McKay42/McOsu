//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		song browser selection button (mode, mods, random, etc)
//
// $NoKeywords: $
//===============================================================================//

#ifndef OSUUISELECTIONBUTTON_H
#define OSUUISELECTIONBUTTON_H

#include "CBaseUIImageButton.h"

class OsuUISelectionButton : public CBaseUIImageButton
{
public:
	OsuUISelectionButton(UString imageResourceName, float xPos, float yPos, float xSize, float ySize, UString name);

	void draw(Graphics *g);

	virtual void onMouseInside();
	virtual void onMouseOutside();

	void keyboardPulse();

	void setImageResourceNameOver(UString imageResourceName) {m_sImageOver = imageResourceName;}
private:
	float m_fAnimation;
	UString m_sImageOver;
};

#endif
