//================ Copyright (c) 2020, PG, All rights reserved. =================//
//
// Purpose:		loader + container for raw beatmap files/data (v2 rewrite)
//
// $NoKeywords: $osudiff
//===============================================================================//

#include "OsuDatabaseBeatmap.h"

#include "Engine.h"
#include "ConVar.h"
#include "File.h"
#include "MD5.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuBeatmap.h"
#include "OsuBeatmapStandard.h"
#include "OsuBeatmapMania.h"
#include "OsuGameRules.h"
#include "OsuNotificationOverlay.h"

#include "OsuHitObject.h"
#include "OsuCircle.h"
#include "OsuSlider.h"
#include "OsuSliderCurves.h"
#include "OsuSpinner.h"
#include "OsuManiaNote.h"

unsigned long long OsuDatabaseBeatmap::sortHackCounter = 0;

ConVar *OsuDatabaseBeatmap::m_osu_slider_curve_max_length_ref = NULL;
ConVar *OsuDatabaseBeatmap::m_osu_show_approach_circle_on_first_hidden_object_ref = NULL;
ConVar *OsuDatabaseBeatmap::m_osu_stars_xexxar_angles_sliders_ref = NULL;
ConVar *OsuDatabaseBeatmap::m_osu_stars_stacking_ref = NULL;
ConVar *OsuDatabaseBeatmap::m_osu_slider_end_inside_check_offset_ref = NULL;
ConVar *OsuDatabaseBeatmap::m_osu_mod_random_ref = NULL;
ConVar *OsuDatabaseBeatmap::m_osu_mod_random_circle_offset_x_percent_ref = NULL;
ConVar *OsuDatabaseBeatmap::m_osu_mod_random_circle_offset_y_percent_ref = NULL;
ConVar *OsuDatabaseBeatmap::m_osu_mod_random_slider_offset_x_percent_ref = NULL;
ConVar *OsuDatabaseBeatmap::m_osu_mod_random_slider_offset_y_percent_ref = NULL;
ConVar *OsuDatabaseBeatmap::m_osu_mod_reverse_sliders_ref = NULL;
ConVar *OsuDatabaseBeatmap::m_osu_mod_random_spinner_offset_x_percent_ref = NULL;
ConVar *OsuDatabaseBeatmap::m_osu_mod_random_spinner_offset_y_percent_ref = NULL;

OsuDatabaseBeatmap::OsuDatabaseBeatmap(Osu *osu, UString filePath, UString folder) : Resource()
{
	m_osu = osu;

	m_sFilePath = filePath;

	m_sFolder = folder;

	m_iSortHack = sortHackCounter++;

	m_iLoadType = 0;
	m_iLoadErrorCode = 0;
	m_loadBeatmap = NULL;



	// convar refs
	if (m_osu_slider_curve_max_length_ref == NULL)
		m_osu_slider_curve_max_length_ref = convar->getConVarByName("osu_slider_curve_max_length");
	if (m_osu_show_approach_circle_on_first_hidden_object_ref == NULL)
		m_osu_show_approach_circle_on_first_hidden_object_ref = convar->getConVarByName("osu_show_approach_circle_on_first_hidden_object");
	if (m_osu_stars_xexxar_angles_sliders_ref == NULL)
		m_osu_stars_xexxar_angles_sliders_ref = convar->getConVarByName("osu_stars_xexxar_angles_sliders");
	if (m_osu_stars_stacking_ref == NULL)
		m_osu_stars_stacking_ref = convar->getConVarByName("osu_stars_stacking");
	if (m_osu_slider_end_inside_check_offset_ref == NULL)
		m_osu_slider_end_inside_check_offset_ref = convar->getConVarByName("osu_slider_end_inside_check_offset");
	if (m_osu_mod_random_ref == NULL)
		m_osu_mod_random_ref = convar->getConVarByName("osu_mod_random");
	if (m_osu_mod_random_circle_offset_x_percent_ref == NULL)
		m_osu_mod_random_circle_offset_x_percent_ref = convar->getConVarByName("osu_mod_random_circle_offset_x_percent");
	if (m_osu_mod_random_circle_offset_y_percent_ref == NULL)
		m_osu_mod_random_circle_offset_y_percent_ref = convar->getConVarByName("osu_mod_random_circle_offset_y_percent");
	if (m_osu_mod_random_slider_offset_x_percent_ref == NULL)
		m_osu_mod_random_slider_offset_x_percent_ref = convar->getConVarByName("osu_mod_random_slider_offset_x_percent");
	if (m_osu_mod_random_slider_offset_y_percent_ref == NULL)
		m_osu_mod_random_slider_offset_y_percent_ref = convar->getConVarByName("osu_mod_random_slider_offset_y_percent");
	if (m_osu_mod_reverse_sliders_ref == NULL)
		m_osu_mod_reverse_sliders_ref = convar->getConVarByName("osu_mod_reverse_sliders");
	if (m_osu_mod_random_spinner_offset_x_percent_ref == NULL)
		m_osu_mod_random_spinner_offset_x_percent_ref = convar->getConVarByName("osu_mod_random_spinner_offset_x_percent");
	if (m_osu_mod_random_spinner_offset_y_percent_ref == NULL)
		m_osu_mod_random_spinner_offset_y_percent_ref = convar->getConVarByName("osu_mod_random_spinner_offset_y_percent");



	// raw metadata

	m_iVersion = 14;
	m_iGameMode = 0;
	m_iID = 0;
	m_iSetID = -1;

	m_iLengthMS = 0;
	m_iPreviewTime = 0;

	m_fAR = 5.0f;
	m_fCS = 5.0f;
	m_fHP = 5.0f;
	m_fOD = 5.0f;

	m_fStackLeniency = 0.7f;
	m_fSliderTickRate = 1.0f;
	m_fSliderMultiplier = 1.0f;



	// precomputed data

	m_fStarsNomod = 0.0f;

	m_iMinBPM = 0;
	m_iMaxBPM = 0;

	m_iNumObjects = 0;
	m_iNumCircles = 0;
	m_iNumSliders = 0;



	// custom data

	m_iLastModificationTime = (std::numeric_limits<long long>::max()/2) + m_iSortHack;

	m_iLocalOffset = 0;
	m_iOnlineOffset = 0;
}

OsuDatabaseBeatmap::OsuDatabaseBeatmap(Osu *osu, std::vector<OsuDatabaseBeatmap*> &difficulties) : OsuDatabaseBeatmap(osu, "", "")
{
	setDifficulties(difficulties);
}

OsuDatabaseBeatmap::~OsuDatabaseBeatmap()
{
	for (size_t i=0; i<m_difficulties.size(); i++)
	{
		SAFE_DELETE(m_difficulties[i]);
	}
}

