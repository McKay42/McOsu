//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		generic button (context menu items, mod selection screen, etc.)
//
// $NoKeywords: $osubt
//===============================================================================//

#ifndef OSUBUTTON_H
#define OSUBUTTON_H

#include "CBaseUIButton.h"

class Osu;

class OsuUIButton : public CBaseUIButton
{
public:
	OsuUIButton(Osu *osu, float xPos, float yPos, float xSize, float ySize, UString name, UString text);

	virtual void draw(Graphics *g);

	void setColor(Color color) {m_color = color; m_backupColor = color;}
	void setUseDefaultSkin() {m_bDefaultSkin = true;}

	virtual void onMouseInside();
	virtual void onMouseOutside();

	void animateClickColor();

private:
	virtual void onClicked();

	Osu *m_osu;

	bool m_bDefaultSkin;
	Color m_color;
	Color m_backupColor;
	float m_fBrightness;
	float m_fAnim;
};

#endif
