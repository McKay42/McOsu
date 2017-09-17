//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		osu!mania gamemode
//
// $NoKeywords: $osumania
//===============================================================================//

#include "OsuBeatmapMania.h"

#include "Engine.h"
#include "ConVar.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "AnimationHandler.h"

#include "Osu.h"
#include "OsuScore.h"
#include "OsuKeyBindings.h"
#include "OsuBeatmapDifficulty.h"
#include "OsuHitObject.h"

ConVar osu_mania_playfield_width_percent("osu_mania_playfield_width_percent", 0.25f);
ConVar osu_mania_playfield_height_percent("osu_mania_playfield_height_percent", 0.85f);
ConVar osu_mania_playfield_offset_x_percent("osu_mania_playfield_offset_x_percent", 0.44f);
ConVar osu_mania_k_override("osu_mania_k_override", -1);

OsuBeatmapMania::OsuBeatmapMania(Osu *osu) : OsuBeatmap(osu)
{
	for (int i=0; i<128; i++)
	{
		m_bColumnKeyDown[i] = false;
	}

	m_fZoom = 0.0f;
}

void OsuBeatmapMania::draw(Graphics *g)
{
	OsuBeatmap::draw(g);
	if (!canDraw()) return;

	if (isLoading()) return; // only start drawing the rest of the playfield if everything has loaded

	g->setAntialiasing(true);
	g->push3DScene(McRect(m_vPlayfieldCenter.x - m_vPlayfieldSize.x/2, m_vPlayfieldCenter.y - m_vPlayfieldSize.y/2, m_vPlayfieldSize.x, m_vPlayfieldSize.y));
	{
		g->translate3DScene(0,0,m_fZoom);
		g->rotate3DScene(-m_vRotation.y, m_vRotation.x, 0);

		const float columnWidth = m_vPlayfieldSize.x / getNumColumns();

		// draw playfield border
		if (true)
		{
			g->setColor(0xffffffff);

			// left
			g->drawLine(m_vPlayfieldCenter.x - m_vPlayfieldSize.x/2, -1, m_vPlayfieldCenter.x - m_vPlayfieldSize.x/2, m_osu->getScreenHeight() + 2);

			// HACKHACK: hardcoded 10k offset
			const int tenkadd = getNumColumns() == 10 ? 1 : 0;

			// right
			g->drawLine((int)(m_vPlayfieldCenter.x - m_vPlayfieldSize.x/2) + ((int)(columnWidth))*(getNumColumns()+tenkadd), -1, (int)(m_vPlayfieldCenter.x - m_vPlayfieldSize.x/2) + ((int)(columnWidth))*(getNumColumns()+tenkadd), m_osu->getScreenHeight() + 2);

			if (getNumColumns() == 10)
			{
				g->drawLine((int)(m_vPlayfieldCenter.x - m_vPlayfieldSize.x/2) + ((int)(columnWidth))*(5), -1, (int)(m_vPlayfieldCenter.x - m_vPlayfieldSize.x/2) + ((int)(columnWidth))*(5), m_osu->getScreenHeight() + 2);
				g->drawLine((int)(m_vPlayfieldCenter.x - m_vPlayfieldSize.x/2) + ((int)(columnWidth))*(6), -1, (int)(m_vPlayfieldCenter.x - m_vPlayfieldSize.x/2) + ((int)(columnWidth))*(6), m_osu->getScreenHeight() + 2);
			}
		}

		// draw key blocks
		if (true)
		{
			int xPos = m_vPlayfieldCenter.x - m_vPlayfieldSize.x/2;
			int yPos = (int)(m_vPlayfieldCenter.y - m_vPlayfieldSize.y/2) + (int)(m_vPlayfieldSize.y) + 1; // kill me pls

			g->setColor(0xffffffff);
			for (int i=0; i<getNumColumns(); i++)
			{
				// HACKHACK: hardcoded 10k offset
				const int tenkoffset = getNumColumns() == 10 && i > 4 ? columnWidth : 0;

				if (m_bColumnKeyDown[i])
					g->fillRect(xPos + tenkoffset, yPos, columnWidth+1, 999);
				else
					g->drawRect(xPos + tenkoffset, yPos, columnWidth, 999);

				xPos += (int)columnWidth;
			}
		}

		// draw all hitobjects in reverse
		for (int i=m_hitobjects.size()-1; i>=0; i--)
		{
			m_hitobjects[i]->draw(g);
		}

		// draw hidden stage overlay
		if (m_osu->getModHD())
		{
			const Color topColor = 0xff000000;
			const Color bottomColor = 0x00000000;
			float heightAnimPercent = (float)std::min(/*400*/2*160, 160 + m_osu->getScore()->getCombo() / 2) / (float)(3*160);
			int heightStartOffset = m_vPlayfieldSize.y*heightAnimPercent;
			g->setColor(0xff000000);
			g->fillRect(m_vPlayfieldCenter.x - m_vPlayfieldSize.x/2 - 1, m_vPlayfieldCenter.y - m_vPlayfieldSize.y/2, m_vPlayfieldSize.x + 2, heightStartOffset + 2);
			g->fillGradient(m_vPlayfieldCenter.x - m_vPlayfieldSize.x/2 - 1, (m_vPlayfieldCenter.y - m_vPlayfieldSize.y/2) + heightStartOffset, m_vPlayfieldSize.x + 2, m_vPlayfieldSize.y-heightStartOffset, topColor, topColor, bottomColor, bottomColor);
		}
	}
	g->pop3DScene();
	g->setAntialiasing(false);
}