std::vector<std::shared_ptr<OsuDifficultyHitObject>> OsuDatabaseBeatmap::generateDifficultyHitObjects(const UString &osuFilePath, Osu::GAMEMODE gameMode, float AR, float CS, int version, float stackLeniency, float speedMultiplier, bool calculateStarsInaccurately)
{
	// build generalized OsuDifficultyHitObjects from the vectors (hitcircles, sliders, spinners)
	// the OsuDifficultyHitObject class is the one getting used in all pp/star calculations, it encompasses every object type for simplicity

	// load primitive arrays
	const PRIMITIVE_CONTAINER c = loadPrimitiveObjects(osuFilePath, gameMode);

	// TODO: if calculating accurately, must calculate scoringTimesForStarCalc etc.!
	// TODO: make that shit static too

	// and generate the difficultyhitobjects
	std::vector<std::shared_ptr<OsuDifficultyHitObject>> diffHitObjects;
	diffHitObjects.reserve(c.hitcircles.size() + c.sliders.size() + c.spinners.size());

	for (int i=0; i<c.hitcircles.size(); i++)
	{
		diffHitObjects.push_back(std::make_shared<OsuDifficultyHitObject>(
				OsuDifficultyHitObject::TYPE::CIRCLE,
				Vector2(c.hitcircles[i].x, c.hitcircles[i].y),
				(long)c.hitcircles[i].time));
	}

	for (int i=0; i<c.sliders.size(); i++)
	{
		if (!calculateStarsInaccurately)
		{
			diffHitObjects.push_back(std::make_shared<OsuDifficultyHitObject>(
					OsuDifficultyHitObject::TYPE::SLIDER,
					Vector2(c.sliders[i].x, c.sliders[i].y),
					c.sliders[i].time,
					c.sliders[i].time + (long)c.sliders[i].sliderTime,
					c.sliders[i].sliderTimeWithoutRepeats,
					c.sliders[i].type,
					c.sliders[i].points,
					c.sliders[i].pixelLength,
					c.sliders[i].scoringTimesForStarCalc));
		}
		else
		{
			diffHitObjects.push_back(std::make_shared<OsuDifficultyHitObject>(
					OsuDifficultyHitObject::TYPE::SLIDER,
					Vector2(c.sliders[i].x, c.sliders[i].y),
					c.sliders[i].time,
					c.sliders[i].time + (long)c.sliders[i].sliderTime,
					c.sliders[i].sliderTimeWithoutRepeats,
					c.sliders[i].type,
					std::vector<Vector2>(),	// NOTE: ignore curve when calculating inaccurately
					c.sliders[i].pixelLength,
					std::vector<long>()));	// NOTE: ignore curve when calculating inaccurately
		}
	}

	for (int i=0; i<c.spinners.size(); i++)
	{
		diffHitObjects.push_back(std::make_shared<OsuDifficultyHitObject>(
				OsuDifficultyHitObject::TYPE::SPINNER,
				Vector2(c.spinners[i].x, c.spinners[i].y),
				(long)c.spinners[i].time,
				(long)c.spinners[i].endTime));
	}

	// sort hitobjects by time
	struct DiffHitObjectSortComparator
	{
	    bool operator() (const std::shared_ptr<OsuDifficultyHitObject> &a, const std::shared_ptr<OsuDifficultyHitObject> &b) const
	    {
	    	// strict weak ordering!
	    	if (a->time == b->time)
	    		return a->sortHack < b->sortHack;
	    	else
	    		return a->time < b->time;
	    }
	};
	std::sort(diffHitObjects.begin(), diffHitObjects.end(), DiffHitObjectSortComparator());

	// calculate stacks
	// see OsuBeatmapStandard.cpp
	// NOTE: this must be done before the speed multiplier is applied!
	// HACKHACK: code duplication ffs
	if (m_osu_stars_stacking_ref->getBool() && !calculateStarsInaccurately) // NOTE: ignore stacking when calculating inaccurately
	{
		const float finalAR = AR;
		const float finalCS = CS;
		const float rawHitCircleDiameter = OsuGameRules::getRawHitCircleDiameter(finalCS);

		const float STACK_LENIENCE = 3.0f;
		const float STACK_OFFSET = 0.05f;

		const float approachTime = OsuGameRules::getApproachTimeForStacking(finalAR);

		if (version > 5)
		{
			// peppy's algorithm
			// https://gist.github.com/peppy/1167470

			for (int i=diffHitObjects.size()-1; i>=0; i--)
			{
				int n = i;

				std::shared_ptr<OsuDifficultyHitObject> objectI = diffHitObjects[i];

				const bool isSpinner = (objectI->type == OsuDifficultyHitObject::TYPE::SPINNER);

				if (objectI->stack != 0 || isSpinner)
					continue;

				const bool isHitCircle = (objectI->type == OsuDifficultyHitObject::TYPE::CIRCLE);
				const bool isSlider = (objectI->type == OsuDifficultyHitObject::TYPE::SLIDER);

				if (isHitCircle)
				{
					while (--n >= 0)
					{
						std::shared_ptr<OsuDifficultyHitObject> objectN = diffHitObjects[n];

						const bool isSpinnerN = (objectN->type == OsuDifficultyHitObject::TYPE::SPINNER);

						if (isSpinnerN)
							continue;

						if (objectI->time - (approachTime * stackLeniency) > (objectN->endTime))
							break;

						Vector2 objectNEndPosition = objectN->getOriginalRawPosAt(objectN->time + objectN->getDuration());
						if (objectN->getDuration() != 0 && (objectNEndPosition - objectI->getOriginalRawPosAt(objectI->time)).length() < STACK_LENIENCE)
						{
							int offset = objectI->stack - objectN->stack + 1;
							for (int j=n+1; j<=i; j++)
							{
								if ((objectNEndPosition - diffHitObjects[j]->getOriginalRawPosAt(diffHitObjects[j]->time)).length() < STACK_LENIENCE)
									diffHitObjects[j]->stack = (diffHitObjects[j]->stack - offset);
							}

							break;
						}

						if ((objectN->getOriginalRawPosAt(objectN->time) - objectI->getOriginalRawPosAt(objectI->time)).length() < STACK_LENIENCE)
						{
							objectN->stack = (objectI->stack + 1);
							objectI = objectN;
						}
					}
				}
				else if (isSlider)
				{
					while (--n >= 0)
					{
						std::shared_ptr<OsuDifficultyHitObject> objectN = diffHitObjects[n];

						const bool isSpinner = (objectN->type == OsuDifficultyHitObject::TYPE::SPINNER);

						if (isSpinner)
							continue;

						if (objectI->time - (approachTime * stackLeniency) > objectN->time)
							break;

						if (((objectN->getDuration() != 0 ? objectN->getOriginalRawPosAt(objectN->time + objectN->getDuration()) : objectN->getOriginalRawPosAt(objectN->time)) - objectI->getOriginalRawPosAt(objectI->time)).length() < STACK_LENIENCE)
						{
							objectN->stack = (objectI->stack + 1);
							objectI = objectN;
						}
					}
				}
			}
		}
		else // version < 6
		{
			// old stacking algorithm for old beatmaps
			// https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Beatmaps/OsuBeatmapProcessor.cs

			for (int i=0; i<diffHitObjects.size(); i++)
			{
				std::shared_ptr<OsuDifficultyHitObject> currHitObject = diffHitObjects[i];

				const bool isSlider = (currHitObject->type == OsuDifficultyHitObject::TYPE::SLIDER);

				if (currHitObject->stack != 0 && !isSlider)
					continue;

				long startTime = currHitObject->time + currHitObject->getDuration();
				int sliderStack = 0;

				for (int j=i+1; j<diffHitObjects.size(); j++)
				{
					std::shared_ptr<OsuDifficultyHitObject> objectJ = diffHitObjects[j];

					if (objectJ->time - (approachTime * stackLeniency) > startTime)
						break;

					// "The start position of the hitobject, or the position at the end of the path if the hitobject is a slider"
					Vector2 position2 = isSlider
						? currHitObject->getOriginalRawPosAt(currHitObject->time + currHitObject->getDuration())
						: currHitObject->getOriginalRawPosAt(currHitObject->time);

					if ((objectJ->getOriginalRawPosAt(objectJ->time) - currHitObject->getOriginalRawPosAt(currHitObject->time)).length() < 3)
					{
						currHitObject->stack++;
						startTime = objectJ->time + objectJ->getDuration();
					}
					else if ((objectJ->getOriginalRawPosAt(objectJ->time) - position2).length() < 3)
					{
						// "Case for sliders - bump notes down and right, rather than up and left."
						sliderStack++;
						objectJ->stack -= sliderStack;
						startTime = objectJ->time + objectJ->getDuration();
					}
				}
			}
		}

		// update hitobject positions
		float stackOffset = rawHitCircleDiameter * STACK_OFFSET;
		for (int i=0; i<diffHitObjects.size(); i++)
		{
			if (diffHitObjects[i]->stack != 0)
				diffHitObjects[i]->updateStackPosition(stackOffset);
		}
	}

	// apply speed multiplier from beatmap (if present)
	if (speedMultiplier != 1.0f && speedMultiplier > 0.0f)
	{
		const double invSpeedMultiplier = 1.0 / (double)speedMultiplier;
		for (int i=0; i<diffHitObjects.size(); i++)
		{
			diffHitObjects[i]->time = (long)((double)diffHitObjects[i]->time * invSpeedMultiplier);
			diffHitObjects[i]->endTime = (long)((double)diffHitObjects[i]->endTime * invSpeedMultiplier);

			if (!calculateStarsInaccurately) // NOTE: ignore slider curves when calculating inaccurately
			{
				diffHitObjects[i]->spanDuration = (double)diffHitObjects[i]->spanDuration * invSpeedMultiplier;
				for (int s=0; s<diffHitObjects[i]->scoringTimes.size(); s++)
				{
					diffHitObjects[i]->scoringTimes[s] = (long)((double)diffHitObjects[i]->scoringTimes[s] * invSpeedMultiplier);
				}
			}
		}
	}

	return diffHitObjects;
}

