//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		song browser selection button (mode, mods, random, etc)
//
// $NoKeywords: $
//===============================================================================//

#ifndef OSUUISELECTIONBUTTON_H
#define OSUUISELECTIONBUTTON_H

#include "CBaseUIButton.h"

class OsuUISelectionButton : public CBaseUIButton
{
public:
	OsuUISelectionButton(std::function<Image*()> getImageFunc, std::function<Image*()> getImageOverFunc, float xPos, float yPos, float xSize, float ySize, UString name);

	void draw(Graphics *g);

	virtual void onMouseInside();
	virtual void onMouseOutside();

	virtual void onResized();

	void keyboardPulse();

private:
	float m_fAnimation;

	Vector2 m_vScale;
	bool m_bScaleToFit;
	bool m_bKeepAspectRatio;

	std::function<Image*()> getImageFunc;
	std::function<Image*()> getImageOverFunc;
};

#endif
