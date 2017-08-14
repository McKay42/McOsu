//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		single note
//
// $NoKeywords: $osumanian
//===============================================================================//

#include "OsuManiaNote.h"

#include "Engine.h"
#include "ConVar.h"
#include "ResourceManager.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuGameRulesMania.h"
#include "OsuBeatmapMania.h"

ConVar osu_mania_note_height("osu_mania_note_height", 20.0f);
ConVar osu_mania_speed("osu_mania_speed", 1.0f);

OsuManiaNote::OsuManiaNote(int column, long sliderTime, long time, int sampleType, int comboNumber, int colorCounter, OsuBeatmapMania *beatmap) : OsuHitObject(time, sampleType, comboNumber, colorCounter, beatmap)
{
	m_iColumn = column;
	m_beatmap = beatmap;

	m_iObjectDuration = sliderTime;
	m_iObjectDuration = m_iObjectDuration >= 0 ? m_iObjectDuration : 0; // force clamp to positive range

	m_bStartFinished = !isHoldNote();
	m_startResult = OsuScore::HIT::HIT_NULL;
	m_endResult = OsuScore::HIT::HIT_NULL;
}

void OsuManiaNote::draw(Graphics *g)
{
	OsuHitObject::draw(g);

	if (!isHoldNote())
	{
		if (m_bFinished)
			return;
	}
	else
	{
		// instantly hide "perfectly" finished hold notes
		if (m_bFinished && m_startResult != OsuScore::HIT::HIT_MISS && m_endResult != OsuScore::HIT::HIT_MISS)
			return;

		// don't draw hold notes after they're un-clickable
		if (m_iDelta < 0 && (std::abs(m_iDelta) > m_iObjectDuration + (long)OsuGameRulesMania::getHitWindow50(m_beatmap)))
			return;
	}

	const float columnWidth = m_beatmap->getPlayfieldSize().x / m_beatmap->getNumColumns();

	const double invSpeedMultiplier = 1.0f / m_beatmap->getOsu()->getSpeedMultiplier();
	int height = osu_mania_note_height.getInt();
	int xPos = m_beatmap->getPlayfieldCenter().x - m_beatmap->getPlayfieldSize().x/2 + m_iColumn*(int)(columnWidth);
	float yPos = -m_iDelta*(double)osu_mania_speed.getFloat()*invSpeedMultiplier + (m_beatmap->getPlayfieldCenter().y + m_beatmap->getPlayfieldSize().y/2) - height + 2;

	// HACKHACK: hardcoded 10k offset
	if (m_beatmap->getNumColumns() == 10 && m_iColumn > 4)
		xPos += columnWidth;

	if (yPos > -100)
	{
		const Color colorWhite = 0xffffffff;
		const Color colorBlue = 0xff408CFF;
		const Color colorGold = 0xffFFD700;

		Color color = 0xffffffff;
		switch (m_beatmap->getNumColumns())
		{
		case 3:
			if (m_iColumn == 1)
				color = colorGold;
			else
				color = colorWhite;
			break;
		case 4:
			if (m_iColumn == 1 || m_iColumn == 2)
				color = colorBlue;
			else
				color = colorWhite;
			break;
		case 5:
			if (m_iColumn == 2)
				color = colorGold;
			else if (m_iColumn == 1 || m_iColumn == 3)
				color = colorBlue;
			else
				color = colorWhite;
			break;
		case 6:
			if (m_iColumn == 1 || m_iColumn == 4)
				color = colorBlue;
			else
				color = colorWhite;
			break;
		case 7:
			if (m_iColumn == 3)
				color = colorGold;
			else if (m_iColumn == 1 || m_iColumn == 5)
				color = colorBlue;
			else
				color = colorWhite;
			break;
		case 8:
			if (m_iColumn == 0)
				color = colorGold;
			else if (m_iColumn % 2 == 0)
				color = colorBlue;
			else
				color = colorWhite;
			break;
		case 9:
			if (m_iColumn == 4)
				color = colorGold;
			else if (m_iColumn % 2 != 0)
				color = colorBlue;
			else
				color = colorWhite;
			break;
		case 10:
			if (m_iColumn == 2 || m_iColumn == 7)
				color = colorGold;
			else if (m_iColumn == 1 || m_iColumn == 3 || m_iColumn == 6 || m_iColumn == 8)
				color = colorBlue;
			else
				color = colorWhite;
			break;
		}
		g->setColor(color);

		if (m_endResult == OsuScore::HIT::HIT_MISS) // key was released too early, can't recover this hold note
			g->setAlpha(0.4f);

		// hold notes: end note + body
		if (isHoldNote())
		{
			// TODO: the body shrinks until the key is accidentally released for the first time, then it stops at the current pixelLength and becomes dark
			bool isStartResultMiss = m_startResult == OsuScore::HIT::HIT_MISS;
			float pixelLength = (m_iObjectDuration + (m_iDelta < 0 && m_bStartFinished && !isStartResultMiss ? m_iDelta : 0))*(double)osu_mania_speed.getFloat()*invSpeedMultiplier;
			int yPosBody = yPos + (m_iDelta < 0 && m_bStartFinished && !isStartResultMiss ? m_iDelta : 0)*(double)osu_mania_speed.getFloat()*invSpeedMultiplier;

			// body
			if (pixelLength > 0.0f)
			{
				int inset = columnWidth*0.2f;
				g->fillRect(xPos + inset, yPosBody - pixelLength + height/2, columnWidth - 2*inset, pixelLength);
			}

			// end note
			g->fillRect(xPos, yPosBody - pixelLength, columnWidth, height);

			if (!isStartResultMiss && m_iDelta > -m_iObjectDuration)
				yPos = yPosBody;
		}

		// start note
		g->fillRect(xPos, yPos-1, columnWidth, height);

		// debug
		/*
		if (isHoldNote())
		{
			g->pushTransform();
			g->translate(xPos + columnWidth + 10, yPos);
			g->drawString(engine->getResourceManager()->getFont("FONT_DEFAULT"), UString::format("m_iDelta = %ld, m_iObjectDuration = %ld, compare = %ld", m_iDelta, m_iObjectDuration, m_iObjectDuration + (long)OsuGameRulesManiagetHitWindow50(m_beatmap)));
			g->popTransform();
		}
		*/
	}
}

