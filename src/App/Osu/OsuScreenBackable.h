//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		screen + back button
//
// $NoKeywords: $
//===============================================================================//

#ifndef OSUSCREENBACKABLE_H
#define OSUSCREENBACKABLE_H

#include "OsuScreen.h"
#include "OsuUIBackButton.h"

class Osu;

class OsuScreenBackable : public OsuScreen
{
public:
	OsuScreenBackable(Osu *osu);
	virtual ~OsuScreenBackable();

	virtual void draw(Graphics *g);
	virtual void update();

	virtual void onKeyDown(KeyboardEvent &e);
	virtual void onKeyUp(KeyboardEvent &e);
	virtual void onChar(KeyboardEvent &e);

	virtual void onResolutionChange(Vector2 newResolution);

	virtual void stealFocus();

protected:
	virtual void onBack() = 0;

	virtual void updateLayout();

	OsuUIBackButton *m_backButton;
};

#endif
