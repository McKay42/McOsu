//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		baseclass for VR UI elements, see ~CBaseUIElement
//
// $NoKeywords: $osuvrui
//===============================================================================//

#include "OsuVRUIElement.h"

OsuVRUIElement::OsuVRUIElement(OsuVR *vr, float x, float y, float width, float height)
{
	m_vr = vr;
	m_vPos = Vector2(x, y);
	m_vSize = Vector2(width, height);

	m_bIsVisible = true;
	m_bIsActive = false;
	m_bIsCursorInside = false;
}

void OsuVRUIElement::drawVR(Graphics *g, Matrix4 &mvp)
{
}

void OsuVRUIElement::update(Vector2 cursorPos)
{
	McRect bounds(m_vPos.x, -m_vPos.y, m_vSize.x, m_vSize.y);
	if (bounds.contains(Vector2(cursorPos.x, -cursorPos.y)))
	{
		if (!m_bIsCursorInside)
		{
			m_bIsCursorInside = true;
			onCursorInside();
		}
	}
	else if (m_bIsCursorInside)
	{
		m_bIsCursorInside = false;
		onCursorOutside();
	}
}