void OsuDatabaseBeatmap::setLoadBeatmapMetadata()
{
	m_iLoadType = 0;
}

void OsuDatabaseBeatmap::setLoadBeatmap(OsuBeatmap *beatmap)
{
	m_iLoadType = 1;
	m_loadBeatmap = beatmap;
}

void OsuDatabaseBeatmap::init()
{
	switch (m_iLoadErrorCode)
	{
	case 1:
		{
			UString errorMessage = "Error: Couldn't load beatmap metadata :(";
			debugLog("Osu Error: Couldn't load beatmap metadata %s\n", m_sFilePath.toUtf8());

			if (m_osu != NULL)
				m_osu->getNotificationOverlay()->addNotification(errorMessage, 0xffff0000);
		}
		break;

	case 2:
		{
			UString errorMessage = "Error: Couldn't load beatmap file :(";
			debugLog("Osu Error: Couldn't load beatmap file %s\n", m_sFilePath.toUtf8());

			if (m_osu != NULL)
				m_osu->getNotificationOverlay()->addNotification(errorMessage, 0xffff0000);
		}
		break;

	case 3:
		{
			UString errorMessage = "Error: No timingpoints in beatmap!";
			debugLog("Osu Error: No timingpoints in beatmap %s\n", m_sFilePath.toUtf8());

			if (m_osu != NULL)
				m_osu->getNotificationOverlay()->addNotification(errorMessage, 0xffff0000);
		}
		break;
	}

	if (m_iLoadErrorCode != 0 || !m_bAsyncReady) return;

	// TODO: set hitobjects vector on beatmap (use m_loadHitObjects!), or not, think about a better loading architecture

	// load beatmap skin
	m_osu->getSkin()->loadBeatmapOverride(m_sFolder);

	m_bReady = true;
}

void OsuDatabaseBeatmap::initAsync()
{
	switch (m_iLoadType)
	{
	case 0:
		m_bAsyncReady = loadBeatmapMetadataInt();
		break;
	case 1:
		m_bAsyncReady = loadBeatmapInt(m_loadBeatmap);
		break;
	}
}

void OsuDatabaseBeatmap::destroy()
{
	// (special case: this is not destroyed via destroy())
}

OsuDatabaseBeatmap::PRIMITIVE_CONTAINER OsuDatabaseBeatmap::loadPrimitiveObjects(const UString &osuFilePath, Osu::GAMEMODE gameMode)
{
	PRIMITIVE_CONTAINER c;
	{
		c.errorCode = 0;
	}

	// open osu file for parsing
	{
		File file(osuFilePath);
		if (!file.canRead())
		{
			c.errorCode = 2;
			return c;
		}

		// load the actual beatmap
		int hitobjectsWithoutSpinnerCounter = 0;
		int colorCounter = 1;
		int colorOffset = 0;
		int comboNumber = 1;
		int curBlock = -1;
		while (file.canRead())
		{
			UString uCurLine = file.readLine();
			const char *curLineChar = uCurLine.toUtf8();
			std::string curLine(curLineChar);

			const int commentIndex = curLine.find("//");
			if (commentIndex == std::string::npos || commentIndex != 0) // ignore comments, but only if at the beginning of a line (e.g. allow Artist:DJ'TEKINA//SOMETHING)
			{
				if (curLine.find("[Events]") != std::string::npos)
					curBlock = 1;
				else if (curLine.find("[Colours]") != std::string::npos)
					curBlock = 2;
				else if (curLine.find("[HitObjects]") != std::string::npos)
					curBlock = 3;

				switch (curBlock)
				{
				case 1: // Events
					{
						int type, startTime, endTime;
						if (sscanf(curLineChar, " %i , %i , %i \n", &type, &startTime, &endTime) == 3)
						{
							if (type == 2)
							{
								BREAK b;
								{
									b.startTime = startTime;
									b.endTime = endTime;
								}
								c.breaks.push_back(b);
							}
						}
					}
					break;

				case 2: // Colours
					{
						int comboNum;
						int r,g,b;
						if (sscanf(curLineChar, " Combo %i : %i , %i , %i \n", &comboNum, &r, &g, &b) == 4)
							c.combocolors.push_back(COLOR(255, r, g, b));
					}
					break;

				case 3: // HitObjects

					// circles:
					// x,y,time,type,hitSound,addition
					// sliders:
					// x,y,time,type,hitSound,sliderType|curveX:curveY|...,repeat,pixelLength,edgeHitsound,edgeAddition,addition
					// spinners:
					// x,y,time,type,hitSound,endTime,addition

					// NOTE: calculating combo numbers and color offsets based on the parsing order is dangerous.
					// maybe the hitobjects are not sorted by time in the file; these values should be calculated after sorting just to be sure?

					int x,y;
					long time;
					int type;
					int hitSound;

					const bool intScan = (sscanf(curLineChar, " %i , %i , %li , %i , %i", &x, &y, &time, &type, &hitSound) == 5);
					bool floatScan = false;
					if (!intScan)
					{
						float fX,fY;
						floatScan = (sscanf(curLineChar, " %f , %f , %li , %i , %i", &fX, &fY, &time, &type, &hitSound) == 5);
						x = (int)fX;
						y = (int)fY;
					}

					if (intScan || floatScan)
					{
						if (!(type & 0x8))
							hitobjectsWithoutSpinnerCounter++;

						if (type & 0x4) // new combo
						{
							comboNumber = 1;

							// special case 1: if the current object is a spinner, then the raw color counter is not increased (but the offset still is!)
							// special case 2: the first (non-spinner) hitobject in a beatmap is always a new combo, therefore the raw color counter is not increased for it (but the offset still is!)
							if (!(type & 0x8) && hitobjectsWithoutSpinnerCounter > 1)
								colorCounter++;

							colorOffset += (type >> 4) & 7; // special case 3: "Bits 4-6 (16, 32, 64) form a 3-bit number (0-7) that chooses how many combo colours to skip."
						}

						if (type & 0x1) // circle
						{
							HITCIRCLE h;
							{
								h.x = x;
								h.y = y;
								h.time = time;
								h.sampleType = hitSound;
								h.number = comboNumber++;
								h.colorCounter = colorCounter;
								h.colorOffset = colorOffset;
								h.clicked = false;
								h.maniaEndTime = 0;
							}
							c.hitcircles.push_back(h);
						}
						else if (type & 0x2) // slider
						{
							UString curLineString = UString(curLineChar);
							std::vector<UString> tokens = curLineString.split(",");
							if (tokens.size() < 8)
							{
								debugLog("Invalid slider in beatmap: %s\n\ncurLine = %s\n", osuFilePath.toUtf8(), curLineChar);
								continue;
								//engine->showMessageError("Error", UString::format("Invalid slider in beatmap: %s\n\ncurLine = %s", m_sFilePath.toUtf8(), curLine));
								//return false;
							}

							std::vector<UString> sliderTokens = tokens[5].split("|");
							if (sliderTokens.size() < 1) // partially allow bullshit sliders (no controlpoints!), e.g. https://osu.ppy.sh/beatmapsets/791900#osu/1676490
							{
								debugLog("Invalid slider tokens: %s\n\nIn beatmap: %s\n", curLineChar, osuFilePath.toUtf8());
								continue;
								//engine->showMessageError("Error", UString::format("Invalid slider tokens: %s\n\nIn beatmap: %s", curLineChar, m_sFilePath.toUtf8()));
								//return false;
							}

							const float sanityRange = m_osu_slider_curve_max_length_ref->getFloat(); // infinity sanity check, same as before
							std::vector<Vector2> points;
							for (int i=1; i<sliderTokens.size(); i++) // NOTE: starting at 1 due to slider type char
							{
								std::vector<UString> sliderXY = sliderTokens[i].split(":");

								// array size check
								// infinity sanity check (this only exists because of https://osu.ppy.sh/b/1029976)
								// not a very elegant check, but it does the job
								if (sliderXY.size() != 2 || sliderXY[0].find("E") != -1 || sliderXY[0].find("e") != -1 || sliderXY[1].find("E") != -1 || sliderXY[1].find("e") != -1)
								{
									debugLog("Invalid slider positions: %s\n\nIn Beatmap: %s\n", curLineChar, osuFilePath.toUtf8());
									continue;
									//engine->showMessageError("Error", UString::format("Invalid slider positions: %s\n\nIn beatmap: %s", curLine, m_sFilePath.toUtf8()));
									//return false;
								}

								points.push_back(Vector2((int)clamp<float>(sliderXY[0].toFloat(), -sanityRange, sanityRange), (int)clamp<float>(sliderXY[1].toFloat(), -sanityRange, sanityRange)));
							}

							// special case: osu! logic for handling the hitobject point vs the controlpoints (since sliders have both, and older beatmaps store the start point inside the control points)
							{
								const Vector2 xy = Vector2(clamp<float>(x, -sanityRange, sanityRange), clamp<float>(y, -sanityRange, sanityRange));
								if (points.size() > 0)
								{
				                    if (points[0] != xy)
				                    	points.insert(points.begin(), xy);
								}
								else
									points.push_back(xy);
							}

							// partially allow bullshit sliders (add second point to make valid), e.g. https://osu.ppy.sh/beatmapsets/791900#osu/1676490
							if (sliderTokens.size() < 2 && points.size() > 0)
								points.push_back(points[0]);

							SLIDER s;
							{
								s.x = x;
								s.y = y;
								s.type = sliderTokens[0][0];
								s.repeat = (int)tokens[6].toFloat();
								s.repeat = s.repeat >= 0 ? s.repeat : 0; // sanity check
								s.pixelLength = clamp<float>(tokens[7].toFloat(), -sanityRange, sanityRange);
								s.time = time;
								s.sampleType = hitSound;
								s.number = comboNumber++;
								s.colorCounter = colorCounter;
								s.colorOffset = colorOffset;
								s.points = points;

								// new beatmaps: slider hitsounds
								if (tokens.size() > 8)
								{
									std::vector<UString> hitSoundTokens = tokens[8].split("|");
									for (int i=0; i<hitSoundTokens.size(); i++)
									{
										s.hitSounds.push_back(hitSoundTokens[i].toInt());
									}
								}
							}
							c.sliders.push_back(s);
						}
						else if (type & 0x8) // spinner
						{
							UString curLineString = UString(curLineChar);
							std::vector<UString> tokens = curLineString.split(",");
							if (tokens.size() < 6)
							{
								debugLog("Invalid spinner in beatmap: %s\n\ncurLine = %s\n", osuFilePath.toUtf8(), curLineChar);
								continue;
								//engine->showMessageError("Error", UString::format("Invalid spinner in beatmap: %s\n\ncurLine = %s", m_sFilePath.toUtf8(), curLine));
								//return false;
							}

							SPINNER s;
							{
								s.x = x;
								s.y = y;
								s.time = time;
								s.sampleType = hitSound;
								s.endTime = tokens[5].toFloat();
							}
							c.spinners.push_back(s);
						}
						else if (gameMode == Osu::GAMEMODE::MANIA && (type & 0x80)) // osu!mania hold note, gamemode check for sanity
						{
							UString curLineString = UString(curLineChar);
							std::vector<UString> tokens = curLineString.split(",");

							if (tokens.size() < 6)
							{
								debugLog("Invalid hold note in beatmap: %s\n\ncurLine = %s\n", osuFilePath.toUtf8(), curLineChar);
								continue;
							}

							std::vector<UString> holdNoteTokens = tokens[5].split(":");
							if (holdNoteTokens.size() < 1)
							{
								debugLog("Invalid hold note in beatmap: %s\n\ncurLine = %s\n", osuFilePath.toUtf8(), curLineChar);
								continue;
							}

							HITCIRCLE h;
							{
								h.x = x;
								h.y = y;
								h.time = time;
								h.sampleType = hitSound;
								h.number = comboNumber++;
								h.colorCounter = colorCounter;
								h.colorOffset = colorOffset;
								h.clicked = false;
								h.maniaEndTime = holdNoteTokens[0].toLong();
							}
							c.hitcircles.push_back(h);
						}
					}
					break;
				}
			}
		}
	}

	return c;
}

