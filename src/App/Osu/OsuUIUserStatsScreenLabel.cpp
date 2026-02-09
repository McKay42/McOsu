//================ Copyright (c) 2022, PG, All rights reserved. =================//
//
// Purpose:		currently only used for showing the pp/star algorithm versions
//
// $NoKeywords: $
//===============================================================================//

#include "OsuUIUserStatsScreenLabel.h"

#include "Osu.h"
#include "OsuTooltipOverlay.h"

OsuUIUserStatsScreenLabel::OsuUIUserStatsScreenLabel(Osu *osu, float xPos, float yPos, float xSize, float ySize, UString name, UString text) : CBaseUILabel()
{
	m_osu = osu;
}

void OsuUIUserStatsScreenLabel::update()
{
	CBaseUILabel::update();
	if (!m_bVisible) return;

	if (isMouseInside())
	{
		bool isEmpty = true;
		for (size_t i=0; i<m_tooltipTextLines.size(); i++)
		{
			if (m_tooltipTextLines[i].lengthUtf8() > 0)
			{
				isEmpty = false;
				break;
			}
		}

		if (!isEmpty)
		{
			m_osu->getTooltipOverlay()->begin();
			{
				for (size_t i=0; i<m_tooltipTextLines.size(); i++)
				{
					if (m_tooltipTextLines[i].lengthUtf8() > 0)
						m_osu->getTooltipOverlay()->addLine(m_tooltipTextLines[i]);
				}
			}
			m_osu->getTooltipOverlay()->end();
		}
	}
}
