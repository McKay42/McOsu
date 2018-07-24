//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		tooltips
//
// $NoKeywords: $osutt
//===============================================================================//

#include "OsuTooltipOverlay.h"

#include "Engine.h"
#include "ConVar.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"
#include "Mouse.h"

#include "Osu.h"

ConVar osu_tooltip_anim_duration("osu_tooltip_anim_duration", 0.4f);

OsuTooltipOverlay::OsuTooltipOverlay(Osu *osu) : OsuScreen(osu)
{
	m_fAnim = 0.0f;
	m_bDelayFadeout = false;
}

OsuTooltipOverlay::~OsuTooltipOverlay()
{
}

void OsuTooltipOverlay::draw(Graphics *g)
{
	if (m_fAnim > 0.0f)
	{
		McFont *font = engine->getResourceManager()->getFont("FONT_DEFAULT");

		Vector2 offset = Vector2(10, 10);
		int margin = 5;
		int lineSpacing = 8;
		float alpha = m_fAnim*m_fAnim*m_fAnim;

		int width = 0;
		for (int i=0; i<m_lines.size(); i++)
		{
			float lineWidth = font->getStringWidth(m_lines[i]);
			if (lineWidth > width)
				width = lineWidth;
		}
		int height = font->getHeight()*m_lines.size() + lineSpacing*(m_lines.size()-1) + 3;

		Vector2 cursorPos = engine->getMouse()->getPos();

		// clamp to right edge
		if (cursorPos.x + width + offset.x + 2*margin > m_osu->getScreenWidth())
			cursorPos.x -= (cursorPos.x + width + offset.x + 2*margin) - m_osu->getScreenWidth() + 1;

		// clamp to bottom edge
		if (cursorPos.y + height + offset.y + 2*margin > m_osu->getScreenHeight())
			cursorPos.y -= (cursorPos.y + height + offset.y + 2*margin) - m_osu->getScreenHeight() + 1;

		// draw background
		g->setColor(0xff000000);
		g->setAlpha(alpha);
		g->fillRect((int)cursorPos.x + offset.x, (int)cursorPos.y + offset.y, width + 2*margin, height + 2*margin);

		// draw text
		g->setColor(0xffffffff);
		g->setAlpha(alpha);
		g->pushTransform();
		g->translate((int)cursorPos.x + offset.x + margin, (int)cursorPos.y + offset.y + margin + font->getHeight());
		for (int i=0; i<m_lines.size(); i++)
		{
			g->drawString(font, m_lines[i]);
			g->translate(0, font->getHeight() + lineSpacing);
		}
		g->popTransform();

		// draw border
		g->setColor(0xffffffff);
		g->setAlpha(alpha);
		g->drawRect((int)cursorPos.x + offset.x, (int)cursorPos.y + offset.y, width + 2*margin, height + 2*margin);
	}
}

void OsuTooltipOverlay::update()
{
	if (m_bDelayFadeout)
		m_bDelayFadeout = false;
	else if (m_fAnim > 0.0f)
		anim->moveLinear(&m_fAnim, 0.0f, (m_fAnim)*osu_tooltip_anim_duration.getFloat(), true);
}

void OsuTooltipOverlay::begin()
{
	m_lines.clear();
	m_bDelayFadeout = true;
}

void OsuTooltipOverlay::addLine(UString text)
{
	m_lines.push_back(text);
}

void OsuTooltipOverlay::end()
{
	anim->moveLinear(&m_fAnim, 1.0f, (1.0f-m_fAnim)*osu_tooltip_anim_duration.getFloat(), true);
}
