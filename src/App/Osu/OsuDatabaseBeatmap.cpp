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

#include "Osu.h"
#include "OsuFile.h"
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

#include <sstream>
#include <iostream>

ConVar osu_mod_random("osu_mod_random", false, FCVAR_NONE);
ConVar osu_mod_random_seed("osu_mod_random_seed", 0, FCVAR_NONE, "0 = random seed every reload, any other value will force that value to be used as the seed");
ConVar osu_mod_random_circle_offset_x_percent("osu_mod_random_circle_offset_x_percent", 1.0f, FCVAR_NONE, "how much the randomness affects things");
ConVar osu_mod_random_circle_offset_y_percent("osu_mod_random_circle_offset_y_percent", 1.0f, FCVAR_NONE, "how much the randomness affects things");
ConVar osu_mod_random_slider_offset_x_percent("osu_mod_random_slider_offset_x_percent", 1.0f, FCVAR_NONE, "how much the randomness affects things");
ConVar osu_mod_random_slider_offset_y_percent("osu_mod_random_slider_offset_y_percent", 1.0f, FCVAR_NONE, "how much the randomness affects things");
ConVar osu_mod_random_spinner_offset_x_percent("osu_mod_random_spinner_offset_x_percent", 1.0f, FCVAR_NONE, "how much the randomness affects things");
ConVar osu_mod_random_spinner_offset_y_percent("osu_mod_random_spinner_offset_y_percent", 1.0f, FCVAR_NONE, "how much the randomness affects things");
ConVar osu_mod_reverse_sliders("osu_mod_reverse_sliders", false, FCVAR_NONE);
ConVar osu_mod_strict_tracking("osu_mod_strict_tracking", false, FCVAR_NONE);
ConVar osu_mod_strict_tracking_remove_slider_ticks("osu_mod_strict_tracking_remove_slider_ticks", false, FCVAR_NONE, "whether the strict tracking mod should remove slider ticks or not, this changed after its initial implementation in lazer");

ConVar osu_show_approach_circle_on_first_hidden_object("osu_show_approach_circle_on_first_hidden_object", true, FCVAR_NONE);

ConVar osu_stars_stacking("osu_stars_stacking", true, FCVAR_NONE, "respect hitobject stacking before calculating stars/pp");

ConVar osu_slider_max_repeats("osu_slider_max_repeats", 9000, FCVAR_NONE, "maximum number of repeats allowed per slider (clamp range)");
ConVar osu_slider_max_ticks("osu_slider_max_ticks", 2048, FCVAR_NONE, "maximum number of ticks allowed per slider (clamp range)");

ConVar osu_number_max("osu_number_max", 0, FCVAR_NONE, "0 = disabled, 1/2/3/4/etc. limits visual circle numbers to this number");
ConVar osu_ignore_beatmap_combo_numbers("osu_ignore_beatmap_combo_numbers", false, FCVAR_NONE, "may be used in conjunction with osu_number_max");

ConVar osu_beatmap_version("osu_beatmap_version", 128, FCVAR_NONE, "maximum supported .osu file version, above this will simply not load (this was 14 but got bumped to 128 due to lazer backports)");
ConVar osu_beatmap_max_num_hitobjects("osu_beatmap_max_num_hitobjects", 40000, FCVAR_NONE, "maximum number of total allowed hitobjects per beatmap (prevent crashing on deliberate game-breaking beatmaps)");
ConVar osu_beatmap_max_num_slider_scoringtimes("osu_beatmap_max_num_slider_scoringtimes", 32768, FCVAR_NONE, "maximum number of slider score increase events allowed per slider (prevent crashing on deliberate game-breaking beatmaps)");

unsigned long long OsuDatabaseBeatmap::sortHackCounter = 0;

ConVar *OsuDatabaseBeatmap::m_osu_slider_curve_max_length_ref = NULL;
ConVar *OsuDatabaseBeatmap::m_osu_stars_xexxar_angles_sliders_ref = NULL;
ConVar *OsuDatabaseBeatmap::m_osu_stars_stacking_ref = NULL;
ConVar *OsuDatabaseBeatmap::m_osu_debug_pp_ref = NULL;
ConVar *OsuDatabaseBeatmap::m_osu_slider_end_inside_check_offset_ref = NULL;

OsuDatabaseBeatmap::OsuDatabaseBeatmap(Osu *osu, UString filePath, UString folder, bool filePathIsInMemoryBeatmap)
{
	m_osu = osu;

	m_sFilePath = filePath;
	m_bFilePathIsInMemoryBeatmap = filePathIsInMemoryBeatmap;

	m_sFolder = folder;

	m_iSortHack = sortHackCounter++;



	// convar refs
	if (m_osu_slider_curve_max_length_ref == NULL)
		m_osu_slider_curve_max_length_ref = convar->getConVarByName("osu_slider_curve_max_length");
	if (m_osu_stars_xexxar_angles_sliders_ref == NULL)
		m_osu_stars_xexxar_angles_sliders_ref = convar->getConVarByName("osu_stars_xexxar_angles_sliders");
	if (m_osu_stars_stacking_ref == NULL)
		m_osu_stars_stacking_ref = convar->getConVarByName("osu_stars_stacking");
	if (m_osu_debug_pp_ref == NULL)
		m_osu_debug_pp_ref = convar->getConVarByName("osu_debug_pp");
	if (m_osu_slider_end_inside_check_offset_ref == NULL)
		m_osu_slider_end_inside_check_offset_ref = convar->getConVarByName("osu_slider_end_inside_check_offset");



	// raw metadata (note the special default values)

	m_iVersion = osu_beatmap_version.getInt();
	m_iGameMode = 0;
	m_iID = 0;
	m_iSetID = -1;

	m_iLengthMS = 0;
	m_iPreviewTime = -1;

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
	m_iMostCommonBPM = 0;

	m_iNumObjects = 0;
	m_iNumCircles = 0;
	m_iNumSliders = 0;
	m_iNumSpinners = 0;



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
		delete m_difficulties[i];
	}
}

OsuDatabaseBeatmap::PRIMITIVE_CONTAINER OsuDatabaseBeatmap::loadPrimitiveObjects(const UString &osuFilePath, Osu::GAMEMODE gameMode, bool filePathIsInMemoryBeatmap)
{
	std::atomic<bool> dead;
	dead = false;
	return loadPrimitiveObjects(osuFilePath, gameMode, filePathIsInMemoryBeatmap, dead);
}