void OsuBeatmapMania::update()
{
	if (!canUpdate())
	{
		OsuBeatmap::update();
		return;
	}

	// baseclass call (does the actual hitobject updates among other things)
	OsuBeatmap::update();

	if (isLoading()) return; // only continue if we have loaded everything

	// update playfield metrics
	m_vPlayfieldSize.x = m_osu->getScreenSize().x * osu_mania_playfield_width_percent.getFloat();
	m_vPlayfieldSize.y = m_osu->getScreenSize().y * osu_mania_playfield_height_percent.getFloat();
	m_vPlayfieldCenter.x = m_osu->getScreenSize().x * osu_mania_playfield_offset_x_percent.getFloat();
	m_vPlayfieldCenter.y = m_vPlayfieldSize.y / 2.0f;

	// handle mouse 3d rotation
	if (engine->getKeyboard()->isControlDown())
	{
		Vector2 delta = engine->getMouse()->getPos() - m_vMouseBackup;
		m_vMouseBackup = engine->getMouse()->getPos();

		if (!anim->isAnimating(&m_vRotation.x) && !anim->isAnimating(&m_vRotation.y) && !anim->isAnimating(&m_fZoom))
		{
			if (engine->getMouse()->isLeftDown())
				m_vRotation += delta*0.5f;
			if (engine->getMouse()->isMiddleDown())
				m_fZoom += delta.y*0.5f;
		}
	}
	if (engine->getMouse()->isRightDown() && !anim->isAnimating(&m_vRotation.x) && !anim->isAnimating(&m_vRotation.y) && !anim->isAnimating(&m_fZoom) && (m_vRotation.x != 0.0f || m_vRotation.y != 0.0f || m_fZoom != 0.0f))
	{
		anim->moveQuadInOut(&m_vRotation.x, 0.0f, 1.0f, 0.0f, true);
		anim->moveQuadInOut(&m_vRotation.y, 0.0f, 1.0f, 0.0f, true);
		anim->moveQuadInOut(&m_fZoom, 0.0f, 1.0f, 0.0f, true);
	}
}

void OsuBeatmapMania::onKeyDown(KeyboardEvent &key)
{
	// lock asap
	std::lock_guard<std::mutex> lk(m_clicksMutex);

	int column = getColumnForKey(getNumColumns(), key);
	if (column != -1 && !m_bColumnKeyDown[column])
	{
		m_bColumnKeyDown[column] = true;

		CLICK click;
		click.musicPos = m_iCurMusicPosWithOffsets;
		click.maniaColumn = column;

		m_clicks.push_back(click);
	}
}

void OsuBeatmapMania::onKeyUp(KeyboardEvent &key)
{
	// lock asap
	std::lock_guard<std::mutex> lk(m_clicksMutex);

	int column = getColumnForKey(getNumColumns(), key);
	if (column != -1 && m_bColumnKeyDown[column])
	{
		m_bColumnKeyDown[column] = false;

		CLICK click;
		click.musicPos = m_iCurMusicPosWithOffsets;
		click.maniaColumn = column;

		m_keyUps.push_back(click);
	}
}

int OsuBeatmapMania::getNumColumns()
{
	if (m_selectedDifficulty != NULL)
		return (osu_mania_k_override.getInt() > 0 ? osu_mania_k_override.getInt() : (int)std::round(m_selectedDifficulty->CS));
	else
		return 4;
}

int OsuBeatmapMania::getColumnForKey(int numColumns, KeyboardEvent &key)
{
	if (numColumns > 0 && numColumns < 11)
	{
		for (int i=0; i<numColumns; i++)
		{
			if (key == (KEYCODE)OsuKeyBindings::MANIA[numColumns-1][i]->getInt())
				return i;
		}
	}
	return -1;
}
