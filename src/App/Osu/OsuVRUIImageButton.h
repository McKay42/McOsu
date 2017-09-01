//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		a simple VR button with an image/icon on top
//
// $NoKeywords: $osuvribt
//===============================================================================//

#ifndef OSUVRUIIMAGEBUTTON_H
#define OSUVRUIIMAGEBUTTON_H

#include "OsuVRUIButton.h"

class OsuVRUIImageButton : public OsuVRUIButton
{
public:
	OsuVRUIImageButton(OsuVR *vr, float x, float y, float width, float height, UString imageResourceName);

	virtual void drawVR(Graphics *g, Matrix4 &mvp);
	virtual void update(Vector2 cursorPos);

private:
	virtual void onCursorInside();
	virtual void onCursorOutside();

	void updateImageResource();

	UString m_sImageResourceName;
	Image *m_image;

	float m_fAnimation;
};

#endif
