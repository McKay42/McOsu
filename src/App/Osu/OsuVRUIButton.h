//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		a simple empty VR button
//
// $NoKeywords: $osuvrbt
//===============================================================================//

#ifndef OSUVRUIBUTTON_H
#define OSUVRUIBUTTON_H

#include "OsuVRUIElement.h"

class OsuVRUIButton : public OsuVRUIElement
{
public:
	OsuVRUIButton(OsuVR *vr, float x, float y, float width, float height);

	virtual void drawVR(Graphics *g, Matrix4 &mvp);
	virtual void update(Vector2 cursorPos);

	typedef fastdelegate::FastDelegate0<> ButtonClickVoidCallback;
	void setClickCallback(ButtonClickVoidCallback clickCallback) {m_clickVoidCallback = clickCallback;}

protected:
	virtual void onClicked();

private:
	bool m_bClickCheck;
	ButtonClickVoidCallback m_clickVoidCallback;
};

#endif
