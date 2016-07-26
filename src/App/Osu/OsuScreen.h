//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		baseclass for any drawable screen state object of the game
//
// $NoKeywords: $
//===============================================================================//

#ifndef OSUSCREEN_H
#define OSUSCREEN_H

#include "cbase.h"
#include "KeyboardListener.h"

class OsuScreen : public KeyboardListener
{
public:
	OsuScreen();
	virtual ~OsuScreen() {;}

	virtual void draw(Graphics *g) {;}
	virtual void update() {;}

	virtual void onKeyDown(KeyboardEvent &e) {;}
	virtual void onKeyUp(KeyboardEvent &e) {;}
	virtual void onChar(KeyboardEvent &e) {;}

	virtual void onResolutionChange(Vector2 newResolution) {;}

	virtual void setVisible(bool visible) {m_bVisible = visible;}

	inline bool isVisible() const {return m_bVisible;}

protected:
	bool m_bVisible;
};

#endif
