//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		tutorial screen for controls
//
// $NoKeywords: $osuvrtut
//===============================================================================//

#ifndef OSUVRTUTORIAL_H
#define OSUVRTUTORIAL_H

#include "OsuScreenBackable.h"

class CBaseUIContainer;
class CBaseUIScrollView;
class CBaseUIImage;

class OsuVRTutorial : public OsuScreenBackable
{
public:
	OsuVRTutorial(Osu *osu);
	virtual ~OsuVRTutorial();

	void draw(Graphics *g);
	void update();

	void onKeyDown(KeyboardEvent &e);

	void setVisible(bool visible);

private:
	virtual void updateLayout();
	virtual void onBack();

	CBaseUIContainer *m_container;
	CBaseUIScrollView *m_scrollView;
	CBaseUIImage *m_tutorialImage1;
	CBaseUIImage *m_tutorialImage2;
};

#endif
