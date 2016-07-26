//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		big ass back button (options, songbrowser)
//
// $NoKeywords: $osubbt
//===============================================================================//

#include "OsuUIBackButton.h"

#include "Engine.h"
#include "AnimationHandler.h"

OsuUIBackButton::OsuUIBackButton(UString imageResourceName, float xPos, float yPos, float xSize, float ySize, UString name) : CBaseUIImageButton(imageResourceName, xPos, yPos, xSize, ySize, name)
{
	m_fAnimation = 0.0f;
}

void OsuUIBackButton::draw(Graphics *g)
{
	float scaleAnimMultiplier = 0.01f;

	g->pushTransform();
	g->translate(m_vSize.x/2, -m_vSize.y/2);
	g->scale((1.0f + m_fAnimation*scaleAnimMultiplier), (1.0f + m_fAnimation*scaleAnimMultiplier));
	g->translate(-m_vSize.x/2, m_vSize.y/2);
	CBaseUIImageButton::draw(g);
	g->popTransform();

	if (m_fAnimation > 0.0f)
	{
		g->pushTransform();
		g->setColor(0xffffffff);
		g->setAlpha(m_fAnimation*0.15f);
		g->translate(m_vSize.x/2, -m_vSize.y/2);
		g->scale(1.0f + m_fAnimation*scaleAnimMultiplier, 1.0f + m_fAnimation*scaleAnimMultiplier);
		g->translate(-m_vSize.x/2, m_vSize.y/2);
		g->translate(m_vPos.x + m_vSize.x/2, m_vPos.y + m_vSize.y/2);
		g->fillRect(-m_vSize.x/2, -m_vSize.y/2, m_vSize.x, m_vSize.y + 5);
		g->popTransform();
	}
}

void OsuUIBackButton::onMouseInside()
{
	CBaseUIImageButton::onMouseInside();

	anim->moveQuadOut(&m_fAnimation, 1.0f, 0.1f, 0.0f, true);
}

void OsuUIBackButton::onMouseOutside()
{
	CBaseUIImageButton::onMouseOutside();

	anim->moveQuadOut(&m_fAnimation, 0.0f, m_fAnimation*0.1f, 0.0f, true);
}

