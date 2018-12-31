//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		user card/button (shows total weighted pp + user switcher)
//
// $NoKeywords: $
//===============================================================================//

#include "OsuUISongBrowserUserButton.h"

#include "Engine.h"
#include "AnimationHandler.h"
#include "ResourceManager.h"
#include "Mouse.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuIcons.h"
#include "OsuTooltipOverlay.h"
#include "OsuSongBrowser2.h"
#include "OsuDatabase.h"

// NOTE: selected username is stored in m_sText

OsuUISongBrowserUserButton::OsuUISongBrowserUserButton(Osu *osu) : CBaseUIButton()
{
	m_osu = osu;

	m_fPP = 0.0f;
	m_fAcc = 0.0f;
	m_iLevel = 0;
	m_fPercentToNextLevel = 0.0f;

	m_iPPIncrease = 0;

	m_fHoverAnim = 0.0f;
}

void OsuUISongBrowserUserButton::draw(Graphics *g)
{
	if (!m_bVisible) return;

	int yCounter = 0;

	// draw background
	const float backgroundBrightness = m_fHoverAnim * 0.175f;
	g->setColor(COLORf(1.0f, backgroundBrightness, backgroundBrightness, backgroundBrightness));
	g->fillRect(m_vPos.x+1, m_vPos.y+1, m_vSize.x-1, m_vSize.y-1);

	const float iconBorder = m_vSize.y*0.03f;
	const float iconHeight = m_vSize.y - 2*iconBorder;
	const float iconWidth = iconHeight;

	// draw user icon background
	g->setColor(COLORf(1.0f, 0.1f, 0.1f, 0.1f));
	g->fillRect(m_vPos.x + iconBorder + 1, m_vPos.y + iconBorder + 1, iconWidth, iconHeight);

	// draw user icon
	/*
	McFont *iconFont = m_osu->getFontIcons();
	g->setColor(0xffffffff);
	g->pushTransform();
	{
		UString iconString; iconString.insert(0, OsuIcons::USER);
		const float stringWidth = iconFont->getStringWidth(iconString);
		const float stringHeight = iconFont->getStringHeight(iconString);
		const float iconScale = (iconHeight / stringHeight)*0.5f;

		g->scale(iconScale, iconScale);
		g->translate(m_vPos.x + iconBorder + iconWidth/2 - (stringWidth/2)*iconScale, m_vPos.y + m_vSize.y/2 + (stringHeight/2)*iconScale);
		g->drawString(iconFont, iconString);
	}
	g->popTransform();
	*/

	g->setColor(0xffffffff);
	g->pushClipRect(McRect(m_vPos.x + iconBorder + 1, m_vPos.y + iconBorder + 2, iconWidth, iconHeight));
	g->pushTransform();
	{
		const float scale = Osu::getImageScaleToFillResolution(m_osu->getSkin()->getUserIcon(), Vector2(iconWidth, iconHeight));
		g->scale(scale, scale);
		g->translate(m_vPos.x + iconBorder + iconWidth/2 + 1, m_vPos.y + iconBorder + iconHeight/2 + 1);
		g->drawImage(m_osu->getSkin()->getUserIcon());
	}
	g->popTransform();
	g->popClipRect();

	// draw username
	McFont *usernameFont = m_osu->getSongBrowserFont();
	const float usernameScale = 0.5f;
	float usernamePaddingLeft = 0.0f;
	g->pushClipRect(McRect(m_vPos.x + iconBorder, m_vPos.y + iconBorder, m_vSize.x - 2*iconBorder, iconHeight));
	g->pushTransform();
	{
		const float height = m_vSize.y*0.5f;
		const float paddingTopPercent = (1.0f - usernameScale)*0.1f;
		const float paddingTop = height*paddingTopPercent;
		const float paddingLeftPercent = (1.0f - usernameScale)*0.31f;
		usernamePaddingLeft = height*paddingLeftPercent;
		const float scale = (height / usernameFont->getHeight())*usernameScale;

		yCounter += (int)(m_vPos.y + usernameFont->getHeight()*scale + paddingTop);

		g->scale(scale, scale);
		g->translate((int)(m_vPos.x + iconWidth + usernamePaddingLeft), yCounter);
		g->setColor(0xffffffff);
		g->drawString(usernameFont, m_sText);
	}
	g->popTransform();
	g->popClipRect();

	// draw performance (pp), and accuracy
	McFont *performanceFont = m_osu->getSubTitleFont();
	const float performanceScale = 0.3f;
	g->pushTransform();
	{
		UString performanceString = UString::format("Performance: %ipp", (int)std::round(m_fPP));
		UString accuracyString = UString::format("Accuracy: %.2f%%", m_fAcc*100.0f);

		const float height = m_vSize.y*0.5f;
		const float paddingTopPercent = (1.0f - performanceScale)*0.25f;
		const float paddingTop = height*paddingTopPercent;
		const float paddingMiddlePercent = (1.0f - performanceScale)*0.15f;
		const float paddingMiddle = height*paddingMiddlePercent;
		const float scale = (height / performanceFont->getHeight())*performanceScale;

		yCounter += performanceFont->getHeight()*scale + paddingTop;

		g->scale(scale, scale);
		g->translate((int)(m_vPos.x + iconWidth + usernamePaddingLeft), yCounter);
		g->setColor(0xffffffff);
		g->drawString(performanceFont, performanceString);

		yCounter += performanceFont->getHeight()*scale + paddingMiddle;

		g->translate(0, performanceFont->getHeight()*scale + paddingMiddle);
		g->drawString(performanceFont, accuracyString);
	}
	g->popTransform();

	// draw level
	McFont *scoreFont = m_osu->getSubTitleFont();
	const float scoreScale = 0.3f;
	g->pushTransform();
	{
		UString scoreString = UString::format("LV%i", m_iLevel);

		const float height = m_vSize.y*0.5f;
		const float paddingTopPercent = (1.0f - scoreScale)*0.25f;
		const float paddingTop = height*paddingTopPercent;
		const float scale = (height / scoreFont->getHeight())*scoreScale;

		yCounter += scoreFont->getHeight()*scale + paddingTop;

		g->scale(scale, scale);
		g->translate((int)(m_vPos.x + iconWidth + usernamePaddingLeft), yCounter);
		g->setColor(0xffffffff);
		g->drawString(scoreFont, scoreString);
	}
	g->popTransform();

	// draw level percentage bar (to next level)
	const float barBorder = (int)(iconBorder);
	const float barHeight = (int)(m_vSize.y - 2*barBorder)*0.1f;
	const float barWidth = (int)((m_vSize.x - 2*barBorder)*0.55f);
	g->setColor(0xffaaaaaa);
	g->drawRect(m_vPos.x + m_vSize.x - barWidth - barBorder - 1, m_vPos.y + m_vSize.y - barHeight - barBorder, barWidth, barHeight);
	g->fillRect(m_vPos.x + m_vSize.x - barWidth - barBorder - 1, m_vPos.y + m_vSize.y - barHeight - barBorder, barWidth*clamp<float>(m_fPercentToNextLevel, 0.0f, 1.0f), barHeight);
}

