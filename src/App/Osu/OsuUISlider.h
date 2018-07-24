//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		generic slider (mod overrides, options, etc.)
//
// $NoKeywords: $osusl
//===============================================================================//

#ifndef OSUUISLIDER_H
#define OSUUISLIDER_H

#include "CBaseUISlider.h"

class Osu;

class OsuUISlider : public CBaseUISlider
{
public:
	OsuUISlider(Osu *osu, float xPos, float yPos, float xSize, float ySize, UString name);

	virtual void draw(Graphics *g);

private:
	Osu *m_osu;
};

#endif
