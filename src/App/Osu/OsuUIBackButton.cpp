//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		big ass back button (options, songbrowser)
//
// $NoKeywords: $osubbt
//===============================================================================//

#include "OsuUIBackButton.h"

#include "Engine.h"
#include "ConVar.h"
#include "AnimationHandler.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuSkinImage.h"

OsuUIBackButton::OsuUIBackButton(Osu *osu, float xPos, float yPos, float xSize, float ySize, UString name) : CBaseUIButton(xPos, yPos, xSize, ySize, name, "")
{
	m_osu = osu;
	m_fAnimation = 0.0f;
	m_fImageScale = 1.0f;
}

void OsuUIBackButton::draw(Graphics *g)
{
	if (!m_bVisible) return;

	const float scaleAnimMultiplier = 0.01f;

	// draw button image
	g->pushTransform();
	{
		g->translate(m_vSize.x/2, -m_vSize.y/2);
		g->scale((1.0f + m_fAnimation*scaleAnimMultiplier), (1.0f + m_fAnimation*scaleAnimMultiplier));
		g->translate(-m_vSize.x/2, m_vSize.y/2);
		g->setColor(0xffffffff);
		m_osu->getSkin()->getMenuBack2()->draw(g, m_vPos + (m_osu->getSkin()->getMenuBack2()->getSize()/2)*m_fImageScale, m_fImageScale);
	}
	g->popTransform();

	// draw anim highlight overlay
	if (m_fAnimation > 0.0f)
	{
		g->pushTransform();
		{
			g->setColor(0xffffffff);
			g->setAlpha(m_fAnimation*0.15f);
			g->translate(m_vSize.x/2, -m_vSize.y/2);
			g->scale(1.0f + m_fAnimation*scaleAnimMultiplier, 1.0f + m_fAnimation*scaleAnimMultiplier);
			g->translate(-m_vSize.x/2, m_vSize.y/2);
			g->translate(m_vPos.x + m_vSize.x/2, m_vPos.y + m_vSize.y/2);
			g->fillRect(-m_vSize.x/2, -m_vSize.y/2, m_vSize.x, m_vSize.y + 5);
		}
		g->popTransform();
	}
}

void OsuUIBackButton::update()
{
	CBaseUIButton::update();
	if (!m_bVisible) return;
}

void OsuUIBackButton::onMouseInside()
{
	CBaseUIButton::onMouseInside();

	anim->moveQuadOut(&m_fAnimation, 1.0f, 0.1f, 0.0f, true);
}

void OsuUIBackButton::onMouseOutside()
{
	CBaseUIButton::onMouseOutside();

	anim->moveQuadOut(&m_fAnimation, 0.0f, m_fAnimation*0.1f, 0.0f, true);
}

void OsuUIBackButton::updateLayout()
{
	const float uiScale = Osu::ui_scale->getFloat();

	Vector2 newSize = m_osu->getSkin()->getMenuBack2()->getSize();
	newSize.y = clamp<float>(newSize.y, 0, m_osu->getSkin()->getMenuBack2()->getSizeBase().y*1.5f); // clamp the height down if it exceeds 1.5x the base height
	newSize *= uiScale;
	m_fImageScale = (newSize.y / m_osu->getSkin()->getMenuBack2()->getSize().y);
	setSize(newSize);
}

void OsuUIBackButton::resetAnimation()
{
	anim->deleteExistingAnimation(&m_fAnimation);
	m_fAnimation = 0.0f;
}