bool OsuDatabaseBeatmap::loadBeatmapMetadataInt()
{
	if (m_difficulties.size() > 0) return false; // we are just a container

	// reset
	m_timingpoints = std::vector<TIMINGPOINT>();

	if (Osu::debug->getBool())
		debugLog("OsuDatabaseBeatmap::loadMetadata() : %s\n", m_sFilePath.toUtf8());

	// generate MD5 hash (loads entire file, very slow)
	m_sMD5Hash.clear();
	{
		File file(m_sFilePath);
		if (file.canRead())
		{
			const char *beatmapFile = file.readFile();
			if (beatmapFile != NULL)
			{
				const char hexDigits[17] = "0123456789abcdef";
				const unsigned char *input = (unsigned char*)beatmapFile;
				MD5 hasher;
				hasher.update(input, file.getFileSize());
				hasher.finalize();
				unsigned char *rawMD5Hash = hasher.getDigest();

				for (int i=0; i<16; i++)
				{
					m_sMD5Hash += hexDigits[(rawMD5Hash[i] >> 4) & 0xf];	// md5hash[i] / 16
					m_sMD5Hash += hexDigits[rawMD5Hash[i] & 0xf];			// md5hash[i] % 16
				}
			}
		}
	}

	// open osu file again, but this time for parsing
	bool foundAR = false;
	{
		File file(m_sFilePath);
		if (!file.canRead())
		{
			debugLog("Osu Error: Couldn't read file %s\n", m_sFilePath.toUtf8());
			return false;
		}

		// load metadata only
		int curBlock = -1;
		unsigned long long timingPointSortHack = 0;
		char stringBuffer[1024];
		while (file.canRead())
		{
			UString uCurLine = file.readLine();
			const char *curLineChar = uCurLine.toUtf8();
			std::string curLine(curLineChar);

			const int commentIndex = curLine.find("//");
			if (commentIndex == std::string::npos || commentIndex != 0) // ignore comments, but only if at the beginning of a line (e.g. allow Artist:DJ'TEKINA//SOMETHING)
			{
				if (curLine.find("[General]") != std::string::npos)
					curBlock = 0;
				else if (curLine.find("[Metadata]") != std::string::npos)
					curBlock = 1;
				else if (curLine.find("[Difficulty]") != std::string::npos)
					curBlock = 2;
				else if (curLine.find("[Events]") != std::string::npos)
					curBlock = 3;
				else if (curLine.find("[TimingPoints]") != std::string::npos)
					curBlock = 4;
				else if (curLine.find("[HitObjects]") != std::string::npos)
					break; // NOTE: stop early

				switch (curBlock)
				{
				case -1: // header (e.g. "osu file format v12")
					{
						sscanf(curLineChar, " osu file format v %i \n", &m_iVersion);
					}
					break;

				case 0: // General
					{
						memset(stringBuffer, '\0', 1024);
						if (sscanf(curLineChar, " AudioFilename : %1023[^\n]", stringBuffer) == 1)
						{
							m_sAudioFileName = UString(stringBuffer);
							m_sAudioFileName = m_sAudioFileName.trim();
						}

						sscanf(curLineChar, " StackLeniency : %f \n", &m_fStackLeniency);
						sscanf(curLineChar, " PreviewTime : %lu \n", &m_iPreviewTime);
						sscanf(curLineChar, " Mode : %i \n", &m_iGameMode);
					}
					break;

				case 1: // Metadata
					{
						memset(stringBuffer, '\0', 1024);
						if (sscanf(curLineChar, " Title :%1023[^\n]", stringBuffer) == 1)
						{
							m_sTitle = UString(stringBuffer);
							m_sTitle = m_sTitle.trim();
						}

						memset(stringBuffer, '\0', 1024);
						if (sscanf(curLineChar, " Artist :%1023[^\n]", stringBuffer) == 1)
						{
							m_sArtist = UString(stringBuffer);
							m_sArtist = m_sArtist.trim();
						}

						memset(stringBuffer, '\0', 1024);
						if (sscanf(curLineChar, " Creator :%1023[^\n]", stringBuffer) == 1)
						{
							m_sCreator = UString(stringBuffer);
							m_sCreator = m_sCreator.trim();
						}

						memset(stringBuffer, '\0', 1024);
						if (sscanf(curLineChar, " Version :%1023[^\n]", stringBuffer) == 1)
						{
							m_sDifficultyName = UString(stringBuffer);
							m_sDifficultyName = m_sDifficultyName.trim();
						}

						memset(stringBuffer, '\0', 1024);
						if (sscanf(curLineChar, " Source :%1023[^\n]", stringBuffer) == 1)
						{
							m_sSource = UString(stringBuffer);
							m_sSource = m_sSource.trim();
						}

						memset(stringBuffer, '\0', 1024);
						if (sscanf(curLineChar, " Tags :%1023[^\n]", stringBuffer) == 1)
						{
							m_sTags = UString(stringBuffer);
							m_sTags = m_sTags.trim();
						}

						sscanf(curLineChar, " BeatmapID : %ld \n", &m_iID);
						sscanf(curLineChar, " BeatmapSetID : %i \n", &m_iSetID);
					}
					break;

				case 2: // Difficulty
					{
						sscanf(curLineChar, " CircleSize : %f \n", &m_fCS);
						if (sscanf(curLineChar, " ApproachRate : %f \n", &m_fAR) == 1)
							foundAR = true;

						sscanf(curLineChar, " HPDrainRate : %f \n", &m_fHP);
						sscanf(curLineChar, " OverallDifficulty : %f \n", &m_fOD);
						sscanf(curLineChar, " SliderMultiplier : %f \n", &m_fSliderMultiplier);
						sscanf(curLineChar, " SliderTickRate : %f \n", &m_fSliderTickRate);
					}
					break;

				case 3: // Events
					{
						memset(stringBuffer, '\0', 1024);
						int type, startTime;
						if (sscanf(curLineChar, " %i , %i , \"%1023[^\"]\"", &type, &startTime, stringBuffer) == 3)
						{
							if (type == 0)
							{
								m_sBackgroundImageFileName = UString(stringBuffer);
								m_sFullBackgroundImageFilePath = m_sFolder;
								m_sFullBackgroundImageFilePath.append(m_sBackgroundImageFileName);
							}
						}
					}
					break;

				case 4: // TimingPoints
					{
						// old beatmaps: Offset, Milliseconds per Beat
						// old new beatmaps: Offset, Milliseconds per Beat, Meter, Sample Type, Sample Set, Volume, !Inherited
						// new new beatmaps: Offset, Milliseconds per Beat, Meter, Sample Type, Sample Set, Volume, !Inherited, Kiai Mode

						double tpOffset;
						float tpMSPerBeat;
						int tpMeter;
						int tpSampleType,tpSampleSet;
						int tpVolume;
						int tpTimingChange;
						int tpKiai = 0; // optional
						if (sscanf(curLineChar, " %lf , %f , %i , %i , %i , %i , %i , %i", &tpOffset, &tpMSPerBeat, &tpMeter, &tpSampleType, &tpSampleSet, &tpVolume, &tpTimingChange, &tpKiai) == 8
							|| sscanf(curLineChar, " %lf , %f , %i , %i , %i , %i , %i", &tpOffset, &tpMSPerBeat, &tpMeter, &tpSampleType, &tpSampleSet, &tpVolume, &tpTimingChange) == 7)
						{
							TIMINGPOINT t;
							{
								t.offset = (long)std::round(tpOffset);
								t.msPerBeat = tpMSPerBeat;

								t.sampleType = tpSampleType;
								t.sampleSet = tpSampleSet;
								t.volume = tpVolume;

								t.timingChange = tpTimingChange == 1;
								t.kiai = tpKiai > 0;

								t.sortHack = timingPointSortHack++;
							}
							m_timingpoints.push_back(t);
						}
						else if (sscanf(curLineChar, " %lf , %f", &tpOffset, &tpMSPerBeat) == 2)
						{
							TIMINGPOINT t;
							{
								t.offset = (long)std::round(tpOffset);
								t.msPerBeat = tpMSPerBeat;

								t.sampleType = 0;
								t.sampleSet = 0;
								t.volume = 100;

								t.timingChange = true;
								t.kiai = false;

								t.sortHack = timingPointSortHack++;
							}
							m_timingpoints.push_back(t);
						}
					}
					break;
				}
			}
		}
	}

	// gamemode filter
	if ((m_iGameMode != 0 && m_osu->getGamemode() == Osu::GAMEMODE::STD) || (m_iGameMode != 0x03 && m_osu->getGamemode() == Osu::GAMEMODE::MANIA))
		return false; // nothing more to do here

	// build sound file path
	m_sFullSoundFilePath = m_sFolder;
	m_sFullSoundFilePath.append(m_sAudioFileName);

	// sort timingpoints and calculate BPM range
	if (m_timingpoints.size() > 0)
	{
		if (Osu::debug->getBool())
			debugLog("OsuDatabaseBeatmap::loadMetadata() : calculating BPM range ...\n");

		// sort timingpoints by time
		std::sort(m_timingpoints.begin(), m_timingpoints.end(), TimingPointSortComparator());

		// calculate bpm range
		float tempMinBPM = 0;
		float tempMaxBPM = std::numeric_limits<float>::max();
		for (int i=0; i<m_timingpoints.size(); i++)
		{
			const TIMINGPOINT &t = m_timingpoints[i];

			if (t.msPerBeat >= 0) // NOT inherited
			{
				if (t.msPerBeat > tempMinBPM)
					tempMinBPM = t.msPerBeat;
				if (t.msPerBeat < tempMaxBPM)
					tempMaxBPM = t.msPerBeat;
			}
		}

		// convert from msPerBeat to BPM
		const float msPerMinute = 1 * 60 * 1000;
		if (tempMinBPM != 0)
			tempMinBPM = msPerMinute / tempMinBPM;
		if (tempMaxBPM != 0)
			tempMaxBPM = msPerMinute / tempMaxBPM;

		m_iMinBPM = (int)std::round(tempMinBPM);
		m_iMaxBPM = (int)std::round(tempMaxBPM);
	}

	// special case: old beatmaps have AR = OD, there is no ApproachRate stored
	if (!foundAR)
		m_fAR = m_fOD;

	return true;
}

