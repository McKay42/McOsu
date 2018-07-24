//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		pause menu button
//
// $NoKeywords: $
//===============================================================================//

#ifndef OSUUIPAUSEMENUBUTTON_H
#define OSUUIPAUSEMENUBUTTON_H

#include "CBaseUIImageButton.h"

class Osu;

class OsuUIPauseMenuButton : public CBaseUIImageButton
{
public:
	OsuUIPauseMenuButton(Osu *osu, UString imageResourceName, float xPos, float yPos, float xSize, float ySize, UString name);

	void onMouseInside();
	void onMouseOutside();

	void setBaseScale(float xScale, float yScale);

private:
	Osu *m_osu;

	Vector2 m_vBaseScale;
	float m_fScaleMultiplier;
};

#endif