void OsuUISongBrowserUserButton::update()
{
	CBaseUIButton::update();
	if (!m_bVisible) return;

	/*
	if (isMouseInside())
	{
		m_osu->getTooltipOverlay()->begin();
		m_osu->getTooltipOverlay()->addLine("Click to change user.");
		m_osu->getTooltipOverlay()->addLine("(McOsu scores only!)");
		m_osu->getTooltipOverlay()->end();
	}
	*/
}

void OsuUISongBrowserUserButton::updateUserStats()
{
	OsuDatabase::PlayerStats stats = m_osu->getSongBrowser()->getDatabase()->calculatePlayerStats(m_sText);

	const bool changed = (m_fPP != stats.pp || m_fAcc != stats.accuracy || m_iLevel != stats.level || m_fPercentToNextLevel != stats.percentToNextLevel);

	m_fPP = stats.pp;
	m_fAcc = stats.accuracy;
	m_iLevel = stats.level;
	m_fPercentToNextLevel = stats.percentToNextLevel;

	if (changed)
	{
		anim->moveQuadOut(&m_fHoverAnim, 1.5f, 0.2f, true);
		anim->moveLinear(&m_fHoverAnim, 0.0f, 1.5f, 0.2f);
	}
}

void OsuUISongBrowserUserButton::onMouseInside()
{
	CBaseUIButton::onMouseInside();
	anim->moveLinear(&m_fHoverAnim, 1.0f, 0.15f*(1.0f - m_fHoverAnim), true);
}

void OsuUISongBrowserUserButton::onMouseOutside()
{
	CBaseUIButton::onMouseOutside();
	anim->moveLinear(&m_fHoverAnim, 0.0f, 0.15f*m_fHoverAnim, true);
}