bool OsuDatabaseBeatmap::loadBeatmapInt(OsuBeatmap *beatmap)
{
	m_iLoadErrorCode = 0;

	// NOTE: reload metadata
	if (!loadBeatmapMetadataInt())
	{
		m_iLoadErrorCode = 1;
		return false;
	}

	// build temporary containers for raw objects
	PRIMITIVE_CONTAINER c = loadPrimitiveObjects(m_sFilePath, m_osu->getGamemode());
	if (c.errorCode != 0)
	{
		m_iLoadErrorCode = c.errorCode;
		return false;
	}

	// check if we have any timingpoints at all
	if (m_timingpoints.size() == 0)
	{
		m_iLoadErrorCode = 3;
		return false;
	}

	// update numObjects
	if (m_iNumObjects == 0)
		m_iNumObjects = c.hitcircles.size() + c.sliders.size() + c.spinners.size();
	if (m_iNumCircles == 0)
		m_iNumCircles = c.hitcircles.size();
	if (m_iNumSliders == 0)
		m_iNumSliders = c.sliders.size();

	// sort timingpoints by time
	std::sort(m_timingpoints.begin(), m_timingpoints.end(), TimingPointSortComparator());

	// calculate sliderTimes, and build slider clicks and ticks
	// TODO: since this is needed for star calc, maybe separate it from here anyway (but make cleaner than before!)
	if (m_timingpoints.size() > 0)
	{
		struct SliderHelper
		{
			static float getSliderTickDistance(float sliderMultiplier, float sliderTickRate)
			{
				return ((100.0f * sliderMultiplier) / sliderTickRate);
			}

			static float getSliderTimeForSlider(const SLIDER &slider, const TIMING_INFO &timingInfo, float sliderMultiplier)
			{
				const float duration = timingInfo.beatLength * (slider.pixelLength / sliderMultiplier) / 100.0f;
				return duration >= 1.0f ? duration : 1.0f; // sanity check
			}

			static float getSliderVelocity(const SLIDER &slider, const TIMING_INFO &timingInfo, float sliderMultiplier, float sliderTickRate)
			{
				const float beatLength = timingInfo.beatLength;
				if (beatLength > 0.0f)
					return (getSliderTickDistance(sliderMultiplier, sliderTickRate) * sliderTickRate * (1000.0f / beatLength));
				else
					return getSliderTickDistance(sliderMultiplier, sliderTickRate) * sliderTickRate;
			}

			static float getTimingPointMultiplierForSlider(const SLIDER &slider, const TIMING_INFO &timingInfo) // needed for slider ticks
			{
				float beatLengthBase = timingInfo.beatLengthBase;
				if (beatLengthBase == 0.0f) // sanity check
					beatLengthBase = 1.0f;

				return timingInfo.beatLength / beatLengthBase;
			}
		};

		for (int i=0; i<c.sliders.size(); i++)
		{
			SLIDER &s = c.sliders[i];

			// sanity reset
			s.ticks.clear();
			s.scoringTimesForStarCalc.clear();

			// calculate duration
			const TIMING_INFO timingInfo = getTimingInfoForTime(s.time);
			s.sliderTimeWithoutRepeats = SliderHelper::getSliderTimeForSlider(s, timingInfo, m_fSliderMultiplier);
			s.sliderTime = s.sliderTimeWithoutRepeats * s.repeat;

			// calculate ticks
			// TODO: validate https://github.com/ppy/osu/pull/3595/files
			const float minTickPixelDistanceFromEnd = 0.01f * SliderHelper::getSliderVelocity(s, timingInfo, m_fSliderMultiplier, m_fSliderTickRate);
			const float tickPixelLength = SliderHelper::getSliderTickDistance(m_fSliderMultiplier, m_fSliderTickRate) / SliderHelper::getTimingPointMultiplierForSlider(s, timingInfo);
			const float tickDurationPercentOfSliderLength = tickPixelLength / (s.pixelLength == 0.0f ? 1.0f : s.pixelLength);
			const int tickCount = (int)std::ceil(s.pixelLength / tickPixelLength) - 1;

			if (tickCount > 0 && !timingInfo.isNaN) // don't generate ticks for NaN timingpoints
			{
				const float tickTOffset = tickDurationPercentOfSliderLength;
				float pixelDistanceToEnd = s.pixelLength;
				float t = tickTOffset;
				for (int i=0; i<tickCount; i++, t+=tickTOffset)
				{
					// skip ticks which are too close to the end of the slider
					pixelDistanceToEnd -= tickPixelLength;
					if (pixelDistanceToEnd <= minTickPixelDistanceFromEnd)
						break;

					s.ticks.push_back(t);
				}
			}

			// calculate s.scoringTimesForStarCalc, which should include every point in time where the cursor must be within the followcircle radius and at least one key must be pressed:
			// see https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Preprocessing/OsuDifficultyHitObject.cs
			// NOTE: only necessary since the latest pp changes (Xexxar)
			if (m_osu_stars_xexxar_angles_sliders_ref->getBool())
			{
				const long osuSliderEndInsideCheckOffset = (long)m_osu_slider_end_inside_check_offset_ref->getInt();

				// 1) "skip the head circle"

				// 2) add repeat times (either at slider begin or end)
				for (int i=0; i<(s.repeat - 1); i++)
				{
					const long time = s.time + (long)(s.sliderTimeWithoutRepeats * (i+1)); // see OsuSlider.cpp
					s.scoringTimesForStarCalc.push_back(time);
				}

				// 3) add tick times (somewhere within slider, repeated for every repeat)
				for (int i=0; i<s.repeat; i++)
				{
					for (int t=0; t<s.ticks.size(); t++)
					{
						const float tickPercentRelativeToRepeatFromStartAbs = (((i+1) % 2) != 0 ? s.ticks[t] : 1.0f - s.ticks[t]); // see OsuSlider.cpp
						const long time = s.time + (long)(s.sliderTimeWithoutRepeats * i) + (long)(tickPercentRelativeToRepeatFromStartAbs * s.sliderTimeWithoutRepeats); // see OsuSlider.cpp
						s.scoringTimesForStarCalc.push_back(time);
					}
				}

				// 4) add slider end (potentially before last tick for bullshit sliders, but sorting takes care of that)
				// see https://github.com/ppy/osu/pull/4193#issuecomment-460127543
				s.scoringTimesForStarCalc.push_back(std::max(s.time + (long)s.sliderTime / 2, (s.time + (long)s.sliderTime) - osuSliderEndInsideCheckOffset)); // see OsuSlider.cpp

				// 5) sort scoringTimes from earliest to latest
				std::sort(s.scoringTimesForStarCalc.begin(), s.scoringTimesForStarCalc.end(), std::less<long>());
			}
		}
	}

	// build hitobjects from the data we loaded from the beatmap file
	OsuBeatmapStandard *beatmapStandard = dynamic_cast<OsuBeatmapStandard*>(beatmap);
	OsuBeatmapMania *beatmapMania = dynamic_cast<OsuBeatmapMania*>(beatmap);
	m_loadHitObjects.clear();
	{
		if (beatmapStandard != NULL)
		{
			// also calculate max combo
			int maxCombo = 0;

			for (int i=0; i<c.hitcircles.size(); i++)
			{
				HITCIRCLE &h = c.hitcircles[i];

				if (m_osu_mod_random_ref->getBool())
				{
					h.x = clamp<int>(h.x - (int)(((rand() % OsuGameRules::OSU_COORD_WIDTH) / 8.0f) * m_osu_mod_random_circle_offset_x_percent_ref->getFloat()), 0, OsuGameRules::OSU_COORD_WIDTH);
					h.y = clamp<int>(h.y - (int)(((rand() % OsuGameRules::OSU_COORD_HEIGHT) / 8.0f) * m_osu_mod_random_circle_offset_y_percent_ref->getFloat()), 0, OsuGameRules::OSU_COORD_HEIGHT);
				}

				m_loadHitObjects.push_back(new OsuCircle(h.x, h.y, h.time, h.sampleType, h.number, false, h.colorCounter, h.colorOffset, beatmapStandard));

				// potential convert-all-circles-to-sliders mod, have to play around more with this
				/*
				if (i+1 < c.hitcircles.size())
				{
					std::vector<Vector2> points;
					Vector2 p1 = Vector2(c.hitcircles[i].x, c.hitcircles[i].y);
					Vector2 p2 = Vector2(c.hitcircles[i+1].x, c.hitcircles[i+1].y);
					points.push_back(p1);
					points.push_back(p2 - (p2 - p1).normalize()*35);
					const float pixelLength = (p2 - p1).length();
					const unsigned long time = c.hitcircles[i].time;
					const unsigned long timeEnd = c.hitcircles[i+1].time;
					const unsigned long sliderTime = timeEnd - time;

					bool blocked = false;
					for (int s=0; s<c.sliders.size(); s++)
					{
						if (c.sliders[s].time > time && c.sliders[s].time < timeEnd)
						{
							blocked = true;
							break;
						}
					}
					for (int s=0; s<c.spinners.size(); s++)
					{
						if (c.spinners[s].time > time && c.spinners[s].time < timeEnd)
						{
							blocked = true;
							break;
						}
					}

					blocked |= pixelLength < 45;

					if (!blocked)
						m_loadHitObjects.push_back(new OsuSlider(OsuSliderCurve::OSUSLIDERCURVETYPE::OSUSLIDERCURVETYPE_LINEAR, 1, pixelLength, points, std::vector<int>(), std::vector<float>(), sliderTime, sliderTime, time, h.sampleType, h.number, false, h.colorCounter, h.colorOffset, beatmapStandard));
					else
						m_loadHitObjects.push_back(new OsuCircle(h.x, h.y, h.time, h.sampleType, h.number, false, h.colorCounter, h.colorOffset, beatmapStandard));
				}
				*/
			}
			maxCombo += c.hitcircles.size();

			for (int i=0; i<c.sliders.size(); i++)
			{
				SLIDER &s = c.sliders[i];

				if (m_osu_mod_random_ref->getBool())
				{
					for (int p=0; p<s.points.size(); p++)
					{
						s.points[p].x = clamp<int>(s.points[p].x - (int)(((rand() % OsuGameRules::OSU_COORD_WIDTH) / 3.0f) * m_osu_mod_random_slider_offset_x_percent_ref->getFloat()), 0, OsuGameRules::OSU_COORD_WIDTH);
						s.points[p].y = clamp<int>(s.points[p].y - (int)(((rand() % OsuGameRules::OSU_COORD_HEIGHT) / 3.0f) * m_osu_mod_random_slider_offset_y_percent_ref->getFloat()), 0, OsuGameRules::OSU_COORD_HEIGHT);
					}
				}

				if (m_osu_mod_reverse_sliders_ref->getBool())
					std::reverse(s.points.begin(), s.points.end());

				m_loadHitObjects.push_back(new OsuSlider(s.type, s.repeat, s.pixelLength, s.points, s.hitSounds, s.ticks, s.sliderTime, s.sliderTimeWithoutRepeats, s.time, s.sampleType, s.number, false, s.colorCounter, s.colorOffset, beatmapStandard));

				const int repeats = std::max((s.repeat - 1), 0);
				maxCombo += 2 + repeats + (repeats+1)*s.ticks.size(); // start/end + repeat arrow + ticks
			}

			for (int i=0; i<c.spinners.size(); i++)
			{
				SPINNER &s = c.spinners[i];

				if (m_osu_mod_random_ref->getBool())
				{
					s.x = clamp<int>(s.x - (int)(((rand() % OsuGameRules::OSU_COORD_WIDTH) / 1.25f) * (rand() % 2 == 0 ? 1.0f : -1.0f) * m_osu_mod_random_spinner_offset_x_percent_ref->getFloat()), 0, OsuGameRules::OSU_COORD_WIDTH);
					s.y = clamp<int>(s.y - (int)(((rand() % OsuGameRules::OSU_COORD_HEIGHT) / 1.25f) * (rand() % 2 == 0 ? 1.0f : -1.0f) * m_osu_mod_random_spinner_offset_y_percent_ref->getFloat()), 0, OsuGameRules::OSU_COORD_HEIGHT);
				}

				m_loadHitObjects.push_back(new OsuSpinner(s.x, s.y, s.time, s.sampleType, false, s.endTime, beatmapStandard));
			}
			maxCombo += c.spinners.size();

			// debug
			// TODO: rewrite this debug feature for the new structure
			/*
			if (m_osu_debug_pp->getBool())
			{
				double aim = 0.0;
				double speed = 0.0;
				double stars = calculateStarDiff(beatmap, &aim, &speed);
				double pp = OsuDifficultyCalculator::calculatePPv2(beatmap->getOsu(), beatmap, aim, speed, hitobjects->size(), hitcircles.size(), m_iMaxCombo);

				engine->showMessageInfo("PP", UString::format("pp = %f, stars = %f, aimstars = %f, speedstars = %f, %i circles, %i sliders, %i spinners, %i hitobjects, maxcombo = %i", pp, stars, aim, speed, hitcircles.size(), sliders.size(), spinners.size(), (hitcircles.size() + sliders.size() + spinners.size()), m_iMaxCombo));
			}
			*/

			beatmapStandard->setMaxPossibleCombo(maxCombo);
		}
		else if (beatmapMania != NULL)
		{
			struct ManiaHelper
			{
				static int getColumn(int availableColumns, float position, bool allowSpecial = false)
				{
					if (allowSpecial && availableColumns == 8)
					{
						const float local_x_divisor = 512.0f / 7;
						return clamp<int>((int)std::floor(position / local_x_divisor), 0, 6) + 1;
					}

					const float localXDivisor = 512.0f / availableColumns;
					return clamp<int>((int)std::floor(position / localXDivisor), 0, availableColumns - 1);
				}
			};

			const int availableColumns = beatmapMania->getNumColumns();

			for (int i=0; i<c.hitcircles.size(); i++)
			{
				HITCIRCLE &h = c.hitcircles[i];
				m_loadHitObjects.push_back(new OsuManiaNote(ManiaHelper::getColumn(availableColumns, h.x), h.maniaEndTime > 0 ? (h.maniaEndTime - h.time) : 0, h.time, h.sampleType, h.number, h.colorCounter, beatmapMania));
			}

			for (int i=0; i<c.sliders.size(); i++)
			{
				SLIDER &s = c.sliders[i];
				m_loadHitObjects.push_back(new OsuManiaNote(ManiaHelper::getColumn(availableColumns, s.x), s.sliderTime, s.time, s.sampleType, s.number, s.colorCounter, beatmapMania));
			}
		}
	}

	// sort hitobjects by starttime
	struct HitObjectSortComparator
	{
	    bool operator() (OsuHitObject const *a, OsuHitObject const *b) const
	    {
	    	// strict weak ordering!
	    	if (a->getTime() == b->getTime())
	    		return a->getSortHack() < b->getSortHack();
	    	else
	    		return a->getTime() < b->getTime();
	    }
	};
	std::sort(m_loadHitObjects.begin(), m_loadHitObjects.end(), HitObjectSortComparator());

	// update beatmap length stat
	if (m_iLengthMS == 0 && m_loadHitObjects.size() > 0)
		m_iLengthMS = m_loadHitObjects[m_loadHitObjects.size() - 1]->getTime() + m_loadHitObjects[m_loadHitObjects.size() - 1]->getDuration();

	// set isEndOfCombo + precalculate Score v2 combo portion maximum
	if (beatmapStandard != NULL)
	{
		unsigned long long scoreV2ComboPortionMaximum = 1;

		if (m_loadHitObjects.size() > 0)
			scoreV2ComboPortionMaximum = 0;

		int combo = 0;
		for (int i=0; i<m_loadHitObjects.size(); i++)
		{
			OsuHitObject *currentHitObject = m_loadHitObjects[i];
			const OsuHitObject *nextHitObject = (i + 1 < m_loadHitObjects.size() ? m_loadHitObjects[i + 1] : NULL);

			const OsuCircle *circlePointer = dynamic_cast<OsuCircle*>(currentHitObject);
			const OsuSlider *sliderPointer = dynamic_cast<OsuSlider*>(currentHitObject);
			const OsuSpinner *spinnerPointer = dynamic_cast<OsuSpinner*>(currentHitObject);

			int scoreComboMultiplier = std::max(combo-1, 0);

			if (circlePointer != NULL || spinnerPointer != NULL)
			{
				scoreV2ComboPortionMaximum += (unsigned long long)(300.0 * (1.0 + (double)scoreComboMultiplier / 10.0));
				combo++;
			}
			else if (sliderPointer != NULL)
			{
				combo += 1 + sliderPointer->getClicks().size();
				scoreComboMultiplier = std::max(combo-1, 0);
				scoreV2ComboPortionMaximum += (unsigned long long)(300.0 * (1.0 + (double)scoreComboMultiplier / 10.0));
				combo++;
			}

			if (nextHitObject == NULL || nextHitObject->getComboNumber() == 1)
				currentHitObject->setIsEndOfCombo(true);
		}

		beatmapStandard->setScoreV2ComboPortionMaximum(scoreV2ComboPortionMaximum);
	}

	// special rule for first hitobject (for 1 approach circle with HD)
	if (m_osu_show_approach_circle_on_first_hidden_object_ref->getBool())
	{
		if (m_loadHitObjects.size() > 0)
			m_loadHitObjects[0]->setForceDrawApproachCircle(true);
	}

	debugLog("OsuDatabaseBeatmap::load() loaded %i hitobjects\n", m_loadHitObjects.size());

	// update combo colors in skin
	// TODO: this is not safe async! (but currently unused, so)
	beatmap->getOsu()->getSkin()->setBeatmapComboColors(c.combocolors);

	return true;
}

