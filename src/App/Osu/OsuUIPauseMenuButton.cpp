//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		pause menu button
//
// $NoKeywords: $
//===============================================================================//

#include "OsuUIPauseMenuButton.h"

#include "Engine.h"
#include "AnimationHandler.h"
#include "SoundEngine.h"

#include "Osu.h"
#include "OsuSkin.h"

OsuUIPauseMenuButton::OsuUIPauseMenuButton(Osu *osu, std::function<Image*()> getImageFunc, float xPos, float yPos, float xSize, float ySize, UString name) : CBaseUIButton(xPos, yPos, xSize, ySize, name)
{
	m_osu = osu;
	this->getImageFunc = getImageFunc;

	m_vScale = Vector2(1, 1);
	m_fScaleMultiplier = 1.1f;
}

void OsuUIPauseMenuButton::draw(Graphics *g)
{
	if (!m_bVisible) return;

	// draw image
	Image *image = getImageFunc();
	if (image != NULL)
	{
		g->setColor(0xffffffff);
		g->pushTransform();
		{
			// scale
			g->scale(m_vScale.x, m_vScale.y);

			// center and draw
			g->translate(m_vPos.x + (int)(m_vSize.x/2), m_vPos.y + (int)(m_vSize.y/2));
			g->drawImage(image);
		}
		g->popTransform();
	}
}

void OsuUIPauseMenuButton::setBaseScale(float xScale, float yScale)
{
	m_vBaseScale.x = xScale;
	m_vBaseScale.y = yScale;

	m_vScale = m_vBaseScale;
}

void OsuUIPauseMenuButton::onMouseInside()
{
	CBaseUIButton::onMouseInside();

	if (engine->hasFocus())
		engine->getSound()->play(m_osu->getSkin()->getMenuClick());

	const float animationDuration = 0.09f;
	anim->moveLinear(&m_vScale.x, m_vBaseScale.x * m_fScaleMultiplier, animationDuration, true);
	anim->moveLinear(&m_vScale.y, m_vBaseScale.y * m_fScaleMultiplier, animationDuration, true);
}

void OsuUIPauseMenuButton::onMouseOutside()
{
	CBaseUIButton::onMouseOutside();

	const float animationDuration = 0.09f;
	anim->moveLinear(&m_vScale.x, m_vBaseScale.x, animationDuration, true);
	anim->moveLinear(&m_vScale.y, m_vBaseScale.y, animationDuration, true);
}
