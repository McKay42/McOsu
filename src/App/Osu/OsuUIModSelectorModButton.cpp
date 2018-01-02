//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		mod image buttons (EZ, HD, HR, HT, DT, etc.)
//
// $NoKeywords: $osumsmb
//===============================================================================//

#include "OsuUIModSelectorModButton.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"
#include "SoundEngine.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuSkinImage.h"
#include "OsuModSelector.h"
#include "OsuTooltipOverlay.h"

OsuUIModSelectorModButton::OsuUIModSelectorModButton(Osu *osu, OsuModSelector *osuModSelector, float xPos, float yPos, float xSize, float ySize, UString name) : CBaseUIButton(xPos, yPos, xSize, ySize, name, "")
{
	m_osu = osu;
	m_osuModSelector = osuModSelector;
	m_iState = 0;
	m_vScale = m_vBaseScale = Vector2(1, 1);
	m_fRot = 0.0f;

	m_fEnabledScaleMultiplier = 1.25f;
	m_fEnabledRotationDeg = 6.0f;
	m_bAvailable = true;
	m_bOn = false;

	getActiveImageFunc = NULL;
}

void OsuUIModSelectorModButton::draw(Graphics *g)
{
	if (!m_bVisible) return;
	if (getActiveImageFunc != NULL && getActiveImageFunc())
	{
		g->pushTransform();

		g->scale(m_vScale.x, m_vScale.y);

		if (m_fRot != 0.0f)
			g->rotate(m_fRot);

		g->setColor(0xffffffff);
		getActiveImageFunc()->draw(g, m_vPos + m_vSize/2);

		g->popTransform();
	}

	if (!m_bAvailable)
	{
		const int size = m_vSize.x > m_vSize.y ? m_vSize.x : m_vSize.y;

		g->setColor(0xff000000);
		g->drawLine(m_vPos.x + 1, m_vPos.y, m_vPos.x + size + 1, m_vPos.y + size);
		g->setColor(0xffffffff);
		g->drawLine(m_vPos.x, m_vPos.y, m_vPos.x + size, m_vPos.y + size);
		g->setColor(0xff000000);
		g->drawLine(m_vPos.x + size + 1, m_vPos.y, m_vPos.x + 1, m_vPos.y + size);
		g->setColor(0xffffffff);
		g->drawLine(m_vPos.x + size, m_vPos.y, m_vPos.x, m_vPos.y + size);
	}
}

void OsuUIModSelectorModButton::update()
{
	CBaseUIButton::update();
	if (!m_bVisible) return;

	// handle tooltips
	if (isMouseInside() && m_bAvailable && m_states.size() > 0)
	{
		m_osu->getTooltipOverlay()->begin();
		for (int i=0; i<m_states[m_iState].tooltipTextLines.size(); i++)
		{
			m_osu->getTooltipOverlay()->addLine(m_states[m_iState].tooltipTextLines[i]);
		}
		m_osu->getTooltipOverlay()->end();
	}
}

void OsuUIModSelectorModButton::resetState()
{
	setOn(false);
	setState(0);
}

void OsuUIModSelectorModButton::onMouseDownInside()
{
	CBaseUIButton::onMouseDownInside();

	if (!m_bAvailable)
		return;

	// increase state, wrap around, switch on and off
	if (m_bOn)
	{
		m_iState = (m_iState+1) % m_states.size();

		if (m_iState == 0)
			setOn(false);
		else
			setOn(true);
	}
	else
		setOn(true);

	// set new state
	setState(m_iState);
}

void OsuUIModSelectorModButton::setBaseScale(float xScale, float yScale)
{
	m_vBaseScale.x = xScale;
	m_vBaseScale.y = yScale;
	m_vScale = m_vBaseScale;

	if (m_bOn)
	{
		m_vScale = m_vBaseScale * m_fEnabledScaleMultiplier;
		m_fRot = m_fEnabledRotationDeg;
	}
}

void OsuUIModSelectorModButton::setOn(bool on)
{
	bool prevState = m_bOn;
	m_bOn = on;
	const float animationDuration = 0.05f;

	if (m_bOn)
	{
		if (prevState)
		{
			// swap effect
			float swapDurationMultiplier = 0.65f;
			anim->moveLinear(&m_fRot, 0.0f, animationDuration*swapDurationMultiplier, true);
			anim->moveLinear(&m_vScale.x, m_vBaseScale.x, animationDuration*swapDurationMultiplier, true);
			anim->moveLinear(&m_vScale.y, m_vBaseScale.y, animationDuration*swapDurationMultiplier, true);

			anim->moveLinear(&m_fRot, m_fEnabledRotationDeg, animationDuration*swapDurationMultiplier, animationDuration*swapDurationMultiplier, false);
			anim->moveLinear(&m_vScale.x, m_vBaseScale.x * m_fEnabledScaleMultiplier, animationDuration*swapDurationMultiplier, animationDuration*swapDurationMultiplier, false);
			anim->moveLinear(&m_vScale.y, m_vBaseScale.y * m_fEnabledScaleMultiplier, animationDuration*swapDurationMultiplier, animationDuration*swapDurationMultiplier, false);
		}
		else
		{
			anim->moveLinear(&m_fRot, m_fEnabledRotationDeg, animationDuration, true);
			anim->moveLinear(&m_vScale.x, m_vBaseScale.x * m_fEnabledScaleMultiplier, animationDuration, true);
			anim->moveLinear(&m_vScale.y, m_vBaseScale.y * m_fEnabledScaleMultiplier, animationDuration, true);
		}

		engine->getSound()->play(m_osu->getSkin()->getCheckOn());
	}
	else
	{
		anim->moveLinear(&m_fRot, 0.0f, animationDuration, true);
		anim->moveLinear(&m_vScale.x, m_vBaseScale.x, animationDuration, true);
		anim->moveLinear(&m_vScale.y, m_vBaseScale.y, animationDuration, true);

		if (prevState && !m_bOn) // only play sound on specific change
			engine->getSound()->play(m_osu->getSkin()->getCheckOff());
	}
}

void OsuUIModSelectorModButton::setState(int state)
{
	m_iState = state;

	// update image
	if (m_iState < m_states.size() && m_states.size() > 0 && m_states[m_iState].getImageFunc != NULL)
		getActiveImageFunc = m_states[m_iState].getImageFunc;

	// update mods
	m_osuModSelector->updateModConVar();
}

void OsuUIModSelectorModButton::setState(unsigned int state, UString modName, UString tooltipText, std::function<OsuSkinImage*()> getImageFunc)
{
	// dynamically add new state
	while (m_states.size() < state+1)
	{
		STATE t;
		t.getImageFunc = NULL;
		m_states.push_back(t);
	}
	m_states[state].modName = modName;
	m_states[state].tooltipTextLines = tooltipText.split("\n");
	m_states[state].getImageFunc = getImageFunc;

	// set initial state image
	if (m_states.size() == 1)
		getActiveImageFunc = m_states[0].getImageFunc;
	else if (m_iState > -1 && m_iState < m_states.size()) // update current state image
		getActiveImageFunc = m_states[m_iState].getImageFunc;
}

UString OsuUIModSelectorModButton::getActiveModName()
{
	if (m_states.size() > 0 && m_iState < m_states.size())
		return m_states[m_iState].modName;
	else
		return "";
}
