//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		generic checkbox
//
// $NoKeywords: $osucb
//===============================================================================//

#include "OsuUICheckbox.h"

#include "Engine.h"

#include "Osu.h"
#include "OsuTooltipOverlay.h"

OsuUICheckbox::OsuUICheckbox(Osu *osu, float xPos, float yPos, float xSize, float ySize, UString name, UString text) : CBaseUICheckbox(xPos, yPos, xSize, ySize, name, text)
{
	m_osu = osu;

	m_bFocusStolenDelay = false;
}

void OsuUICheckbox::update()
{
	CBaseUICheckbox::update();
	if (!m_bVisible) return;

	if (isMouseInside() && m_tooltipTextLines.size() > 0 && !m_bFocusStolenDelay)
	{
		m_osu->getTooltipOverlay()->begin();
		for (int i=0; i<m_tooltipTextLines.size(); i++)
		{
			m_osu->getTooltipOverlay()->addLine(m_tooltipTextLines[i]);
		}
		m_osu->getTooltipOverlay()->end();
	}

	m_bFocusStolenDelay = false;
}

void OsuUICheckbox::onFocusStolen()
{
	CBaseUICheckbox::onFocusStolen();

	m_bMouseInside = false;
	m_bFocusStolenDelay = true;
}

void OsuUICheckbox::setTooltipText(UString text)
{
	m_tooltipTextLines = text.split("\n");
}
