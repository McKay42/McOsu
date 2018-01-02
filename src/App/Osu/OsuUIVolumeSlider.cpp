//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		slider used for the volume overlay HUD
//
// $NoKeywords: $osuvolsl
//===============================================================================//

#include "OsuUIVolumeSlider.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"

#include "Osu.h"

OsuUIVolumeSlider::OsuUIVolumeSlider(Osu *osu, float xPos, float yPos, float xSize, float ySize, UString name) : CBaseUISlider(xPos, yPos, xSize, ySize, name)
{
	m_osu = osu;

	m_type = TYPE::MASTER;
	m_bSelected = false;

	m_bWentMouseInside = false;
	m_fSelectionAnim = 0.0f;

	engine->getResourceManager()->loadImage("ic_volume_mute_white_48dp.png",	"OSU_UI_VOLUME_SLIDER_BLOCK_0");
	engine->getResourceManager()->loadImage("ic_volume_up_white_48dp.png",		"OSU_UI_VOLUME_SLIDER_BLOCK_1");
	engine->getResourceManager()->loadImage("ic_music_off_48dp.png",			"OSU_UI_VOLUME_SLIDER_MUSIC_0");
	engine->getResourceManager()->loadImage("ic_music_48dp.png",				"OSU_UI_VOLUME_SLIDER_MUSIC_1");
	engine->getResourceManager()->loadImage("ic_effects_off_48dp.png",			"OSU_UI_VOLUME_SLIDER_EFFECTS_0");
	engine->getResourceManager()->loadImage("ic_effects_48dp.png",				"OSU_UI_VOLUME_SLIDER_EFFECTS_1");

	setFrameColor(0xff7f7f7f);
}

void OsuUIVolumeSlider::drawBlock(Graphics *g)
{
	// draw icon
	Image *img = NULL;
	if (getFloat() < 0.01f)
		img = engine->getResourceManager()->getImage(m_type == TYPE::MASTER ? "OSU_UI_VOLUME_SLIDER_BLOCK_0" : (m_type == TYPE::MUSIC ? "OSU_UI_VOLUME_SLIDER_MUSIC_0" : "OSU_UI_VOLUME_SLIDER_EFFECTS_0"));
	else
		img = engine->getResourceManager()->getImage(m_type == TYPE::MASTER ? "OSU_UI_VOLUME_SLIDER_BLOCK_1" : (m_type == TYPE::MUSIC ? "OSU_UI_VOLUME_SLIDER_MUSIC_1" : "OSU_UI_VOLUME_SLIDER_EFFECTS_1"));

	g->pushTransform();
	{
		const float scaleMultiplier = 0.95f + 0.2f*m_fSelectionAnim;

		g->scale((m_vBlockSize.y / img->getSize().x)*scaleMultiplier, (m_vBlockSize.y / img->getSize().y)*scaleMultiplier);
		g->translate(m_vPos.x + m_vBlockPos.x + m_vBlockSize.x/2.0f, m_vPos.y + m_vBlockPos.y + m_vBlockSize.y/2.0f + 1);
		g->setColor(0xffffffff);
		g->drawImage(img);
	}
	g->popTransform();

	// draw percentage
	McFont *font = engine->getResourceManager()->getFont("FONT_DEFAULT");
	g->pushTransform();
	{
		g->translate((int)(m_vPos.x + m_vSize.x + 15), (int)(m_vPos.y + m_vSize.y/2 + font->getHeight()/2));
		g->setColor(0xff000000);
		g->translate(1, 1);
		g->drawString(font, UString::format("%i%%", (int)(std::round(getFloat()*100.0f))));
		g->setColor(0xffffffff);
		g->translate(-1, -1);
		g->drawString(font, UString::format("%i%%", (int)(std::round(getFloat()*100.0f))));
	}
	g->popTransform();
}

void OsuUIVolumeSlider::onMouseInside()
{
	CBaseUISlider::onMouseInside();

	m_bWentMouseInside = true;
}

void OsuUIVolumeSlider::setSelected(bool selected)
{
	m_bSelected = selected;

	if (m_bSelected)
	{
		setFrameColor(0xffffffff);
		anim->moveQuadOut(&m_fSelectionAnim, 1.0f, 0.1f, true);
		anim->moveQuadIn(&m_fSelectionAnim, 0.0f, 0.15f, 0.1f, false);
	}
	else
	{
		setFrameColor(0xff7f7f7f);
		anim->moveLinear(&m_fSelectionAnim, 0.0f, 0.15f*m_fSelectionAnim, true);
	}
}

bool OsuUIVolumeSlider::checkWentMouseInside()
{
	const bool temp = m_bWentMouseInside;
	m_bWentMouseInside = false;
	return temp;
}
