//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		a simple filled VR slider
//
// $NoKeywords: $osuvrsl
//===============================================================================//

#ifndef OSUVRUISLIDER_H
#define OSUVRUISLIDER_H

#include "OsuVRUIElement.h"

class OsuVRUISlider : public OsuVRUIElement
{
public:
	OsuVRUISlider(OsuVR *vr, float x, float y, float width, float height);

	virtual void drawVR(Graphics *g, Matrix4 &mvp);
	virtual void update(Vector2 cursorPos);

	typedef fastdelegate::FastDelegate1<OsuVRUISlider*> SliderChangeCallback;
	void setChangeCallback(SliderChangeCallback changeCallback) {m_sliderChangeCallback = changeCallback;}

	void setValue(float value, bool ignoreCallback = false);

	inline float getFloat() {return m_fCurValue;}

private:
	SliderChangeCallback m_sliderChangeCallback;

	float m_fMinValue, m_fMaxValue, m_fCurValue, m_fCurPercent;
	bool m_bClickCheck;
};

#endif
