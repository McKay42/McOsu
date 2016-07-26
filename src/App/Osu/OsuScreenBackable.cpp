//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		screen + back button
//
// $NoKeywords: $
//===============================================================================//

#include "OsuScreenBackable.h"

#include "Keyboard.h"
#include "ResourceManager.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuKeyBindings.h"

#include "OsuUIBackButton.h"

OsuScreenBackable::OsuScreenBackable(Osu *osu) : OsuScreen()
{
	m_osu = osu;

	m_backButton = new OsuUIBackButton("", 0, 0, 0, 0, "");
	m_backButton->setScaleToFit(true);
	m_backButton->setClickCallback( MakeDelegate(this, &OsuScreenBackable::onBack) );

	updateLayout();
}

OsuScreenBackable::~OsuScreenBackable()
{
	SAFE_DELETE(m_backButton);
}

void OsuScreenBackable::draw(Graphics *g)
{
	if (!m_bVisible) return;

	m_backButton->draw(g);
}

void OsuScreenBackable::update()
{
	if (!m_bVisible) return;

	m_backButton->update();
}

void OsuScreenBackable::onKeyDown(KeyboardEvent &e)
{
	if (!m_bVisible) return;

	if (e == KEY_ESCAPE || e == (KEYCODE)OsuKeyBindings::GAME_PAUSE.getInt())
		onBack();

	e.consume();
}

void OsuScreenBackable::onKeyUp(KeyboardEvent &e)
{
	if (!m_bVisible) return;

	e.consume();
}

void OsuScreenBackable::onChar(KeyboardEvent &e)
{
	if (!m_bVisible) return;

	e.consume();
}

void OsuScreenBackable::updateLayout()
{
	m_backButton->setImageResourceName(m_osu->getSkin()->getMenuBack()->getName());

	// back button
	Image *backButton = m_osu->getSkin()->getMenuBack();
	float backButtonScale = Osu::getImageScale(backButton, 86.0f);
	float maxFactor = 0.169f; // HACKHACK:
	if (backButton->getWidth()*backButtonScale > Osu::getScreenWidth()*maxFactor)
		backButtonScale = (Osu::getScreenWidth()*maxFactor)/backButton->getWidth();
	m_backButton->setSize(backButton->getWidth()*backButtonScale, backButton->getHeight()*backButtonScale);
	m_backButton->setPosY(Osu::getScreenHeight() - m_backButton->getSize().y);
}

void OsuScreenBackable::onResolutionChange(Vector2 newResolution)
{
	updateLayout();
}