void OsuDatabaseBeatmap::setDifficulties(std::vector<OsuDatabaseBeatmap*> &difficulties)
{
	m_difficulties = difficulties;

	if (m_difficulties.size() > 0)
	{
		// set representative values for this container (i.e. use values from first difficulty)
		m_sTitle = m_difficulties[0]->m_sTitle;
		m_sArtist = m_difficulties[0]->m_sArtist;
		m_sCreator = m_difficulties[0]->m_sCreator;
		m_sBackgroundImageFileName = m_difficulties[0]->m_sBackgroundImageFileName;

		// also precalculate some largest representative values
		m_iLengthMS = 0;
		m_fCS = 0.0f;
		m_fAR = 0.0f;
		m_fOD = 0.0f;
		m_fHP = 0.0f;
		m_fStarsNomod = 0.0f; // TODO: this needs dynamic recalculation on parent as well
		m_iMaxBPM = 0;
		m_iLastModificationTime = 0;
		for (size_t i=0; i<m_difficulties.size(); i++)
		{
			if (m_difficulties[i]->getLengthMS() > m_iLengthMS)
				m_iLengthMS = m_difficulties[i]->getLengthMS();
			if (m_difficulties[i]->getCS() > m_fCS)
				m_fCS = m_difficulties[i]->getCS();
			if (m_difficulties[i]->getAR() > m_fAR)
				m_fAR = m_difficulties[i]->getAR();
			if (m_difficulties[i]->getHP() > m_fHP)
				m_fHP = m_difficulties[i]->getHP();
			if (m_difficulties[i]->getOD() > m_fOD)
				m_fOD = m_difficulties[i]->getOD();
			if (m_difficulties[i]->getStarsNomod() > m_fStarsNomod)
				m_fStarsNomod = m_difficulties[i]->getStarsNomod();
			if (m_difficulties[i]->getMaxBPM() > m_iMaxBPM)
				m_iMaxBPM = m_difficulties[i]->getMaxBPM();
			if (m_difficulties[i]->getLastModificationTime() > m_iLastModificationTime)
				m_iLastModificationTime = m_difficulties[i]->getLastModificationTime();
		}
	}
}

