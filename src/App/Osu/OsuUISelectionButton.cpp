//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		song browser selection button (mode, mods, random, etc)
//
// $NoKeywords: $
//===============================================================================//

#include "OsuUISelectionButton.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"

#include "OsuSkinImage.h"

OsuUISelectionButton::OsuUISelectionButton(std::function<OsuSkinImage*()> getImageFunc, std::function<OsuSkinImage*()> getImageOverFunc, float xPos, float yPos, float xSize, float ySize, UString name) : CBaseUIButton(xPos, yPos, xSize, ySize, name)
{
	this->getImageFunc = getImageFunc;
	this->getImageOverFunc = getImageOverFunc;

	m_fAnimation = 0.0f;
}

void OsuUISelectionButton::draw(Graphics *g)
{
	if (!m_bVisible) return;

	// draw image
	OsuSkinImage *image = getImageFunc();
	if (image != NULL)
	{
		const Vector2 imageSize = image->getImageSizeForCurrentFrame();

		const float scale = (m_vSize.x/imageSize.x < m_vSize.y/imageSize.y ? m_vSize.x/imageSize.x : m_vSize.y/imageSize.y);

		g->setColor(0xffffffff);
		image->drawRaw(g, Vector2(m_vPos.x + (int)(m_vSize.x/2), m_vPos.y + (int)(m_vSize.y/2)), scale);
	}

	// draw over image
	if (m_fAnimation > 0.0f)
	{
		OsuSkinImage *overImage = getImageOverFunc();
		if (overImage != NULL)
		{
			const Vector2 imageSize = overImage->getImageSizeForCurrentFrame();

			const float scale = (m_vSize.x/imageSize.x < m_vSize.y/imageSize.y ? m_vSize.x/imageSize.x : m_vSize.y/imageSize.y);

			g->setColor(0xffffffff);
			g->setAlpha(m_fAnimation);

			overImage->drawRaw(g, Vector2(m_vPos.x + (int)(m_vSize.x/2), m_vPos.y + (int)(m_vSize.y/2)), scale);
		}
	}

	CBaseUIButton::drawText(g);
}

void OsuUISelectionButton::onMouseInside()
{
	CBaseUIButton::onMouseInside();

	anim->moveLinear(&m_fAnimation, 1.0f, 0.1f, true);
}

void OsuUISelectionButton::onMouseOutside()
{
	CBaseUIButton::onMouseOutside();

	anim->moveLinear(&m_fAnimation, 0.0f, m_fAnimation*0.1f, true);
}

void OsuUISelectionButton::onResized()
{
	CBaseUIButton::onResized();

	// NOTE: we get the height set to the current bottombarheight, so use that with the aspect ratio to get the correct relative "hardcoded" width

	OsuSkinImage *image = getImageFunc();
	if (image != NULL)
	{
		float aspectRatio = image->getSizeBaseRaw().x / image->getSizeBaseRaw().y;
		aspectRatio += 0.025f; // very slightly overscale to make most skins fit better with the bottombar blue line
		m_vSize.x = aspectRatio * m_vSize.y;
	}
}

void OsuUISelectionButton::keyboardPulse()
{
	if (isMouseInside()) return;

	m_fAnimation = 1.0f;
	anim->moveLinear(&m_fAnimation, 0.0f, 0.1f, 0.05f, true);
}