OsuDatabaseBeatmap::PRIMITIVE_CONTAINER OsuDatabaseBeatmap::loadPrimitiveObjects(const UString &osuFilePath, Osu::GAMEMODE gameMode, bool filePathIsInMemoryBeatmap, const std::atomic<bool> &dead)
{
	PRIMITIVE_CONTAINER c;
	{
		c.errorCode = 0;

		c.stackLeniency = 0.7f;

		c.sliderMultiplier = 1.0f;
		c.sliderTickRate = 1.0f;

		c.version = 14;
	}

	const float sliderSanityRange = m_osu_slider_curve_max_length_ref->getFloat(); // infinity sanity check, same as before
	const int sliderMaxRepeatRange = osu_slider_max_repeats.getInt(); // NOTE: osu! will refuse to play any beatmap which has sliders with more than 9000 repeats, here we just clamp it instead



	// open osu file for parsing
	{
		File file(!filePathIsInMemoryBeatmap ? osuFilePath : "");
		if (!file.canRead() && !filePathIsInMemoryBeatmap)
		{
			c.errorCode = 2;
			return c;
		}

		std::istringstream ss(filePathIsInMemoryBeatmap ? osuFilePath.toUtf8() : ""); // eh

		// load the actual beatmap
		unsigned long long timingPointSortHack = 0;
		int hitobjectsWithoutSpinnerCounter = 0;
		int colorCounter = 1;
		int colorOffset = 0;
		int comboNumber = 1;
		int curBlock = -1;
		std::string curLine;
		while (!filePathIsInMemoryBeatmap ? file.canRead() : static_cast<bool>(std::getline(ss, curLine)))
		{
			if (dead.load())
			{
				c.errorCode = 6;
				return c;
			}

			UString uCurLine;
			char *curLineChar = NULL;
			{
				if (!filePathIsInMemoryBeatmap)
				{
					uCurLine = file.readLine();
					curLineChar = (char*)uCurLine.toUtf8();
					curLine = std::string(curLineChar);
				}
				else
					curLineChar = (char*)curLine.c_str();
			}

			const int commentIndex = curLine.find("//");
			if (commentIndex == std::string::npos || commentIndex != 0) // ignore comments, but only if at the beginning of a line (e.g. allow Artist:DJ'TEKINA//SOMETHING)
			{
				if (curLine.find("[General]") != std::string::npos)
					curBlock = 1;
				else if (curLine.find("[Difficulty]") != std::string::npos)
					curBlock = 2;
				else if (curLine.find("[Events]") != std::string::npos)
					curBlock = 3;
				else if (curLine.find("[TimingPoints]") != std::string::npos)
					curBlock = 4;
				else if (curLine.find("[Colours]") != std::string::npos)
					curBlock = 5;
				else if (curLine.find("[HitObjects]") != std::string::npos)
					curBlock = 6;

				switch (curBlock)
				{
				case -1: // header (e.g. "osu file format v12")
					{
						sscanf(curLineChar, " osu file format v %i \n", &c.version);
					}
					break;

				case 1: // General
					{
						sscanf(curLineChar, " StackLeniency : %f \n", &c.stackLeniency);
					}
					break;

				case 2: // Difficulty
					{
						sscanf(curLineChar, " SliderMultiplier : %f \n", &c.sliderMultiplier);
						sscanf(curLineChar, " SliderTickRate : %f \n", &c.sliderTickRate);
					}
					break;

				case 3: // Events
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
							c.timingpoints.push_back(t);
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
							c.timingpoints.push_back(t);
						}
					}
					break;

				case 5: // Colours
					{
						int comboNum;
						int r,g,b;
						if (sscanf(curLineChar, " Combo %i : %i , %i , %i \n", &comboNum, &r, &g, &b) == 4)
							c.combocolors.push_back(COLOR(255, r, g, b));
					}
					break;

				case 6: // HitObjects

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

								points.push_back(Vector2((int)clamp<float>(sliderXY[0].toFloat(), -sliderSanityRange, sliderSanityRange), (int)clamp<float>(sliderXY[1].toFloat(), -sliderSanityRange, sliderSanityRange)));
							}

							// special case: osu! logic for handling the hitobject point vs the controlpoints (since sliders have both, and older beatmaps store the start point inside the control points)
							{
								const Vector2 xy = Vector2(clamp<float>(x, -sliderSanityRange, sliderSanityRange), clamp<float>(y, -sliderSanityRange, sliderSanityRange));
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
								s.repeat = clamp<int>((int)tokens[6].toFloat(), -sliderMaxRepeatRange, sliderMaxRepeatRange);
								s.repeat = s.repeat >= 0 ? s.repeat : 0; // sanity check
								s.pixelLength = clamp<float>(tokens[7].toFloat(), -sliderSanityRange, sliderSanityRange);
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

	// late bail if too many hitobjects would run out of memory and crash
	const size_t numHitobjects = c.hitcircles.size() + c.sliders.size() + c.spinners.size();
	if (numHitobjects > (size_t)osu_beatmap_max_num_hitobjects.getInt())
	{
		c.errorCode = 5;
		return c;
	}

	// sort timingpoints by time
	if (c.timingpoints.size() > 0)
		std::sort(c.timingpoints.begin(), c.timingpoints.end(), TimingPointSortComparator());

	return c;
}

OsuDatabaseBeatmap::CALCULATE_SLIDER_TIMES_CLICKS_TICKS_RESULT OsuDatabaseBeatmap::calculateSliderTimesClicksTicks(int beatmapVersion, std::vector<SLIDER> &sliders, std::vector<TIMINGPOINT> &timingpoints, float sliderMultiplier, float sliderTickRate)
{
	std::atomic<bool> dead;
	dead = false;
	return calculateSliderTimesClicksTicks(beatmapVersion, sliders, timingpoints, sliderMultiplier, sliderTickRate, false);
}

OsuDatabaseBeatmap::CALCULATE_SLIDER_TIMES_CLICKS_TICKS_RESULT OsuDatabaseBeatmap::calculateSliderTimesClicksTicks(int beatmapVersion, std::vector<SLIDER> &sliders, std::vector<TIMINGPOINT> &timingpoints, float sliderMultiplier, float sliderTickRate, const std::atomic<bool> &dead)
{
	CALCULATE_SLIDER_TIMES_CLICKS_TICKS_RESULT r;
	{
		r.errorCode = 0;
	}

	if (timingpoints.size() < 1)
	{
		r.errorCode = 3;
		return r;
	}

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

	unsigned long long sortHackCounter = 0;
	for (int i=0; i<sliders.size(); i++)
	{
		if (dead.load())
		{
			r.errorCode = 6;
			return r;
		}

		SLIDER &s = sliders[i];

		// sanity reset
		s.ticks.clear();
		s.scoringTimesForStarCalc.clear();

		// calculate duration
		const TIMING_INFO timingInfo = getTimingInfoForTimeAndTimingPoints(s.time, timingpoints);
		s.sliderTimeWithoutRepeats = SliderHelper::getSliderTimeForSlider(s, timingInfo, sliderMultiplier);
		s.sliderTime = s.sliderTimeWithoutRepeats * s.repeat;

		// calculate ticks
		{
			const float minTickPixelDistanceFromEnd = 0.01f * SliderHelper::getSliderVelocity(s, timingInfo, sliderMultiplier, sliderTickRate);
			const float tickPixelLength = (beatmapVersion < 8 ? SliderHelper::getSliderTickDistance(sliderMultiplier, sliderTickRate) : SliderHelper::getSliderTickDistance(sliderMultiplier, sliderTickRate) / SliderHelper::getTimingPointMultiplierForSlider(s, timingInfo));
			const float tickDurationPercentOfSliderLength = tickPixelLength / (s.pixelLength == 0.0f ? 1.0f : s.pixelLength);
			const int tickCount = std::min((int)std::ceil(s.pixelLength / tickPixelLength) - 1, osu_slider_max_ticks.getInt()); // NOTE: hard sanity limit number of ticks per slider

			if (tickCount > 0 && !timingInfo.isNaN && !std::isnan(s.pixelLength) && !std::isnan(tickPixelLength)) // don't generate ticks for NaN timingpoints and infinite values
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
		}

		// bail if too many predicted heuristic scoringTimes would run out of memory and crash
		if ((size_t)std::abs(s.repeat) * s.ticks.size() > (size_t)osu_beatmap_max_num_slider_scoringtimes.getInt())
		{
			r.errorCode = 5;
			return r;
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
				const float time = (float)s.time + (s.sliderTimeWithoutRepeats * (i+1)); // see OsuSlider.cpp
				s.scoringTimesForStarCalc.push_back(OsuDifficultyHitObject::SLIDER_SCORING_TIME{
					.type = OsuDifficultyHitObject::SLIDER_SCORING_TIME::TYPE::REPEAT,
					.time = time,
					.sortHack = sortHackCounter++,
				});
			}

			// 3) add tick times (somewhere within slider, repeated for every repeat)
			for (int i=0; i<s.repeat; i++)
			{
				for (int t=0; t<s.ticks.size(); t++)
				{
					const float tickPercentRelativeToRepeatFromStartAbs = (((i+1) % 2) != 0 ? s.ticks[t] : 1.0f - s.ticks[t]); // see OsuSlider.cpp
					const float time = (float)s.time + (s.sliderTimeWithoutRepeats * i) + (tickPercentRelativeToRepeatFromStartAbs * s.sliderTimeWithoutRepeats); // see OsuSlider.cpp
					s.scoringTimesForStarCalc.push_back(OsuDifficultyHitObject::SLIDER_SCORING_TIME{
						.type = OsuDifficultyHitObject::SLIDER_SCORING_TIME::TYPE::TICK,
						.time = time,
						.sortHack = sortHackCounter++,
					});
				}
			}

			// 4) add slider end (potentially before last tick for bullshit sliders, but sorting takes care of that)
			// see https://github.com/ppy/osu/pull/4193#issuecomment-460127543
			const float time = std::max((float)s.time + s.sliderTime / 2.0f, ((float)s.time + s.sliderTime) - osuSliderEndInsideCheckOffset);
			s.scoringTimesForStarCalc.push_back(OsuDifficultyHitObject::SLIDER_SCORING_TIME{
				.type = OsuDifficultyHitObject::SLIDER_SCORING_TIME::TYPE::END,
				.time = time,
				.sortHack = sortHackCounter++,
			});

			// 5) sort scoringTimes from earliest to latest
			std::sort(s.scoringTimesForStarCalc.begin(), s.scoringTimesForStarCalc.end(), OsuDifficultyHitObject::SliderScoringTimeComparator());
		}
	}

	return r;
}

OsuDatabaseBeatmap::LOAD_DIFFOBJ_RESULT OsuDatabaseBeatmap::loadDifficultyHitObjects(const UString &osuFilePath, Osu::GAMEMODE gameMode, float AR, float CS, float speedMultiplier, bool calculateStarsInaccurately)
{
	std::atomic<bool> dead;
	dead = false;
	return loadDifficultyHitObjects(osuFilePath, gameMode, AR, CS, speedMultiplier, calculateStarsInaccurately, dead);
}

OsuDatabaseBeatmap::LOAD_DIFFOBJ_RESULT OsuDatabaseBeatmap::loadDifficultyHitObjects(const UString &osuFilePath, Osu::GAMEMODE gameMode, float AR, float CS, float speedMultiplier, bool calculateStarsInaccurately, const std::atomic<bool> &dead)
{
	LOAD_DIFFOBJ_RESULT result = LOAD_DIFFOBJ_RESULT();

	// build generalized OsuDifficultyHitObjects from the vectors (hitcircles, sliders, spinners)
	// the OsuDifficultyHitObject class is the one getting used in all pp/star calculations, it encompasses every object type for simplicity

	// load primitive arrays
	PRIMITIVE_CONTAINER c = loadPrimitiveObjects(osuFilePath, gameMode, false, dead);
	if (c.errorCode != 0)
	{
		result.errorCode = c.errorCode;
		return result;
	}

	// calculate sliderTimes, and build slider clicks and ticks
	CALCULATE_SLIDER_TIMES_CLICKS_TICKS_RESULT sliderTimeCalcResult = calculateSliderTimesClicksTicks(c.version, c.sliders, c.timingpoints, c.sliderMultiplier, c.sliderTickRate, dead);
	if (sliderTimeCalcResult.errorCode != 0)
	{
		result.errorCode = sliderTimeCalcResult.errorCode;
		return result;
	}

	// now we can calculate the max possible combo (because that needs ticks/clicks to be filled, mostly convenience)
	{
		result.maxPossibleCombo += c.hitcircles.size();
		for (int i=0; i<c.sliders.size(); i++)
		{
			const SLIDER &s = c.sliders[i];

			const int repeats = std::max((s.repeat - 1), 0);
			result.maxPossibleCombo += 2 + repeats + (repeats+1)*s.ticks.size(); // start/end + repeat arrow + ticks
		}
		result.maxPossibleCombo += c.spinners.size();
	}

	// and generate the difficultyhitobjects
	result.diffobjects.reserve(c.hitcircles.size() + c.sliders.size() + c.spinners.size());

	for (int i=0; i<c.hitcircles.size(); i++)
	{
		result.diffobjects.push_back(OsuDifficultyHitObject(
				OsuDifficultyHitObject::TYPE::CIRCLE,
				Vector2(c.hitcircles[i].x, c.hitcircles[i].y),
				(long)c.hitcircles[i].time));
	}

	const bool calculateSliderCurveInConstructor = (c.sliders.size() < 5000); // NOTE: for explanation see OsuDifficultyHitObject constructor
	for (int i=0; i<c.sliders.size(); i++)
	{
		if (dead.load())
		{
			result.errorCode = 6;
			return result;
		}

		if (!calculateStarsInaccurately)
		{
			result.diffobjects.push_back(OsuDifficultyHitObject(
					OsuDifficultyHitObject::TYPE::SLIDER,
					Vector2(c.sliders[i].x, c.sliders[i].y),
					c.sliders[i].time,
					c.sliders[i].time + (long)c.sliders[i].sliderTime,
					c.sliders[i].sliderTimeWithoutRepeats,
					c.sliders[i].type,
					c.sliders[i].points,
					c.sliders[i].pixelLength,
					c.sliders[i].scoringTimesForStarCalc,
					c.sliders[i].repeat,
					calculateSliderCurveInConstructor));
		}
		else
		{
			result.diffobjects.push_back(OsuDifficultyHitObject(
					OsuDifficultyHitObject::TYPE::SLIDER,
					Vector2(c.sliders[i].x, c.sliders[i].y),
					c.sliders[i].time,
					c.sliders[i].time + (long)c.sliders[i].sliderTime,
					c.sliders[i].sliderTimeWithoutRepeats,
					c.sliders[i].type,
					std::vector<Vector2>(),	// NOTE: ignore curve when calculating inaccurately
					c.sliders[i].pixelLength,
					std::vector<OsuDifficultyHitObject::SLIDER_SCORING_TIME>(),	// NOTE: ignore curve when calculating inaccurately
					c.sliders[i].repeat,
					false));				// NOTE: ignore curve when calculating inaccurately
		}
	}

	for (int i=0; i<c.spinners.size(); i++)
	{
		result.diffobjects.push_back(OsuDifficultyHitObject(
				OsuDifficultyHitObject::TYPE::SPINNER,
				Vector2(c.spinners[i].x, c.spinners[i].y),
				(long)c.spinners[i].time,
				(long)c.spinners[i].endTime));
	}

	// sort hitobjects by time
	struct DiffHitObjectSortComparator
	{
	    bool operator() (const OsuDifficultyHitObject &a, const OsuDifficultyHitObject &b) const
	    {
	    	// strict weak ordering!
	    	if (a.time == b.time)
	    		return a.sortHack < b.sortHack;
	    	else
	    		return a.time < b.time;
	    }
	};
	std::sort(result.diffobjects.begin(), result.diffobjects.end(), DiffHitObjectSortComparator());

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

		const float approachTime = OsuGameRules::getApproachTimeForStacking(finalAR);

		if (c.version > 5)
		{
			// peppy's algorithm
			// https://gist.github.com/peppy/1167470

			for (int i=result.diffobjects.size()-1; i>=0; i--)
			{
				int n = i;

				OsuDifficultyHitObject *objectI = &result.diffobjects[i];

				const bool isSpinner = (objectI->type == OsuDifficultyHitObject::TYPE::SPINNER);

				if (objectI->stack != 0 || isSpinner)
					continue;

				const bool isHitCircle = (objectI->type == OsuDifficultyHitObject::TYPE::CIRCLE);
				const bool isSlider = (objectI->type == OsuDifficultyHitObject::TYPE::SLIDER);

				if (isHitCircle)
				{
					while (--n >= 0)
					{
						OsuDifficultyHitObject *objectN = &result.diffobjects[n];

						const bool isSpinnerN = (objectN->type == OsuDifficultyHitObject::TYPE::SPINNER);

						if (isSpinnerN)
							continue;

						if (objectI->time - (approachTime * c.stackLeniency) > (objectN->endTime))
							break;

						Vector2 objectNEndPosition = objectN->getOriginalRawPosAt(objectN->time + objectN->getDuration());
						if (objectN->getDuration() != 0 && (objectNEndPosition - objectI->getOriginalRawPosAt(objectI->time)).length() < STACK_LENIENCE)
						{
							int offset = objectI->stack - objectN->stack + 1;
							for (int j=n+1; j<=i; j++)
							{
								if ((objectNEndPosition - result.diffobjects[j].getOriginalRawPosAt(result.diffobjects[j].time)).length() < STACK_LENIENCE)
									result.diffobjects[j].stack = (result.diffobjects[j].stack - offset);
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
						OsuDifficultyHitObject *objectN = &result.diffobjects[n];

						const bool isSpinner = (objectN->type == OsuDifficultyHitObject::TYPE::SPINNER);

						if (isSpinner)
							continue;

						if (objectI->time - (approachTime * c.stackLeniency) > objectN->time)
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

			for (int i=0; i<result.diffobjects.size(); i++)
			{
				OsuDifficultyHitObject *currHitObject = &result.diffobjects[i];

				const bool isSlider = (currHitObject->type == OsuDifficultyHitObject::TYPE::SLIDER);

				if (currHitObject->stack != 0 && !isSlider)
					continue;

				long startTime = currHitObject->time + currHitObject->getDuration();
				int sliderStack = 0;

				for (int j=i+1; j<result.diffobjects.size(); j++)
				{
					OsuDifficultyHitObject *objectJ = &result.diffobjects[j];

					if (objectJ->time - (approachTime * c.stackLeniency) > startTime)
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
		float stackOffset = rawHitCircleDiameter / 128.0f / OsuGameRules::broken_gamefield_rounding_allowance * 6.4f;
		for (int i=0; i<result.diffobjects.size(); i++)
		{
			if (dead.load())
			{
				result.errorCode = 6;
				return result;
			}

			if (result.diffobjects[i].stack != 0)
				result.diffobjects[i].updateStackPosition(stackOffset);
		}
	}

	// apply speed multiplier (if present)
	if (speedMultiplier != 1.0f && speedMultiplier > 0.0f)
	{
		const double invSpeedMultiplier = 1.0 / (double)speedMultiplier;
		for (int i=0; i<result.diffobjects.size(); i++)
		{
			if (dead.load())
			{
				result.errorCode = 6;
				return result;
			}

			result.diffobjects[i].time = (long)((double)result.diffobjects[i].time * invSpeedMultiplier);
			result.diffobjects[i].endTime = (long)((double)result.diffobjects[i].endTime * invSpeedMultiplier);

			if (!calculateStarsInaccurately) // NOTE: ignore slider curves when calculating inaccurately
			{
				result.diffobjects[i].spanDuration = (double)result.diffobjects[i].spanDuration * invSpeedMultiplier;
				for (int s=0; s<result.diffobjects[i].scoringTimes.size(); s++)
				{
					result.diffobjects[i].scoringTimes[s].time = (double)result.diffobjects[i].scoringTimes[s].time * invSpeedMultiplier;
				}
			}
		}
	}

	return result;
}

bool OsuDatabaseBeatmap::loadMetadata(OsuDatabaseBeatmap *databaseBeatmap)
{
	if (databaseBeatmap == NULL) return false;
	if (databaseBeatmap->m_difficulties.size() > 0) return false; // we are just a container

	// reset
	databaseBeatmap->m_timingpoints = std::vector<TIMINGPOINT>();

	if (Osu::debug->getBool())
		debugLog("OsuDatabaseBeatmap::loadMetadata() : %s\n", databaseBeatmap->m_sFilePath.toUtf8());

	// generate MD5 hash (loads entire file, very slow)
	databaseBeatmap->m_sMD5Hash.clear();
	{
		File file(!databaseBeatmap->m_bFilePathIsInMemoryBeatmap ? databaseBeatmap->m_sFilePath : "");

		const char *beatmapFile = NULL;
		size_t beatmapFileSize = 0;
		{
			if (!databaseBeatmap->m_bFilePathIsInMemoryBeatmap)
			{
				if (file.canRead())
				{
					beatmapFile = file.readFile();
					beatmapFileSize = file.getFileSize();
				}
			}
			else
			{
				beatmapFile = databaseBeatmap->m_sFilePath.toUtf8();
				beatmapFileSize = databaseBeatmap->m_sFilePath.lengthUtf8();
			}
		}

		if (beatmapFile != NULL)
			databaseBeatmap->m_sMD5Hash = OsuFile::md5((unsigned char*)beatmapFile, beatmapFileSize);
	}

	// open osu file again, but this time for parsing
	bool foundAR = false;
	{
		File file(!databaseBeatmap->m_bFilePathIsInMemoryBeatmap ? databaseBeatmap->m_sFilePath : "");
		if (!file.canRead() && !databaseBeatmap->m_bFilePathIsInMemoryBeatmap)
		{
			debugLog("Osu Error: Couldn't read file %s\n", databaseBeatmap->m_sFilePath.toUtf8());
			return false;
		}

		std::istringstream ss(databaseBeatmap->m_bFilePathIsInMemoryBeatmap ? databaseBeatmap->m_sFilePath.toUtf8() : ""); // eh

		// load metadata only
		int curBlock = -1;
		unsigned long long timingPointSortHack = 0;
		char stringBuffer[1024];
		std::string curLine;
		while (!databaseBeatmap->m_bFilePathIsInMemoryBeatmap ? file.canRead() : static_cast<bool>(std::getline(ss, curLine)))
		{
			UString uCurLine;
			char *curLineChar = NULL;
			{
				if (!databaseBeatmap->m_bFilePathIsInMemoryBeatmap)
				{
					uCurLine = file.readLine();
					curLineChar = (char*)uCurLine.toUtf8();
					curLine = std::string(curLineChar);
				}
				else
					curLineChar = (char*)curLine.c_str();
			}

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
						if (sscanf(curLineChar, " osu file format v %i \n", &databaseBeatmap->m_iVersion) == 1)
						{
							if (databaseBeatmap->m_iVersion > osu_beatmap_version.getInt())
							{
								debugLog("Ignoring unknown/invalid beatmap version %i\n", databaseBeatmap->m_iVersion);
								return false;
							}
						}
					}
					break;

				case 0: // General
					{
						memset(stringBuffer, '\0', 1024);
						if (sscanf(curLineChar, " AudioFilename : %1023[^\n]", stringBuffer) == 1)
						{
							databaseBeatmap->m_sAudioFileName = UString(stringBuffer);
							databaseBeatmap->m_sAudioFileName = databaseBeatmap->m_sAudioFileName.trim();
						}

						sscanf(curLineChar, " StackLeniency : %f \n", &databaseBeatmap->m_fStackLeniency);
						sscanf(curLineChar, " PreviewTime : %i \n", &databaseBeatmap->m_iPreviewTime);
						sscanf(curLineChar, " Mode : %i \n", &databaseBeatmap->m_iGameMode);
					}
					break;

				case 1: // Metadata
					{
						memset(stringBuffer, '\0', 1024);
						if (sscanf(curLineChar, " Title :%1023[^\n]", stringBuffer) == 1)
						{
							databaseBeatmap->m_sTitle = UString(stringBuffer);
							databaseBeatmap->m_sTitle = databaseBeatmap->m_sTitle.trim();
						}

						memset(stringBuffer, '\0', 1024);
						if (sscanf(curLineChar, " Artist :%1023[^\n]", stringBuffer) == 1)
						{
							databaseBeatmap->m_sArtist = UString(stringBuffer);
							databaseBeatmap->m_sArtist = databaseBeatmap->m_sArtist.trim();
						}

						memset(stringBuffer, '\0', 1024);
						if (sscanf(curLineChar, " Creator :%1023[^\n]", stringBuffer) == 1)
						{
							databaseBeatmap->m_sCreator = UString(stringBuffer);
							databaseBeatmap->m_sCreator = databaseBeatmap->m_sCreator.trim();
						}

						memset(stringBuffer, '\0', 1024);
						if (sscanf(curLineChar, " Version :%1023[^\n]", stringBuffer) == 1)
						{
							databaseBeatmap->m_sDifficultyName = UString(stringBuffer);
							databaseBeatmap->m_sDifficultyName = databaseBeatmap->m_sDifficultyName.trim();
						}

						memset(stringBuffer, '\0', 1024);
						if (sscanf(curLineChar, " Source :%1023[^\n]", stringBuffer) == 1)
						{
							databaseBeatmap->m_sSource = UString(stringBuffer);
							databaseBeatmap->m_sSource = databaseBeatmap->m_sSource.trim();
						}

						memset(stringBuffer, '\0', 1024);
						if (sscanf(curLineChar, " Tags :%1023[^\n]", stringBuffer) == 1)
						{
							databaseBeatmap->m_sTags = UString(stringBuffer);
							databaseBeatmap->m_sTags = databaseBeatmap->m_sTags.trim();
						}

						sscanf(curLineChar, " BeatmapID : %ld \n", &databaseBeatmap->m_iID);
						sscanf(curLineChar, " BeatmapSetID : %i \n", &databaseBeatmap->m_iSetID);
					}
					break;

				case 2: // Difficulty
					{
						sscanf(curLineChar, " CircleSize : %f \n", &databaseBeatmap->m_fCS);
						if (sscanf(curLineChar, " ApproachRate : %f \n", &databaseBeatmap->m_fAR) == 1)
							foundAR = true;

						sscanf(curLineChar, " HPDrainRate : %f \n", &databaseBeatmap->m_fHP);
						sscanf(curLineChar, " OverallDifficulty : %f \n", &databaseBeatmap->m_fOD);
						sscanf(curLineChar, " SliderMultiplier : %f \n", &databaseBeatmap->m_fSliderMultiplier);
						sscanf(curLineChar, " SliderTickRate : %f \n", &databaseBeatmap->m_fSliderTickRate);
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
								databaseBeatmap->m_sBackgroundImageFileName = UString(stringBuffer);
								databaseBeatmap->m_sFullBackgroundImageFilePath = databaseBeatmap->m_sFolder;
								databaseBeatmap->m_sFullBackgroundImageFilePath.append(databaseBeatmap->m_sBackgroundImageFileName);
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
							databaseBeatmap->m_timingpoints.push_back(t);
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
							databaseBeatmap->m_timingpoints.push_back(t);
						}
					}
					break;
				}
			}
		}
	}

	// gamemode filter
	if ((databaseBeatmap->m_iGameMode != 0 && databaseBeatmap->m_osu->getGamemode() == Osu::GAMEMODE::STD) || (databaseBeatmap->m_iGameMode != 0x03 && databaseBeatmap->m_osu->getGamemode() == Osu::GAMEMODE::MANIA))
		return false; // nothing more to do here

	// general sanity checks
	if ((databaseBeatmap->m_timingpoints.size() < 1))
	{
		if (Osu::debug->getBool())
			debugLog("OsuDatabaseBeatmap::loadMetadata() : no timingpoints in beatmap!\n");

		return false; // nothing more to do here
	}

	// build sound file path
	databaseBeatmap->m_sFullSoundFilePath = databaseBeatmap->m_sFolder;
	databaseBeatmap->m_sFullSoundFilePath.append(databaseBeatmap->m_sAudioFileName);

	// sort timingpoints and calculate BPM range
	if (databaseBeatmap->m_timingpoints.size() > 0)
	{
		if (Osu::debug->getBool())
			debugLog("OsuDatabaseBeatmap::loadMetadata() : calculating BPM range ...\n");

		// sort timingpoints by time
		std::sort(databaseBeatmap->m_timingpoints.begin(), databaseBeatmap->m_timingpoints.end(), TimingPointSortComparator());

		// calculate bpm range
		float tempMinBPM = 0.0f;
		float tempMaxBPM = std::numeric_limits<float>::max();
		std::vector<TIMINGPOINT> uninheritedTimingpoints;
		for (int i=0; i<databaseBeatmap->m_timingpoints.size(); i++)
		{
			const TIMINGPOINT &t = databaseBeatmap->m_timingpoints[i];

			if (t.msPerBeat >= 0.0f) // NOT inherited
			{
				uninheritedTimingpoints.push_back(t);

				if (t.msPerBeat > tempMinBPM)
					tempMinBPM = t.msPerBeat;
				if (t.msPerBeat < tempMaxBPM)
					tempMaxBPM = t.msPerBeat;
			}
		}

		// convert from msPerBeat to BPM
		const float msPerMinute = 1.0f * 60.0f * 1000.0f;
		if (tempMinBPM != 0.0f)
			tempMinBPM = msPerMinute / tempMinBPM;
		if (tempMaxBPM != 0.0f)
			tempMaxBPM = msPerMinute / tempMaxBPM;

		databaseBeatmap->m_iMinBPM = (int)std::round(tempMinBPM);
		databaseBeatmap->m_iMaxBPM = (int)std::round(tempMaxBPM);

		struct MostCommonBPMHelper
		{
			static int calculateMostCommonBPM(const std::vector<OsuDatabaseBeatmap::TIMINGPOINT> &uninheritedTimingpoints, long lastTime)
			{
				if (uninheritedTimingpoints.size() < 1) return 0;

				struct Tuple
				{
					float beatLength;
					long duration;

					size_t sortHack;
				};

				// "Construct a set of (beatLength, duration) tuples for each individual timing point."
				std::vector<Tuple> tuples;
				tuples.reserve(uninheritedTimingpoints.size());
				for (size_t i=0; i<uninheritedTimingpoints.size(); i++)
				{
					const OsuDatabaseBeatmap::TIMINGPOINT &t = uninheritedTimingpoints[i];

					Tuple tuple;
					{
						if (t.offset > lastTime)
						{
							tuple.beatLength = std::round(t.msPerBeat * 1000.0f) / 1000.0f;
							tuple.duration = 0;
						}
						else
						{
							// "osu-stable forced the first control point to start at 0."
							// "This is reproduced here to maintain compatibility around osu!mania scroll speed and song select display."
							const long currentTime = (i == 0 ? 0 : t.offset);
							const long nextTime = (i >= uninheritedTimingpoints.size() - 1 ? lastTime : uninheritedTimingpoints[i + 1].offset);

							tuple.beatLength = std::round(t.msPerBeat * 1000.0f) / 1000.0f;
							tuple.duration = std::max(nextTime - currentTime, (long)0);
						}

						tuple.sortHack = i;
					}
					tuples.push_back(tuple);
				}

				// "Aggregate durations into a set of (beatLength, duration) tuples for each beat length"
				std::vector<Tuple> aggregations;
				aggregations.reserve(tuples.size());
				for (size_t i=0; i<tuples.size(); i++)
				{
					const Tuple &t = tuples[i];

					bool foundExistingAggregation = false;
					size_t aggregationIndex = 0;
					for (size_t j=0; j<aggregations.size(); j++)
					{
						if (aggregations[j].beatLength == t.beatLength)
						{
							foundExistingAggregation = true;
							aggregationIndex = j;
							break;
						}
					}

					if (!foundExistingAggregation)
						aggregations.push_back(t);
					else
						aggregations[aggregationIndex].duration += t.duration;
				}

				// "Get the most common one, or 0 as a suitable default"
				struct SortByDuration
				{
				    bool operator() (Tuple const &a, Tuple const &b) const
				    {
				    	// first condition: duration
				    	// second condition: if duration is the same, higher BPM goes before lower BPM

				    	// strict weak ordering!
				    	if (a.duration == b.duration && a.beatLength == b.beatLength)
				    		return a.sortHack > b.sortHack;
				    	else if (a.duration == b.duration)
				    		return (a.beatLength < b.beatLength);
				    	else
				    		return (a.duration > b.duration);
				    }
				};
				std::sort(aggregations.begin(), aggregations.end(), SortByDuration());

				float mostCommonBPM = aggregations[0].beatLength;
				{
					// convert from msPerBeat to BPM
					const float msPerMinute = 1.0f * 60.0f * 1000.0f;
					if (mostCommonBPM != 0.0f)
						mostCommonBPM = msPerMinute / mostCommonBPM;
				}
				return (int)std::round(mostCommonBPM);
			}
		};
		databaseBeatmap->m_iMostCommonBPM = MostCommonBPMHelper::calculateMostCommonBPM(uninheritedTimingpoints, databaseBeatmap->m_timingpoints[databaseBeatmap->m_timingpoints.size() - 1].offset);
	}

	// special case: old beatmaps have AR = OD, there is no ApproachRate stored
	if (!foundAR)
		databaseBeatmap->m_fAR = databaseBeatmap->m_fOD;

	return true;
}

OsuDatabaseBeatmap::LOAD_GAMEPLAY_RESULT OsuDatabaseBeatmap::loadGameplay(OsuDatabaseBeatmap *databaseBeatmap, OsuBeatmap *beatmap)
{
	LOAD_GAMEPLAY_RESULT result = LOAD_GAMEPLAY_RESULT();

	// NOTE: reload metadata (force ensures that all necessary data is ready for creating hitobjects and playing etc., also if beatmap file is changed manually in the meantime)
	if (!loadMetadata(databaseBeatmap))
	{
		result.errorCode = 1;
		return result;
	}

	// load primitives, put in temporary container
	PRIMITIVE_CONTAINER c = loadPrimitiveObjects(databaseBeatmap->m_sFilePath, databaseBeatmap->m_osu->getGamemode(), databaseBeatmap->m_bFilePathIsInMemoryBeatmap);
	if (c.errorCode != 0)
	{
		result.errorCode = c.errorCode;
		return result;
	}
	result.breaks = std::move(c.breaks);
	result.combocolors = std::move(c.combocolors);

	// override some values with data from primitive load, even though they should already be loaded from metadata (sanity)
	databaseBeatmap->m_timingpoints.swap(c.timingpoints);
	databaseBeatmap->m_fSliderMultiplier = c.sliderMultiplier;
	databaseBeatmap->m_fSliderTickRate = c.sliderTickRate;
	databaseBeatmap->m_fStackLeniency = c.stackLeniency;
	databaseBeatmap->m_iVersion = c.version;

	// check if we have any timingpoints at all
	if (databaseBeatmap->m_timingpoints.size() == 0)
	{
		result.errorCode = 3;
		return result;
	}

	// update numObjects
	databaseBeatmap->m_iNumObjects = c.hitcircles.size() + c.sliders.size() + c.spinners.size();
	databaseBeatmap->m_iNumCircles = c.hitcircles.size();
	databaseBeatmap->m_iNumSliders = c.sliders.size();
	databaseBeatmap->m_iNumSpinners = c.spinners.size();

	// check if we have any hitobjects at all
	if (databaseBeatmap->m_iNumObjects < 1)
	{
		result.errorCode = 4;
		return result;
	}

	// calculate sliderTimes, and build slider clicks and ticks
	CALCULATE_SLIDER_TIMES_CLICKS_TICKS_RESULT sliderTimeCalcResult = calculateSliderTimesClicksTicks(c.version, c.sliders, databaseBeatmap->m_timingpoints, databaseBeatmap->m_fSliderMultiplier, databaseBeatmap->m_fSliderTickRate);
	if (sliderTimeCalcResult.errorCode != 0)
	{
		result.errorCode = sliderTimeCalcResult.errorCode;
		return result;
	}

	// build hitobjects from the primitive data we loaded from the osu file
	OsuBeatmapStandard *beatmapStandard = dynamic_cast<OsuBeatmapStandard*>(beatmap);
	OsuBeatmapMania *beatmapMania = dynamic_cast<OsuBeatmapMania*>(beatmap);
	{
		struct Helper
		{
			static inline uint32_t pcgHash(uint32_t input)
			{
				const uint32_t state = input * 747796405u + 2891336453u;
				const uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
				return (word >> 22u) ^ word;
			}
		};

		result.randomSeed = (osu_mod_random_seed.getInt() == 0 ? rand() : osu_mod_random_seed.getInt());

		if (beatmapStandard != NULL)
		{
			// also calculate max possible combo
			int maxPossibleCombo = 0;

			for (size_t i=0; i<c.hitcircles.size(); i++)
			{
				HITCIRCLE &h = c.hitcircles[i];

				if (osu_mod_random.getBool())
				{
					h.x = clamp<int>(h.x - (int)(((Helper::pcgHash(result.randomSeed + h.x) % OsuGameRules::OSU_COORD_WIDTH) / 8.0f) * osu_mod_random_circle_offset_x_percent.getFloat()), 0, OsuGameRules::OSU_COORD_WIDTH);
					h.y = clamp<int>(h.y - (int)(((Helper::pcgHash(result.randomSeed + h.y) % OsuGameRules::OSU_COORD_HEIGHT) / 8.0f) * osu_mod_random_circle_offset_y_percent.getFloat()), 0, OsuGameRules::OSU_COORD_HEIGHT);
				}

				result.hitobjects.push_back(new OsuCircle(h.x, h.y, h.time, h.sampleType, h.number, false, h.colorCounter, h.colorOffset, beatmapStandard));

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
			maxPossibleCombo += c.hitcircles.size();

			for (size_t i=0; i<c.sliders.size(); i++)
			{
				SLIDER &s = c.sliders[i];

				if (osu_mod_strict_tracking.getBool() && osu_mod_strict_tracking_remove_slider_ticks.getBool())
					s.ticks.clear();

				if (osu_mod_random.getBool())
				{
					for (int p=0; p<s.points.size(); p++)
					{
						s.points[p].x = clamp<int>(s.points[p].x - (int)(((Helper::pcgHash(result.randomSeed + s.points[p].x) % OsuGameRules::OSU_COORD_WIDTH) / 3.0f) * osu_mod_random_slider_offset_x_percent.getFloat()), 0, OsuGameRules::OSU_COORD_WIDTH);
						s.points[p].y = clamp<int>(s.points[p].y - (int)(((Helper::pcgHash(result.randomSeed + s.points[p].y) % OsuGameRules::OSU_COORD_HEIGHT) / 3.0f) * osu_mod_random_slider_offset_y_percent.getFloat()), 0, OsuGameRules::OSU_COORD_HEIGHT);
					}
				}

				if (osu_mod_reverse_sliders.getBool())
					std::reverse(s.points.begin(), s.points.end());

				result.hitobjects.push_back(new OsuSlider(s.type, s.repeat, s.pixelLength, s.points, s.hitSounds, s.ticks, s.sliderTime, s.sliderTimeWithoutRepeats, s.time, s.sampleType, s.number, false, s.colorCounter, s.colorOffset, beatmapStandard));

				const int repeats = std::max((s.repeat - 1), 0);
				maxPossibleCombo += 2 + repeats + (repeats+1)*s.ticks.size(); // start/end + repeat arrow + ticks
			}

			for (size_t i=0; i<c.spinners.size(); i++)
			{
				SPINNER &s = c.spinners[i];

				if (osu_mod_random.getBool())
				{
					s.x = clamp<int>(s.x - (int)(((Helper::pcgHash(result.randomSeed + s.x) % OsuGameRules::OSU_COORD_WIDTH) / 1.25f) * (Helper::pcgHash(result.randomSeed + s.x) % 2 == 0 ? 1.0f : -1.0f) * osu_mod_random_spinner_offset_x_percent.getFloat()), 0, OsuGameRules::OSU_COORD_WIDTH);
					s.y = clamp<int>(s.y - (int)(((Helper::pcgHash(result.randomSeed + s.y) % OsuGameRules::OSU_COORD_HEIGHT) / 1.25f) * (Helper::pcgHash(result.randomSeed + s.y) % 2 == 0 ? 1.0f : -1.0f) * osu_mod_random_spinner_offset_y_percent.getFloat()), 0, OsuGameRules::OSU_COORD_HEIGHT);
				}

				result.hitobjects.push_back(new OsuSpinner(s.x, s.y, s.time, s.sampleType, false, s.endTime, beatmapStandard));
			}
			maxPossibleCombo += c.spinners.size();

			beatmapStandard->setMaxPossibleCombo(maxPossibleCombo);

			// debug
			if (m_osu_debug_pp_ref->getBool())
			{
				const UString &osuFilePath = databaseBeatmap->m_sFilePath;
				const Osu::GAMEMODE gameMode = Osu::GAMEMODE::STD;
				const float AR = beatmap->getAR();
				const float CS = beatmap->getCS();
				const float OD = beatmap->getOD();
				const float speedMultiplier = databaseBeatmap->m_osu->getSpeedMultiplier(); // NOTE: not this->getSpeedMultiplier()!
				const bool relax = databaseBeatmap->m_osu->getModRelax();
				const bool touchDevice = databaseBeatmap->m_osu->getModTD();

				LOAD_DIFFOBJ_RESULT diffres = OsuDatabaseBeatmap::loadDifficultyHitObjects(osuFilePath, gameMode, AR, CS, speedMultiplier);

				double aim = 0.0;
				double aimSliderFactor = 0.0;
				double aimDifficultStrains = 0.0;
				double speed = 0.0;
				double speedNotes = 0.0;
				double speedDifficultStrains = 0.0;
				double stars = OsuDifficultyCalculator::calculateStarDiffForHitObjects(diffres.diffobjects, CS, OD, speedMultiplier, relax, touchDevice, &aim, &aimSliderFactor, &aimDifficultStrains, &speed, &speedNotes, &speedDifficultStrains);
				double pp = OsuDifficultyCalculator::calculatePPv2(beatmap->getOsu(), beatmap, aim, aimSliderFactor, aimDifficultStrains, speed, speedNotes, speedDifficultStrains, databaseBeatmap->m_iNumObjects, databaseBeatmap->m_iNumCircles, databaseBeatmap->m_iNumSliders, databaseBeatmap->m_iNumSpinners, maxPossibleCombo);

				engine->showMessageInfo("PP", UString::format("pp = %f, stars = %f, aimstars = %f, speedstars = %f, %i circles, %i sliders, %i spinners, %i hitobjects, maxcombo = %i", pp, stars, aim, speed, databaseBeatmap->m_iNumCircles, databaseBeatmap->m_iNumSliders, databaseBeatmap->m_iNumSpinners, databaseBeatmap->m_iNumObjects, maxPossibleCombo));
			}
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
				result.hitobjects.push_back(new OsuManiaNote(ManiaHelper::getColumn(availableColumns, h.x), h.maniaEndTime > 0 ? (h.maniaEndTime - h.time) : 0, h.time, h.sampleType, h.number, h.colorCounter, beatmapMania));
			}

			for (int i=0; i<c.sliders.size(); i++)
			{
				SLIDER &s = c.sliders[i];
				result.hitobjects.push_back(new OsuManiaNote(ManiaHelper::getColumn(availableColumns, s.x), s.sliderTime, s.time, s.sampleType, s.number, s.colorCounter, beatmapMania));
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
	std::sort(result.hitobjects.begin(), result.hitobjects.end(), HitObjectSortComparator());

	// update beatmap length stat
	if (databaseBeatmap->m_iLengthMS == 0 && result.hitobjects.size() > 0)
		databaseBeatmap->m_iLengthMS = result.hitobjects[result.hitobjects.size() - 1]->getTime() + result.hitobjects[result.hitobjects.size() - 1]->getDuration();

	// set isEndOfCombo + precalculate Score v2 combo portion maximum
	if (beatmapStandard != NULL)
	{
		unsigned long long scoreV2ComboPortionMaximum = 1;

		if (result.hitobjects.size() > 0)
			scoreV2ComboPortionMaximum = 0;

		int combo = 0;
		for (size_t i=0; i<result.hitobjects.size(); i++)
		{
			OsuHitObject *currentHitObject = result.hitobjects[i];
			const OsuHitObject *nextHitObject = (i + 1 < result.hitobjects.size() ? result.hitobjects[i + 1] : NULL);

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
	if (osu_show_approach_circle_on_first_hidden_object.getBool())
	{
		if (result.hitobjects.size() > 0)
			result.hitobjects[0]->setForceDrawApproachCircle(true);
	}

	// custom override for forcing a hard number cap and/or sequence (visually only)
	// NOTE: this is done after we have already calculated/set isEndOfCombos
	{
		if (osu_ignore_beatmap_combo_numbers.getBool())
		{
			// NOTE: spinners don't increment the combo number
			int comboNumber = 1;
			for (size_t i=0; i<result.hitobjects.size(); i++)
			{
				OsuHitObject *currentHitObject = result.hitobjects[i];

				const OsuSpinner *spinnerPointer = dynamic_cast<OsuSpinner*>(currentHitObject);

				if (spinnerPointer == NULL)
				{
					currentHitObject->setComboNumber(comboNumber);
					comboNumber++;
				}
			}
		}

		const int numberMax = osu_number_max.getInt();
		if (numberMax > 0)
		{
			for (size_t i=0; i<result.hitobjects.size(); i++)
			{
				OsuHitObject *currentHitObject = result.hitobjects[i];

				const int currentComboNumber = currentHitObject->getComboNumber();
				const int newComboNumber = (currentComboNumber % numberMax);

				currentHitObject->setComboNumber(newComboNumber == 0 ? numberMax : newComboNumber);
			}
		}
	}

	debugLog("OsuDatabaseBeatmap::load() loaded %i hitobjects\n", result.hitobjects.size());

	return result;
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
		m_fStarsNomod = 0.0f;
		m_iMinBPM = std::numeric_limits<int>::max();
		m_iMaxBPM = 0;
		m_iMostCommonBPM = 0;
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
			if (m_difficulties[i]->getMinBPM() < m_iMinBPM)
				m_iMinBPM = m_difficulties[i]->getMinBPM();
			if (m_difficulties[i]->getMaxBPM() > m_iMaxBPM)
				m_iMaxBPM = m_difficulties[i]->getMaxBPM();
			if (m_difficulties[i]->getMostCommonBPM() > m_iMostCommonBPM)
				m_iMostCommonBPM = m_difficulties[i]->getMostCommonBPM();
			if (m_difficulties[i]->getLastModificationTime() > m_iLastModificationTime)
				m_iLastModificationTime = m_difficulties[i]->getLastModificationTime();
		}
	}
}

void OsuDatabaseBeatmap::updateSetHeuristics()
{
	setDifficulties(m_difficulties);
}

OsuDatabaseBeatmap::TIMING_INFO OsuDatabaseBeatmap::getTimingInfoForTime(unsigned long positionMS)
{
	return getTimingInfoForTimeAndTimingPoints(positionMS, m_timingpoints);
}

OsuDatabaseBeatmap::TIMING_INFO OsuDatabaseBeatmap::getTimingInfoForTimeAndTimingPoints(unsigned long positionMS, std::vector<TIMINGPOINT> &timingpoints)
{
	TIMING_INFO ti;
	ti.offset = 0;
	ti.beatLengthBase = 1;
	ti.beatLength = 1;
	ti.volume = 100;
	ti.sampleType = 0;
	ti.sampleSet = 0;
	ti.isNaN = false;

	if (timingpoints.size() < 1) return ti;

	// initial values
	ti.offset = timingpoints[0].offset;
	ti.volume = timingpoints[0].volume;
	ti.sampleSet = timingpoints[0].sampleSet;
	ti.sampleType = timingpoints[0].sampleType;

	// new (peppy's algorithm)
	// (correctly handles aspire & NaNs)
	{
		const bool allowMultiplier = true;

		int point = 0;
		int samplePoint = 0;
		int audioPoint = 0;

		for (int i=0; i<timingpoints.size(); i++)
		{
			if (timingpoints[i].offset <= positionMS)
			{
				audioPoint = i;

				if (timingpoints[i].timingChange)
					point = i;
				else
					samplePoint = i;
			}
		}

		double mult = 1;

		if (allowMultiplier && samplePoint > point && timingpoints[samplePoint].msPerBeat < 0)
		{
			if (timingpoints[samplePoint].msPerBeat >= 0)
				mult = 1;
			else
				mult = clamp<float>((float)-timingpoints[samplePoint].msPerBeat, 10.0f, 1000.0f) / 100.0f;
		}

		ti.beatLengthBase = timingpoints[point].msPerBeat;
		ti.offset = timingpoints[point].offset;

		ti.isNaN = std::isnan(timingpoints[samplePoint].msPerBeat) || std::isnan(timingpoints[point].msPerBeat);
		ti.beatLength = ti.beatLengthBase * mult;

		ti.volume = timingpoints[audioPoint].volume;
		ti.sampleType = timingpoints[audioPoint].sampleType;
		ti.sampleSet = timingpoints[audioPoint].sampleSet;
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
	m_bDead = true; // NOTE: start dead! need to revive() before use

	m_diff2 = NULL;

	m_fAR = 5.0f;
	m_fCS = 5.0f;
	m_fOD = 5.0f;
	m_fSpeedMultiplier = 1.0f;
	m_bRelax = false;
	m_bTouchDevice = false;

	m_totalStars = 0.0;
	m_aimStars = 0.0;
	m_aimSliderFactor = 0.0;
	m_aimDifficultStrains = 0.0;
	m_speedStars = 0.0;
	m_speedNotes = 0.0;
	m_speedDifficultStrains = 0.0;
	m_pp = 0.0;

	m_iLengthMS = 0;

	m_iErrorCode = 0;
	m_iNumObjects = 0;
	m_iNumCircles = 0;
	m_iNumSliders = 0;
	m_iNumSpinners = 0;
	m_iMaxPossibleCombo = 0;
}

void OsuDatabaseBeatmapStarCalculator::init()
{
	// NOTE: this accesses runtime mods, so must be run sync (not async)
	// technically the getSelectedBeatmap() call here is a bit unsafe, since the beatmap could have changed already between async and sync, but in that case we recalculate immediately after anyways
	if (!m_bDead.load() && m_iErrorCode == 0 && m_diff2->m_osu->getSelectedBeatmap() != NULL)
		m_pp = OsuDifficultyCalculator::calculatePPv2(m_diff2->m_osu, m_diff2->m_osu->getSelectedBeatmap(), m_aimStars.load(), m_aimSliderFactor.load(), m_aimDifficultStrains.load(), m_speedStars.load(), m_speedNotes.load(), m_speedDifficultStrains.load(), m_iNumObjects.load(), m_iNumCircles.load(), m_iNumSliders.load(), m_iNumSpinners.load(), m_iMaxPossibleCombo);

	m_bReady = true;
}

void OsuDatabaseBeatmapStarCalculator::initAsync()
{
	// sanity reset
	m_totalStars = 0.0;
	m_aimStars = 0.0;
	m_aimSliderFactor = 0.0;
	m_aimDifficultStrains = 0.0;
	m_speedStars = 0.0;
	m_speedNotes = 0.0;
	m_speedDifficultStrains = 0.0;
	m_pp = 0.0;

	m_iLengthMS = 0;

	const Osu::GAMEMODE gameMode = Osu::GAMEMODE::STD;
	OsuDatabaseBeatmap::LOAD_DIFFOBJ_RESULT diffres = OsuDatabaseBeatmap::loadDifficultyHitObjects(m_sFilePath, gameMode, m_fAR, m_fCS, m_fSpeedMultiplier, false, m_bDead);
	m_iErrorCode = diffres.errorCode;

	if (m_iErrorCode == 0)
	{
		m_iNumObjects = diffres.diffobjects.size();
		{
			m_iNumCircles = 0;
			m_iNumSliders = 0;
			m_iNumSpinners = 0;
			for (size_t i=0; i<diffres.diffobjects.size(); i++)
			{
				if (diffres.diffobjects[i].type == OsuDifficultyHitObject::TYPE::CIRCLE)
					m_iNumCircles++;
				if (diffres.diffobjects[i].type == OsuDifficultyHitObject::TYPE::SLIDER)
					m_iNumSliders++;
				if (diffres.diffobjects[i].type == OsuDifficultyHitObject::TYPE::SPINNER)
					m_iNumSpinners++;
			}
		}
		m_iMaxPossibleCombo = diffres.maxPossibleCombo;

		double aimStars = 0.0;
		double aimSliderFactor = 0.0;
		double aimDifficultStrains = 0.0;
		double speedStars = 0.0;
		double speedNotes = 0.0;
		double speedDifficultStrains = 0.0;
		m_totalStars = OsuDifficultyCalculator::calculateStarDiffForHitObjects(diffres.diffobjects, m_fCS, m_fOD, m_fSpeedMultiplier, m_bRelax, m_bTouchDevice, &aimStars, &aimSliderFactor, &aimDifficultStrains, &speedStars, &speedNotes, &speedDifficultStrains, -1, &m_aimStrains, &m_speedStrains, m_bDead);
		m_aimStars = aimStars;
		m_aimSliderFactor = aimSliderFactor;
		m_aimDifficultStrains = aimDifficultStrains;
		m_speedStars = speedStars;
		m_speedNotes = speedNotes;
		m_speedDifficultStrains = speedDifficultStrains;

		// NOTE: this matches osu, i.e. it is the time from the start of the music track until the end of the last hitobject (including all breaks and initial skippable sections!)
		if (diffres.diffobjects.size() > 0)
			m_iLengthMS = (long)((float)std::max((diffres.diffobjects[diffres.diffobjects.size() - 1].endTime), (long)0) * m_fSpeedMultiplier); // NOTE: reverse compensate for diffobj speed compensation in order to get actual length
	}

	m_bAsyncReady = true;
}

void OsuDatabaseBeatmapStarCalculator::setBeatmapDifficulty(OsuDatabaseBeatmap *diff2, float AR, float CS, float OD, float speedMultiplier, bool relax, bool touchDevice)
{
	m_diff2 = diff2;

	m_sFilePath = diff2->getFilePath();

	m_fAR = AR;
	m_fCS = CS;
	m_fOD = OD;
	m_fSpeedMultiplier = speedMultiplier;
	m_bRelax = relax;
	m_bTouchDevice = touchDevice;
}