OsuDatabaseBeatmap::TIMING_INFO OsuDatabaseBeatmap::getTimingInfoForTime(unsigned long positionMS)
{
	TIMING_INFO ti;
	ti.offset = 0;
	ti.beatLengthBase = 1;
	ti.beatLength = 1;
	ti.volume = 100;
	ti.sampleType = 0;
	ti.sampleSet = 0;
	ti.isNaN = false;

	if (m_timingpoints.size() < 1) return ti;

	// initial values
	ti.offset = m_timingpoints[0].offset;
	ti.volume = m_timingpoints[0].volume;
	ti.sampleSet = m_timingpoints[0].sampleSet;
	ti.sampleType = m_timingpoints[0].sampleType;

	// new (peppy's algorithm)
	// (correctly handles aspire & NaNs)
	{
		const bool allowMultiplier = true;

		int point = 0;
		int samplePoint = 0;
		int audioPoint = 0;

		for (int i=0; i<m_timingpoints.size(); i++)
		{
			if (m_timingpoints[i].offset <= positionMS)
			{
				audioPoint = i;

				if (m_timingpoints[i].timingChange)
					point = i;
				else
					samplePoint = i;
			}
		}

		double mult = 1;

		if (allowMultiplier && samplePoint > point && m_timingpoints[samplePoint].msPerBeat < 0)
		{
			if (m_timingpoints[samplePoint].msPerBeat >= 0)
				mult = 1;
			else
				mult = clamp<float>((float)-m_timingpoints[samplePoint].msPerBeat, 10.0f, 1000.0f) / 100.0f;
		}

		ti.beatLengthBase = m_timingpoints[point].msPerBeat;
		ti.offset = m_timingpoints[point].offset;

		ti.isNaN = std::isnan(m_timingpoints[samplePoint].msPerBeat) || std::isnan(m_timingpoints[point].msPerBeat);
		ti.beatLength = ti.beatLengthBase * mult;

		ti.volume = m_timingpoints[audioPoint].volume;
		ti.sampleType = m_timingpoints[audioPoint].sampleType;
		ti.sampleSet = m_timingpoints[audioPoint].sampleSet;
	}

	// old (McKay's algorithm)
	// (doesn't work for all aspire maps, e.g. XNOR)
	/*
	// initial timing values (get first non-inherited timingpoint as base)
	for (int i=0; i<timingpoints.size(); i++)
	{
		TIMINGPOINT *t = &timingpoints[i];
		if (t->msPerBeat >= 0)
		{
			ti.beatLength = t->msPerBeat;
			ti.beatLengthBase = ti.beatLength;
			ti.offset = t->offset;
			break;
		}
	}

	// go through all timingpoints before positionMS
	for (int i=0; i<timingpoints.size(); i++)
	{
		TIMINGPOINT *t = &timingpoints[i];
		if (t->offset > (long)positionMS)
			break;

		//debugLog("timingpoint %i msperbeat = %f\n", i, t->msPerBeat);

		if (t->msPerBeat >= 0) // NOT inherited
		{
			ti.beatLengthBase = t->msPerBeat;
			ti.beatLength = ti.beatLengthBase;
			ti.offset = t->offset;
		}
		else // inherited
		{
			// note how msPerBeat is clamped
			ti.isNaN = std::isnan(t->msPerBeat);
			ti.beatLength = ti.beatLengthBase * (clamp<float>(!ti.isNaN ? std::abs(t->msPerBeat) : 1000, 10, 1000) / 100.0f); // sliderMultiplier of a timingpoint = (t->velocity / -100.0f)
		}

		ti.volume = t->volume;
		ti.sampleType = t->sampleType;
		ti.sampleSet = t->sampleSet;
	}
	*/

	return ti;
}



