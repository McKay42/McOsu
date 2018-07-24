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

OsuUIPauseMenuButton::OsuUIPauseMenuButton(Osu *osu, UString imageResourceName, float xPos, float yPos, float xSize, float ySize, UString name) : CBaseUIImageButton(imageResourceName, xPos, yPos, xSize, ySize, name)
{
	m_osu = osu;
	m_fScaleMultiplier = 1.1f;
}

void OsuUIPauseMenuButton::setBaseScale(float xScale, float yScale)
{
	m_vBaseScale.x = xScale;
	m_vBaseScale.y = yScale;

	m_vScale = m_vBaseScale;
}

void OsuUIPauseMenuButton::onMouseInside()
{
	CBaseUIImageButton::onMouseInside();

	if (engine->hasFocus())
		engine->getSound()->play(m_osu->getSkin()->getMenuClick());

	const float animationDuration = 0.09f;
	anim->moveLinear(&m_vScale.x, m_vBaseScale.x * m_fScaleMultiplier, animationDuration, true);
	anim->moveLinear(&m_vScale.y, m_vBaseScale.y * m_fScaleMultiplier, animationDuration, true);
}

void OsuUIPauseMenuButton::onMouseOutside()
{
	CBaseUIImageButton::onMouseOutside();

	const float animationDuration = 0.09f;
	anim->moveLinear(&m_vScale.x, m_vBaseScale.x, animationDuration, true);
	anim->moveLinear(&m_vScale.y, m_vBaseScale.y, animationDuration, true);
}
