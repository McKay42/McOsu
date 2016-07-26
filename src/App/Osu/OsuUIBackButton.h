//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		big ass back button (options, songbrowser)
//
// $NoKeywords: $osubbt
//===============================================================================//

#ifndef OSUUIBACKBUTTON_H
#define OSUUIBACKBUTTON_H

#include "CBaseUIImageButton.h"

class OsuUIBackButton : public CBaseUIImageButton
{
public:
	OsuUIBackButton(UString imageResourceName, float xPos, float yPos, float xSize, float ySize, UString name);

	void draw(Graphics *g);

	virtual void onMouseInside();
	virtual void onMouseOutside();

private:
	float m_fAnimation;
};
#endif
