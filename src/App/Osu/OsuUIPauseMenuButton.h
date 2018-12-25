//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		pause menu button
//
// $NoKeywords: $
//===============================================================================//

#ifndef OSUUIPAUSEMENUBUTTON_H
#define OSUUIPAUSEMENUBUTTON_H

#include "CBaseUIButton.h"

class Osu;

class Image;

class OsuUIPauseMenuButton : public CBaseUIButton
{
public:
	OsuUIPauseMenuButton(Osu *osu, std::function<Image*()> getImageFunc, float xPos, float yPos, float xSize, float ySize, UString name);

	virtual void draw(Graphics *g);

	virtual void onMouseInside();
	virtual void onMouseOutside();

	void setBaseScale(float xScale, float yScale);

	Image *getImage() {return getImageFunc != NULL ? getImageFunc() : NULL;}

private:
	Osu *m_osu;

	Vector2 m_vScale;
	Vector2 m_vBaseScale;
	float m_fScaleMultiplier;

	std::function<Image*()> getImageFunc;
};

#endif