OsuDatabaseBeatmapBackgroundImagePathLoader::OsuDatabaseBeatmapBackgroundImagePathLoader(const UString &filePath) : Resource()
{
	m_sFilePath = filePath;
}

void OsuDatabaseBeatmapBackgroundImagePathLoader::init()
{
	// (nothing)
	m_bReady = true;
}

void OsuDatabaseBeatmapBackgroundImagePathLoader::initAsync()
{
	File file(m_sFilePath);
	if (!file.canRead())
		return;

	int curBlock = -1;
	char stringBuffer[1024];
	while (file.canRead())
	{
		UString uCurLine = file.readLine();
		const char *curLineChar = uCurLine.toUtf8();
		std::string curLine(curLineChar);

		const int commentIndex = curLine.find("//");
		if (commentIndex == std::string::npos || commentIndex != 0) // ignore comments, but only if at the beginning of a line (e.g. allow Artist:DJ'TEKINA//SOMETHING)
		{
			if (curLine.find("[Events]") != std::string::npos)
				curBlock = 1;
			else if (curLine.find("[TimingPoints]") != std::string::npos)
				break; // NOTE: stop early
			else if (curLine.find("[Colours]") != std::string::npos)
				break; // NOTE: stop early
			else if (curLine.find("[HitObjects]") != std::string::npos)
				break; // NOTE: stop early

			switch (curBlock)
			{
			case 1: // Events
				{
					memset(stringBuffer, '\0', 1024);
					int type, startTime;
					if (sscanf(curLineChar, " %i , %i , \"%1023[^\"]\"", &type, &startTime, stringBuffer) == 3)
					{
						if (type == 0)
							m_sLoadedBackgroundImageFileName = UString(stringBuffer);
					}
				}
				break;
			}
		}
	}

	m_bAsyncReady = true;
	m_bReady = true; // NOTE: on purpose. there is nothing to do in init(), so finish 1 frame early
}



OsuDatabaseBeatmapStarCalculator::OsuDatabaseBeatmapStarCalculator() : Resource()
{

}

void OsuDatabaseBeatmapStarCalculator::init()
{

}

void OsuDatabaseBeatmapStarCalculator::initAsync()
{

}