void OsuManiaNote::update(long curPos)
{
	OsuHitObject::update(curPos);

	// if we have not been clicked yet, check if we are in the timeframe of a miss, also handle auto
	if (!m_bStartFinished || !m_bFinished)
	{
		if (m_beatmap->getOsu()->getModAuto())
		{
			if (curPos >= m_iTime + (!m_bStartFinished ? 0 : m_iObjectDuration))
				onHit(OsuScore::HIT::HIT_300, 0, !m_bStartFinished);
		}
		else
		{
			const long delta = curPos - (m_iTime + (!m_bStartFinished ? 0 : m_iObjectDuration));

			// if this is a miss after waiting
			if (delta >= 0)
			{
				if (delta > (long)OsuGameRulesMania::getHitWindow50(m_beatmap))
					onHit(OsuScore::HIT::HIT_MISS, delta, !m_bStartFinished, true);
			}
		}
	}
}

void OsuManiaNote::onClickEvent(std::vector<OsuBeatmap::CLICK> &clicks)
{
	if (m_bFinished)
		return;
	if (clicks[0].maniaColumn != m_iColumn)
		return;

	if (!m_bStartFinished || (!m_bFinished && !isHoldNote()))
	{
		const long delta = clicks[0].musicPos - m_iTime;

		OsuScore::HIT result = OsuGameRulesMania::getHitResult(delta, m_beatmap);
		if (result != OsuScore::HIT::HIT_NULL)
		{
			m_beatmap->consumeClickEvent();
			onHit(result, delta, !m_bStartFinished);
		}
	}
}

void OsuManiaNote::onKeyUpEvent(std::vector<OsuBeatmap::CLICK> &keyUps)
{
	if (!isHoldNote() || m_bFinished)
		return;
	if (keyUps[0].maniaColumn != m_iColumn)
		return;

	if (!m_bFinished && m_bStartFinished)
	{
		const long delta = keyUps[0].musicPos - (m_iTime + m_iObjectDuration);

		OsuScore::HIT result = OsuGameRulesMania::getHitResult(delta, m_beatmap);

		// if we released extremely early, so early that we're not even within miss-range, it should still count as a miss
		if (result == OsuScore::HIT::HIT_NULL)
			result = OsuScore::HIT::HIT_MISS;

		m_beatmap->consumeKeyUpEvent();
		onHit(result, delta, false);
	}
}

void OsuManiaNote::onReset(long curPos)
{
	OsuHitObject::onReset(curPos);

	m_startResult = OsuScore::HIT::HIT_NULL;
	m_endResult = OsuScore::HIT::HIT_NULL;

	if (m_iTime > curPos)
	{
		m_bFinished = false;
		m_bStartFinished = !isHoldNote();
	}
	else
	{
		m_bStartFinished = true;

		if (curPos > m_iTime + m_iObjectDuration)
			m_bFinished = true;
	}
}

void OsuManiaNote::onHit(OsuScore::HIT result, long delta, bool start, bool ignoreOnHitErrorBar)
{
	// sound and hit animation
	if (result != OsuScore::HIT::HIT_MISS && (start || !isHoldNote()))
	{
		if (m_osu_timingpoints_force->getBool())
			m_beatmap->updateTimingPoints(start ? m_iTime : m_iTime + m_iObjectDuration);

		m_beatmap->getSkin()->playHitCircleSound(m_iSampleType);
	}

	// add it, and we are finished
	Vector2 hitResultPos = m_beatmap->getPlayfieldCenter() + Vector2(0, m_beatmap->getPlayfieldSize().y*0.2f);
	addHitResult(result, delta, hitResultPos, 0.0f, 0.0f, ignoreOnHitErrorBar);

	if (!start || !isHoldNote())
	{
		m_endResult = result;
		m_bFinished = true;
	}
	else
	{
		m_startResult = result;
		m_bStartFinished = true;
	}
}
