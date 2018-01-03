//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		baseclass for VR UI elements, similar to ~CBaseUIElement
//
// $NoKeywords: $osuvrui
//===============================================================================//

#ifndef OSUVRUIELEMENT_H
#define OSUVRUIELEMENT_H

#include "cbase.h"

class OsuVR;

class OsuVRUIElement
{
public:
	OsuVRUIElement(OsuVR *vr, float x, float y, float width, float height);
	virtual ~OsuVRUIElement() {;}

	virtual void drawVR(Graphics *g, Matrix4 &mvp);
	virtual void update(Vector2 cursorPos);

	virtual void setPos(Vector2 pos) {m_vPos = pos;}
	virtual void setPos(float x, float y) {m_vPos.x = x; m_vPos.y = y;}
	virtual void setPosX(float x) {m_vPos.x = x;}
	virtual void setPosY(float y) {m_vPos.y = y;}
	virtual void setSize(Vector2 size) {m_vSize = size;}

	virtual void setVisible(bool visible) {m_bIsVisible = visible;}

	inline const Vector2& getPos() const {return m_vPos;}
	inline const Vector2& getSize() const {return m_vSize;}

	virtual bool isVisible() {return m_bIsVisible;}
	virtual bool isActive() {return m_bIsActive && isVisible();}
	virtual bool isCursorInside() {return m_bIsCursorInside && isVisible();}

protected:
	virtual void onCursorInside() {;}
	virtual void onCursorOutside() {;}

	OsuVR *m_vr;

	Vector2 m_vPos;
	Vector2 m_vSize;

	bool m_bIsVisible;
	bool m_bIsActive;
	bool m_bIsCursorInside;
};

#endif
