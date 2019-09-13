//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		generic slider (mod overrides, options, etc.)
//
// $NoKeywords: $osusl
//===============================================================================//

#include "OsuUISlider.h"

#include "ResourceManager.h"
#include "AnimationHandler.h"

#include "Osu.h"
#include "OsuSkin.h"

OsuUISlider::OsuUISlider(Osu *osu, float xPos, float yPos, float xSize, float ySize, UString name) : CBaseUISlider(xPos, yPos, xSize, ySize, name)
{
	m_osu = osu;
	setBlockSize(20, 20);
}

void OsuUISlider::draw(Graphics *g)
{
	if (!m_bVisible) return;

	Image *img = m_osu->getSkin()->getCircleEmpty();
	if (img == NULL)
	{
		CBaseUISlider::draw(g);
		return;
	}

	int lineAdd = 1;

	float line1Start = m_vPos.x + (m_vBlockSize.x-1)/2 + 1;
	float line1End = m_vPos.x + m_vBlockPos.x + lineAdd;
	float line2Start = m_vPos.x + m_vBlockPos.x + m_vBlockSize.x - lineAdd;
	float line2End = m_vPos.x + m_vSize.x - (m_vBlockSize.x-1)/2;

	// draw sliding line
	g->setColor(m_frameColor);
	if (line1End > line1Start)
		g->drawLine((int)(line1Start), (int)(m_vPos.y + m_vSize.y/2.0f + 1), (int)(line1End), (int)(m_vPos.y + m_vSize.y/2.0f + 1));
	if (line2End > line2Start)
		g->drawLine((int)(line2Start), (int)(m_vPos.y + m_vSize.y/2.0f + 1), (int)(line2End), (int)(m_vPos.y + m_vSize.y/2.0f + 1));

	// draw sliding block
	Vector2 blockCenter = m_vPos + m_vBlockPos + m_vBlockSize/2;
	Vector2 scale = Vector2(m_vBlockSize.x/img->getWidth(), m_vBlockSize.y/img->getHeight());

	g->setColor(m_frameColor);
	g->pushTransform();
	{
		g->scale(scale.x, scale.y);
		g->translate(blockCenter.x, blockCenter.y + 1);
		g->drawImage(m_osu->getSkin()->getCircleEmpty());
	}
	g->popTransform();
}
