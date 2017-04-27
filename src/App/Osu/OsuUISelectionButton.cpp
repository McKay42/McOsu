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

OsuUISelectionButton::OsuUISelectionButton(UString imageResourceName, float xPos, float yPos, float xSize, float ySize, UString name) : CBaseUIImageButton(imageResourceName, xPos, yPos, xSize, ySize, name)
{
	m_fAnimation = 0.0f;
}

void OsuUISelectionButton::draw(Graphics *g)
{
	CBaseUIImageButton::draw(g);

	// draw over image
	if (m_fAnimation > 0.0f)
	{
		Image *image = engine->getResourceManager()->getImage(m_sImageOver);
		if (image != NULL)
		{
			g->setColor(0xffffffff);
			g->setAlpha(m_fAnimation);
			g->pushTransform();

				// scale
				float scale = (float)m_vSize.y / (float)image->getHeight(); // HACKHACK: force scale
				g->scale(scale, scale);

				// rotate
				if (m_fRot != 0.0f)
					g->rotate(m_fRot);

				// center and draw
				g->translate(m_vPos.x + (int)(m_vSize.x/2), m_vPos.y + (int)(m_vSize.y/2));
				g->drawImage(image);

			g->popTransform();
		}
	}
}

void OsuUISelectionButton::onMouseInside()
{
	CBaseUIImageButton::onMouseInside();

	anim->moveLinear(&m_fAnimation, 1.0f, 0.1f, true);
}

void OsuUISelectionButton::onMouseOutside()
{
	CBaseUIImageButton::onMouseOutside();

	anim->moveLinear(&m_fAnimation, 0.0f, m_fAnimation*0.1f, true);
}

void OsuUISelectionButton::keyboardPulse()
{
	if (isMouseInside())
		return;

	m_fAnimation = 1.0f;
	anim->moveLinear(&m_fAnimation, 0.0f, 0.1f, 0.05f, true);
}

