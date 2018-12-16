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

OsuUISelectionButton::OsuUISelectionButton(std::function<Image*()> getImageFunc, std::function<Image*()> getImageOverFunc, float xPos, float yPos, float xSize, float ySize, UString name) : CBaseUIButton(xPos, yPos, xSize, ySize, name)
{
	this->getImageFunc = getImageFunc;
	this->getImageOverFunc = getImageOverFunc;

	m_fAnimation = 0.0f;

	m_vScale = Vector2(1, 1);
	m_bScaleToFit = true;
	m_bKeepAspectRatio = true;
}

void OsuUISelectionButton::draw(Graphics *g)
{
	if (!m_bVisible) return;

	// draw image
	Image *image = getImageFunc();
	if (image != NULL)
	{
		g->setColor(0xffffffff);
		g->pushTransform();

			// scale
			g->scale(m_vScale.x, m_vScale.y);

			// center and draw
			g->translate(m_vPos.x + (int)(m_vSize.x/2), m_vPos.y + (int)(m_vSize.y/2));
			g->drawImage(image);

		g->popTransform();
	}

	// draw over image
	if (m_fAnimation > 0.0f)
	{
		Image *overImage = getImageOverFunc();
		if (overImage != NULL)
		{
			g->setColor(0xffffffff);
			g->setAlpha(m_fAnimation);
			g->pushTransform();
			{
				// scale
				float scale = (float)m_vSize.y / (float)overImage->getHeight(); // HACKHACK: force scale
				g->scale(scale, scale);

				// center and draw
				g->translate(m_vPos.x + (int)(m_vSize.x/2), m_vPos.y + (int)(m_vSize.y/2));
				g->drawImage(overImage);
			}
			g->popTransform();
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

	Image *image = getImageFunc();
	if (m_bScaleToFit && image != NULL)
	{
		if (!m_bKeepAspectRatio)
		{
			m_vScale = Vector2(m_vSize.x/image->getWidth(), m_vSize.y/image->getHeight());
			m_vSize.x = (int)(image->getWidth()*m_vScale.x);
			m_vSize.y = (int)(image->getHeight()*m_vScale.y);
		}
		else
		{
			float scaleFactor = m_vSize.x/image->getWidth() < m_vSize.y/image->getHeight() ? m_vSize.x/image->getWidth() : m_vSize.y/image->getHeight();
			m_vScale = Vector2(scaleFactor, scaleFactor);
			m_vSize.x = (int)(image->getWidth()*m_vScale.x);
			m_vSize.y = (int)(image->getHeight()*m_vScale.y);
		}
	}
}

void OsuUISelectionButton::keyboardPulse()
{
	if (isMouseInside()) return;

	m_fAnimation = 1.0f;
	anim->moveLinear(&m_fAnimation, 0.0f, 0.1f, 0.05f, true);
}

