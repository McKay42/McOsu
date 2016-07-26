//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		text bar overlay which can block inputs
//
// $NoKeywords: $osunot
//===============================================================================//

#include "OsuNotificationOverlay.h"

#include "Engine.h"
#include "ConVar.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"
#include "Keyboard.h"

#include "Osu.h"

ConVar osu_notification_duration("osu_notification_duration", 1.25f);

OsuNotificationOverlay::OsuNotificationOverlay(Osu *osu) : OsuScreen()
{
	m_osu = osu;

	m_bWaitForKey = false;
	m_keyListener = NULL;
}

void OsuNotificationOverlay::draw(Graphics *g)
{
	if (!isVisible()) return;

	if (m_bWaitForKey)
	{
		g->setColor(0x22ffffff);
		g->setAlpha((m_notification1.backgroundAnim/0.5f)*0.13f);
		g->fillRect(0, 0, Osu::getScreenWidth(), Osu::getScreenHeight());
	}

	drawNotificationBackground(g, m_notification2);
	drawNotificationBackground(g, m_notification1);
	drawNotificationText(g, m_notification2);
	drawNotificationText(g, m_notification1);
}

void OsuNotificationOverlay::drawNotificationText(Graphics *g, OsuNotificationOverlay::NOTIFICATION &n)
{
	McFont *font = m_osu->getSubTitleFont();
	int height = font->getHeight()*2;
	int stringWidth = font->getStringWidth(n.text);

	g->pushTransform();
		g->setColor(0xff000000);
		g->setAlpha(n.alpha);
		g->translate(Osu::getScreenWidth()/2 - stringWidth/2 + 1, Osu::getScreenHeight()/2 + font->getHeight()/2 + n.fallAnim*height*0.15f + 1);
		g->drawString(font, n.text);

		g->setColor(n.textColor);
		g->setAlpha(n.alpha);
		g->translate(-1, -1);
		g->drawString(font, n.text);
	g->popTransform();
}

void OsuNotificationOverlay::drawNotificationBackground(Graphics *g, OsuNotificationOverlay::NOTIFICATION &n)
{
	McFont *font = m_osu->getSubTitleFont();
	int height = font->getHeight()*2*n.backgroundAnim;

	g->setColor(0xff000000);
	g->setAlpha(n.alpha*0.75f);
	g->fillRect(0, Osu::getScreenHeight()/2 - height/2, Osu::getScreenWidth(), height);
}

void OsuNotificationOverlay::update()
{
	if (!isVisible()) return;


}

void OsuNotificationOverlay::onKeyDown(KeyboardEvent &e)
{
	if (!isVisible()) return;

	if (e.getKeyCode() == KEY_ESCAPE)
	{
		if (m_bWaitForKey)
			e.consume();
		m_bWaitForKey = false;
	}

	if (m_bWaitForKey)
	{
		// TODO: create key <=> name mapping (e.g. TAB/SHIFT/CONTROL etc.)
		//McFont *font = m_osu->getSubTitleFont();
		//if (font->hasGlyph(UString::format("%c", e.getKeyCode()).wc_str()[0]))
		//	addNotification(UString::format("%c is the new key.", e.getKeyCode()));
		//else
		float prevDuration = osu_notification_duration.getFloat();
		osu_notification_duration.setValue(0.85f);
		addNotification(UString::format("The new key is (ASCII Keycode): %lu", e.getKeyCode()));
		osu_notification_duration.setValue(prevDuration);

		if (m_keyListener != NULL)
			m_keyListener->onKey(e);

		m_bWaitForKey = false;
		e.consume();
	}

	if (m_bWaitForKey)
		e.consume();
}

void OsuNotificationOverlay::onKeyUp(KeyboardEvent &e)
{
	if (!isVisible()) return;

	if (m_bWaitForKey)
		e.consume();
}

void OsuNotificationOverlay::onChar(KeyboardEvent &e)
{
	if (!isVisible()) return;

	if (m_bWaitForKey)
		e.consume();
}

void OsuNotificationOverlay::addNotification(UString text, Color textColor, bool waitForKey)
{
	// swap effect
	if (isVisible())
	{
		m_notification2.text = m_notification1.text;
		m_notification2.textColor = 0xffffffff;

		m_notification2.time = 0.0f;
		m_notification2.alpha = 0.5f;
		m_notification2.backgroundAnim = 1.0f;
		m_notification2.fallAnim = 0.0f;

		anim->deleteExistingAnimation(&m_notification1.alpha);

		anim->moveQuadIn(&m_notification2.fallAnim, 1.0f, 0.2f, 0.0f, true);
		anim->moveQuadIn(&m_notification2.alpha, 0.0f, 0.2f, 0.0f, true);
	}

	// new notification
	m_bWaitForKey = waitForKey;

	float fadeOutTime = 0.4f;

	m_notification1.text = text;
	m_notification1.textColor = textColor;

	if (!waitForKey)
		m_notification1.time = engine->getTime() + osu_notification_duration.getFloat() + fadeOutTime;
	else
		m_notification1.time = 0.0f;
	m_notification1.alpha = 0.0f;
	m_notification1.backgroundAnim = 0.5f;
	m_notification1.fallAnim = 0.0f;

	// animations
	if (isVisible())
		m_notification1.alpha = 1.0f;
	else
		anim->moveLinear(&m_notification1.alpha, 1.0f, 0.075f, true);

	if (!waitForKey)
		anim->moveQuadOut(&m_notification1.alpha, 0.0f, fadeOutTime, osu_notification_duration.getFloat(), false);

	anim->moveQuadOut(&m_notification1.backgroundAnim, 1.0f, 0.15f, 0.0f, true);
}

bool OsuNotificationOverlay::isVisible()
{
	return engine->getTime() < m_notification1.time || engine->getTime() < m_notification2.time || m_bWaitForKey;
}
