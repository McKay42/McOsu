//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		beatmap file loader and container (also generates hitobjects)
//
// $NoKeywords: $osudiff
//===============================================================================//

#include "OsuBeatmapDifficulty.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "ConVar.h"
#include "File.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuGameRules.h"
#include "OsuBeatmapStandard.h"
#include "OsuBeatmapMania.h"
#include "OsuNotificationOverlay.h"

#include "OsuHitObject.h"
#include "OsuCircle.h"
#include "OsuSlider.h"
#include "OsuSpinner.h"
#include "OsuManiaNote.h"

ConVar osu_mod_random("osu_mod_random", false);
ConVar osu_show_approach_circle_on_first_hidden_object("osu_show_approach_circle_on_first_hidden_object", true);
ConVar osu_load_beatmap_background_images("osu_load_beatmap_background_images", true);



class OsuManiaHelper
{
public:
	static int getColumn(int availableColumns, float position, bool allowSpecial = false)
	{
		if (allowSpecial && availableColumns == 8)
		{
			const float local_x_divisor = 512.0f / 7;
			return clamp<int>((int)std::floor(position / local_x_divisor), 0, 6) + 1;
		}

		float localXDivisor = 512.0f / availableColumns;
		return clamp<int>((int)std::floor(position / localXDivisor), 0, availableColumns - 1);
	}

private:
};



class BackgroundImagePathLoader : public Resource
{
public:
	BackgroundImagePathLoader(OsuBeatmapDifficulty *diff) : Resource()
	{
		m_diff = diff;

		m_bDead = false;

		m_bAsyncReady = false;
		m_bReady = false;
	};

	void kill() {m_bDead = true;}

protected:
	virtual void init()
	{
		m_bReady = true; // this is up here on purpose
		if (m_bDead) return;

		if (m_diff->shouldBackgroundImageBeLoaded()) // check if we still really need to load the image (could have changed while loading the path)
			m_diff->loadBackgroundImage(); // this immediately deletes us, which is allowed since the init() function is the last event and nothing happens after the call in here
		else
			m_diff->deleteBackgroundImagePathLoader(); // same here
	}
	virtual void initAsync()
	{
		if (m_bDead)
		{
			m_bAsyncReady = true;
			return;
		}

		if (OsuBeatmapDifficulty::m_osu_database_dynamic_star_calculation->getBool())
			m_diff->loadMetadataRaw(true); // for stars

		m_diff->loadBackgroundImagePath();
		m_bAsyncReady = true;
	}
	virtual void destroy() {;}

private:
	OsuBeatmapDifficulty *m_diff;
	bool m_bDead;
};



struct TimingPointSortComparator
{
    bool operator() (OsuBeatmapDifficulty::TIMINGPOINT const &a, OsuBeatmapDifficulty::TIMINGPOINT const &b) const
    {
    	// first condition: offset
    	// second condition: if offset is the same, non-inherited timingpoints go before inherited timingpoints

    	// strict weak ordering!
    	if (a.offset == b.offset && ((a.msPerBeat >= 0 && b.msPerBeat < 0) == (b.msPerBeat >= 0 && a.msPerBeat < 0)))
    		return a.sortHack < b.sortHack;
    	else
    		return (a.offset < b.offset) || (a.offset == b.offset && a.msPerBeat >= 0 && b.msPerBeat < 0);
    }
};



ConVar *OsuBeatmapDifficulty::m_osu_slider_scorev2 = NULL;
ConVar *OsuBeatmapDifficulty::m_osu_draw_statistics_pp = NULL;
ConVar *OsuBeatmapDifficulty::m_osu_debug_pp = NULL;
ConVar *OsuBeatmapDifficulty::m_osu_database_dynamic_star_calculation = NULL;
unsigned long long OsuBeatmapDifficulty::sortHackCounter = 0;

OsuBeatmapDifficulty::OsuBeatmapDifficulty(Osu *osu, UString filepath, UString folder)
{
	m_osu = osu;

	loaded = false;
	m_sFilePath = filepath;
	m_sFolder = folder;
	m_bShouldBackgroundImageBeLoaded = false;

	// convar refs
	if (m_osu_slider_scorev2 == NULL)
		m_osu_slider_scorev2 = convar->getConVarByName("osu_slider_scorev2");
	if (m_osu_draw_statistics_pp == NULL)
		m_osu_draw_statistics_pp = convar->getConVarByName("osu_draw_statistics_pp");
	if (m_osu_debug_pp == NULL)
		m_osu_debug_pp = convar->getConVarByName("osu_debug_pp");
	if (m_osu_database_dynamic_star_calculation == NULL)
		m_osu_database_dynamic_star_calculation = convar->getConVarByName("osu_database_dynamic_star_calculation");

	// default values
	stackLeniency = 0.7f;

	beatmapId = -1;

	CS = 5;
	AR = 5;
	OD = 5;
	HP = 5;

	sliderMultiplier = 1;
	sliderTickRate = 1;

	previewTime = 0;
	lastModificationTime = 0;
	mode = 0;
	lengthMS = 0;

	backgroundImage = NULL;
	localoffset = 0;
	onlineOffset = 0;
	minBPM = 0;
	maxBPM = 0;
	numObjects = 0;
	starsNoMod = 0.0f;
	ID = 0;
	setID = 0;

	m_backgroundImagePathLoader = NULL;

	m_iMaxCombo = 0;
	m_fScoreV2ComboPortionMaximum = 1;

	m_iSortHack = sortHackCounter++;
}

OsuBeatmapDifficulty::~OsuBeatmapDifficulty()
{
	if (m_backgroundImagePathLoader != NULL)
	{
		m_backgroundImagePathLoader->kill();
		engine->getResourceManager()->destroyResource(m_backgroundImagePathLoader);
	}
	unloadBackgroundImage();
}

void OsuBeatmapDifficulty::unload()
{
	loaded = false;

	hitcircles = std::vector<HITCIRCLE>();
	sliders = std::vector<SLIDER>();
	spinners = std::vector<SPINNER>();
	breaks = std::vector<BREAK>();
	///timingpoints = std::vector<TIMINGPOINT>(); // currently commented for main menu button animation

	m_aimStarsForNumHitObjects = std::vector<double>();
	m_speedStarsForNumHitObjects = std::vector<double>();
}

bool OsuBeatmapDifficulty::loadMetadataRaw(bool calculateStars)
{
	unload();

	if (Osu::debug->getBool())
		debugLog("OsuBeatmapDifficulty::loadMetadata() : %s\n", m_sFilePath.toUtf8());

	// open osu file
	File file(m_sFilePath);
	if (!file.canRead())
	{
		debugLog("Osu Error: Couldn't fopen() file %s\n", m_sFilePath.toUtf8());
		return false;
	}

	// load metadata only
	int curBlock = -1;
	bool foundAR = false;
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
			else if (curLine.find("[Colours]") != std::string::npos)
				curBlock = 4;
			else if (curLine.find("[TimingPoints]") != std::string::npos)
				curBlock = 5;
			else if (curLine.find("[HitObjects]") != std::string::npos)
			{
				if (calculateStars)
					curBlock = 6; // star calculation needs hitobjects
				else
					break; // stop early otherwise
			}

			switch (curBlock)
			{
			case 0: // General
				{
					memset(stringBuffer, '\0', 1024);
					if (sscanf(curLineChar, " AudioFilename : %1023[^\n]", stringBuffer) == 1)
					{
						audioFileName = UString(stringBuffer);
						audioFileName = audioFileName.trim();
					}

					sscanf(curLineChar, " StackLeniency : %f \n", &stackLeniency);
					sscanf(curLineChar, " PreviewTime : %lu \n", &previewTime);
					sscanf(curLineChar, " Mode : %i \n", &mode);
				}
				break;

			case 1: // Metadata
				{
					memset(stringBuffer, '\0', 1024);
					if (sscanf(curLineChar, " Title :%1023[^\n]", stringBuffer) == 1)
					{
						title = UString(stringBuffer);
						title = title.trim();
					}

					memset(stringBuffer, '\0', 1024);
					if (sscanf(curLineChar, " Artist :%1023[^\n]", stringBuffer) == 1)
					{
						artist = UString(stringBuffer);
						artist = artist.trim();
					}

					memset(stringBuffer, '\0', 1024);
					if (sscanf(curLineChar, " Creator :%1023[^\n]", stringBuffer) == 1)
					{
						creator = UString(stringBuffer);
						creator = creator.trim();
					}

					memset(stringBuffer, '\0', 1024);
					if (sscanf(curLineChar, " Version :%1023[^\n]", stringBuffer) == 1)
					{
						name = UString(stringBuffer);
						name = name.trim();
					}

					memset(stringBuffer, '\0', 1024);
					if (sscanf(curLineChar, " Source :%1023[^\n]", stringBuffer) == 1)
					{
						source = UString(stringBuffer);
						source = source.trim();
					}

					memset(stringBuffer, '\0', 1024);
					if (sscanf(curLineChar, " Tags :%1023[^\n]", stringBuffer) == 1)
					{
						tags = UString(stringBuffer);
						tags = tags.trim();
					}

					memset(stringBuffer, '\0', 1024);
					if (sscanf(curLineChar, " BeatmapID :%1023[^\n]", stringBuffer) == 1)
					{
						// FUCK stol(), causing crashes in e.g. std::stol("--123456");
						//beatmapId = std::stol(stringBuffer);
						beatmapId = 0;
					}
				}
				break;

			case 2: // Difficulty
				sscanf(curLineChar, " CircleSize : %f \n", &CS);
				if (sscanf(curLineChar, " ApproachRate : %f \n", &AR) == 1)
					foundAR = true;
				sscanf(curLineChar, " HPDrainRate : %f \n", &HP);
				sscanf(curLineChar, " OverallDifficulty : %f \n", &OD);
				sscanf(curLineChar, " SliderMultiplier : %f \n", &sliderMultiplier);
				sscanf(curLineChar, " SliderTickRate : %f \n", &sliderTickRate);
				break;
			case 3: // Events
				{
					memset(stringBuffer, '\0', 1024);
					int type, startTime;
					if (sscanf(curLineChar, " %i , %i , \"%1023[^\"]\"", &type, &startTime, stringBuffer) == 3)
					{
						if (type == 0)
							backgroundImageName = UString(stringBuffer);
					}
				}
				break;
			case 4: // Colours
				{
					int comboNum;
					int r,g,b;
					if (sscanf(curLineChar, " Combo %i : %i , %i , %i \n", &comboNum, &r, &g, &b) == 4)
						combocolors.push_back(COLOR(255, r, g, b));
				}
				break;
			case 5: // TimingPoints

				// old beatmaps: Offset, Milliseconds per Beat
				// new beatmaps: Offset, Milliseconds per Beat, Meter, Sample Type, Sample Set, Volume, Inherited, Kiai Mode

				double tpOffset;
				float tpMSPerBeat;
				int tpMeter;
				int tpSampleType,tpSampleSet;
				int tpVolume;
				int tpKiai;
				if (sscanf(curLineChar, " %lf , %f , %i , %i , %i , %i , %i", &tpOffset, &tpMSPerBeat, &tpMeter, &tpSampleType, &tpSampleSet, &tpVolume, &tpKiai) == 7)
				{
					TIMINGPOINT t;
					t.offset = (long)std::round(tpOffset);
					t.msPerBeat = tpMSPerBeat;
					t.sampleType = tpSampleType;
					t.sampleSet = tpSampleSet;
					t.volume = tpVolume;
					t.kiai = tpKiai > 0;
					t.sortHack = timingPointSortHack++;

					timingpoints.push_back(t);
				}
				else if (sscanf(curLineChar, " %lf , %f", &tpOffset, &tpMSPerBeat) == 2)
				{
					TIMINGPOINT t;
					t.offset = (long)std::round(tpOffset);
					t.msPerBeat = tpMSPerBeat;

					t.sampleType = 0;
					t.sampleSet = 0;
					t.volume = 100;
					t.sortHack = timingPointSortHack++;

					timingpoints.push_back(t);
				}
				break;
			case 6: // HitObjects

				int x,y;
				long time;
				int type;
				int hitSound;

				// minimalist hitobject loading, this only loads the parts necessary for the star calculation
				if (sscanf(curLineChar, " %i , %i , %li , %i , %i", &x, &y, &time, &type, &hitSound) == 5)
				{
					if (type & 0x1) // circle
					{
						HITCIRCLE c;

						c.x = x;
						c.y = y;
						c.time = time;

						hitcircles.push_back(c);
					}
					else if (type & 0x2) // slider
					{
						SLIDER s;

						s.x = x;
						s.y = y;
						s.time = time;
						s.repeat = 0; // unused for star calculation
						s.sliderTime = 0.0f; // unused for star calculation
						s.sliderTimeWithoutRepeats = 0.0f; // unused for star calculation

						sliders.push_back(s);
					}
					else if (type & 0x8) // spinner
					{
						SPINNER s;

						s.x = x;
						s.y = y;
						s.time = time;
						s.endTime = 0; // unused for star calculation

						spinners.push_back(s);
					}
				}
				break;
			}
		}
	}

	// gamemode filter
	if ((mode != 0 && m_osu->getGamemode() == Osu::GAMEMODE::STD) || (mode != 0x03 && m_osu->getGamemode() == Osu::GAMEMODE::MANIA))
		return false;

	// build sound file path
	fullSoundFilePath = m_sFolder;
	fullSoundFilePath.append(audioFileName);

	// calculate BPM range
	if (timingpoints.size() > 0)
	{
		if (Osu::debug->getBool())
			debugLog("OsuBeatmapDifficulty::loadMetadata() : calculating BPM range ...\n");

		// sort timingpoints by time
		std::sort(timingpoints.begin(), timingpoints.end(), TimingPointSortComparator());

		// calculate bpm range
		float tempMinBPM = 0;
		float tempMaxBPM = std::numeric_limits<float>::max();
		for (int i=0; i<timingpoints.size(); i++)
		{
			TIMINGPOINT *t = &timingpoints[i];

			if (t->msPerBeat >= 0) // NOT inherited
			{
				if (t->msPerBeat > tempMinBPM)
					tempMinBPM = t->msPerBeat;
				if (t->msPerBeat < tempMaxBPM)
					tempMaxBPM = t->msPerBeat;
			}
		}

		// convert from msPerBeat to BPM
		const float msPerMinute = 1 * 60 * 1000;
		if (tempMinBPM != 0)
			tempMinBPM = msPerMinute / tempMinBPM;
		if (tempMaxBPM != 0)
			tempMaxBPM = msPerMinute / tempMaxBPM;

		minBPM = (int)std::round(tempMinBPM);
		maxBPM = (int)std::round(tempMaxBPM);
	}

	// old beatmaps: AR = OD, there is no ApproachRate stored
	if (!foundAR)
		AR = OD;

	// calculate default nomod standard stars, and immediately unload everything unnecessary after that
	if (calculateStars && m_osu_database_dynamic_star_calculation->getBool())
	{
		if (starsNoMod == 0.0f)
		{
			double aimStars = 0.0;
			double speedStars = 0.0;
			starsNoMod = calculateStarDiff(NULL, &aimStars, &speedStars);
			unload();

			if (starsNoMod == 0.0f)
				starsNoMod = -0.0000001f; // to avoid reloading endlessly on beatmaps which simply have zero stars (or where the calculation fails)
		}
	}

	return true;
}

bool OsuBeatmapDifficulty::loadRaw(OsuBeatmap *beatmap, std::vector<OsuHitObject*> *hitobjects)
{
	unload();
	combocolors = std::vector<Color>();
	loadMetadataRaw();
	timingpoints = std::vector<TIMINGPOINT>(); // delete basic timingpoints which just got loaded by loadMetadataRaw(), just so we have a clean start

	// open osu file
	File file(m_sFilePath);
	if (!file.canRead())
	{
		UString errorMessage = "Error: Couldn't load beatmap file :(";
		debugLog("Osu Error: Couldn't load beatmap file (%s)!\n", m_sFilePath.toUtf8());
		if (m_osu != NULL)
			m_osu->getNotificationOverlay()->addNotification(errorMessage, 0xffff0000);
		return false;
	}

	// load beatmap skin
	beatmap->getOsu()->getSkin()->setBeatmapComboColors(combocolors);
	beatmap->getOsu()->getSkin()->loadBeatmapOverride(m_sFolder);

	// load the actual beatmap
	int colorCounter = 1;
	int comboNumber = 1;
	int curBlock = -1;
	unsigned long long timingPointSortHack = 0;
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
							b.startTime = startTime;
							b.endTime = endTime;
							breaks.push_back(b);
						}
					}
				}
				break;
			case 2: // TimingPoints

				// old beatmaps: Offset, Milliseconds per Beat
				// new beatmaps: Offset, Milliseconds per Beat, Meter, Sample Type, Sample Set, Volume, Inherited, Kiai Mode

				double tpOffset;
				float tpMSPerBeat;
				int tpMeter;
				int tpSampleType,tpSampleSet;
				int tpVolume;
				int tpKiai;
				if (sscanf(curLineChar, " %lf , %f , %i , %i , %i , %i , %i", &tpOffset, &tpMSPerBeat, &tpMeter, &tpSampleType, &tpSampleSet, &tpVolume, &tpKiai) == 7)
				{
					TIMINGPOINT t;
					t.offset = (long)std::round(tpOffset);
					t.msPerBeat = tpMSPerBeat;
					t.sampleType = tpSampleType;
					t.sampleSet = tpSampleSet;
					t.volume = tpVolume;
					t.kiai = tpKiai > 0;
					t.sortHack = timingPointSortHack++;

					timingpoints.push_back(t);
				}
				else if (sscanf(curLineChar, " %lf , %f", &tpOffset, &tpMSPerBeat) == 2)
				{
					TIMINGPOINT t;
					t.offset = (long)std::round(tpOffset);
					t.msPerBeat = tpMSPerBeat;

					t.sampleType = 0;
					t.sampleSet = 0;
					t.volume = 100;
					t.kiai = false;
					t.sortHack = timingPointSortHack++;

					timingpoints.push_back(t);
				}
				break;

			case 3: // HitObjects

				// circles:
				// x,y,time,type,hitSound,addition
				// sliders:
				// x,y,time,type,hitSound,sliderType|curveX:curveY|...,repeat,pixelLength,edgeHitsound,edgeAddition,addition
				// spinners:
				// x,y,time,type,hitSound,endTime,addition

				int x,y;
				long time;
				int type;
				int hitSound;

				if (sscanf(curLineChar, " %i , %i , %li , %i , %i", &x, &y, &time, &type, &hitSound) == 5)
				{
					if (type & 0x4) // new combo
					{
						comboNumber = 1;
						colorCounter++;
					}

					if (type & 0x1) // circle
					{
						HITCIRCLE c;

						c.x = x;
						c.y = y;
						c.time = time;
						c.sampleType = hitSound;
						c.number = comboNumber++;
						c.colorCounter = colorCounter;
						c.clicked = false;
						c.maniaEndTime = 0;

						hitcircles.push_back(c);
					}
					else if (type & 0x2) // slider
					{
						UString curLineString = UString(curLineChar);
						std::vector<UString> tokens = curLineString.split(",");
						if (tokens.size() < 8)
						{
							debugLog("Invalid slider in beatmap: %s\n\ncurLine = %s\n", m_sFilePath.toUtf8(), curLineChar);
							continue;
							//engine->showMessageError("Error", UString::format("Invalid slider in beatmap: %s\n\ncurLine = %s", m_sFilePath.toUtf8(), curLine));
							//return false;
						}

						std::vector<UString> sliderTokens = tokens[5].split("|");
						if (sliderTokens.size() < 2)
						{
							debugLog("Invalid slider tokens: %s\n\nIn beatmap: %s\n", curLineChar, m_sFilePath.toUtf8());
							continue;
							//engine->showMessageError("Error", UString::format("Invalid slider tokens: %s\n\nIn beatmap: %s", curLineChar, m_sFilePath.toUtf8()));
							//return false;
						}

						const float sanityRange = 65536/2; // infinity sanity check, same as before
						std::vector<Vector2> points;
						points.push_back(Vector2(clamp<float>(x, -sanityRange, sanityRange), clamp<float>(y, -sanityRange, sanityRange)));
						for (int i=1; i<sliderTokens.size(); i++)
						{
							std::vector<UString> sliderXY = sliderTokens[i].split(":");

							// array size check
							// infinity sanity check (this only exists because of https://osu.ppy.sh/b/1029976)
							// not a very elegant check, but it does the job
							if (sliderXY.size() != 2 || sliderXY[0].find("E") != -1 || sliderXY[0].find("e") != -1 || sliderXY[1].find("E") != -1 || sliderXY[1].find("e") != -1)
							{
								debugLog("Invalid slider positions: %s\n\nIn Beatmap: %s\n", curLineChar, m_sFilePath.toUtf8());
								continue;
								//engine->showMessageError("Error", UString::format("Invalid slider positions: %s\n\nIn beatmap: %s", curLine, m_sFilePath.toUtf8()));
								//return false;
							}
							points.push_back(Vector2((int)clamp<float>(sliderXY[0].toFloat(), -sanityRange, sanityRange), (int)clamp<float>(sliderXY[1].toFloat(), -sanityRange, sanityRange)));
						}

						SLIDER s;
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
						s.points = points;

						if (tokens.size() > 8)
						{
							std::vector<UString> hitSoundTokens = tokens[8].split("|");
							for (int i=0; i<hitSoundTokens.size(); i++)
							{
								s.hitSounds.push_back(hitSoundTokens[i].toInt());
							}
						}

						sliders.push_back(s);
					}
					else if (type & 0x8) // spinner
					{
						UString curLineString = UString(curLineChar);
						std::vector<UString> tokens = curLineString.split(",");
						if (tokens.size() < 6)
						{
							debugLog("Invalid spinner in beatmap: %s\n\ncurLine = %s\n", m_sFilePath.toUtf8(), curLineChar);
							continue;
							//engine->showMessageError("Error", UString::format("Invalid spinner in beatmap: %s\n\ncurLine = %s", m_sFilePath.toUtf8(), curLine));
							//return false;
						}

						SPINNER s;
						s.x = x;
						s.y = y;
						s.time = time;
						s.sampleType = hitSound;
						s.endTime = tokens[5].toFloat();

						spinners.push_back(s);
					}
					else if (m_osu->getGamemode() == Osu::GAMEMODE::MANIA && (type & 0x80)) // osu!mania hold note, gamemode check for sanity
					{
						UString curLineString = UString(curLineChar);
						std::vector<UString> tokens = curLineString.split(",");

						if (tokens.size() < 6)
						{
							debugLog("Invalid hold note in beatmap: %s\n\ncurLine = %s\n", m_sFilePath.toUtf8(), curLineChar);
							continue;
						}

						std::vector<UString> holdNoteTokens = tokens[5].split(":");
						if (holdNoteTokens.size() < 1)
						{
							debugLog("Invalid hold note in beatmap: %s\n\ncurLine = %s\n", m_sFilePath.toUtf8(), curLineChar);
							continue;
						}

						HITCIRCLE c;

						c.x = x;
						c.y = y;
						c.time = time;
						c.sampleType = hitSound;
						c.number = comboNumber++;
						c.colorCounter = colorCounter;
						c.clicked = false;
						c.maniaEndTime = holdNoteTokens[0].toLong();

						hitcircles.push_back(c);
					}
				}
				break;
			}
		}
	}

	// check if we have any timingpoints at all
	if (timingpoints.size() == 0)
	{
		UString errorMessage = "Error: No timingpoints in beatmap";
		debugLog("Osu Error: No timingpoints in beatmap (%s)!\n", m_sFilePath.toUtf8());
		if (m_osu != NULL)
			m_osu->getNotificationOverlay()->addNotification(errorMessage, 0xffff0000);
		return false;
	}

	// sort timingpoints by time
	std::sort(timingpoints.begin(), timingpoints.end(), TimingPointSortComparator());

	// calculate sliderTimes, and build clicks and ticks
	for (int i=0; i<sliders.size(); i++)
	{
		SLIDER *s = &sliders[i];
		s->sliderTimeWithoutRepeats = getSliderTimeForSlider(s);
		s->sliderTime = s->sliderTimeWithoutRepeats * s->repeat;

		// start & end clicks
		s->clicked.push_back( (struct SLIDER_CLICK) {s->time, false} );
		s->clicked.push_back( (struct SLIDER_CLICK) {s->time + (long)s->sliderTime, false} );

		// repeat clicks
		if (s->repeat > 1)
		{
			long part = (long) (s->sliderTime / (float) s->repeat);
			for (int c=1; c<s->repeat; c++)
			{
				s->clicked.push_back( (struct SLIDER_CLICK) {s->time + (long)(part*c), false} );
			}
		}

		// ticks
		float minTickPixelDistanceFromEnd = 0.01f * getSliderVelocity(s);
		float tickPixelLength = getSliderTickDistance() / getTimingPointMultiplierForSlider(s);
		float tickDurationPercentOfSliderLength = tickPixelLength / (s->pixelLength == 0.0f ? 1.0f : s->pixelLength);
		int tickCount = (int)std::ceil(s->pixelLength / tickPixelLength) - 1;
		if (tickCount > 0)
		{
			float tickTOffset = tickDurationPercentOfSliderLength;
			float t = tickTOffset;
			float pixelDistanceToEnd = s->pixelLength;
			for (int i=0; i<tickCount; i++, t+=tickTOffset)
			{
				// skip ticks which are too close to the end of the slider
				pixelDistanceToEnd -= tickPixelLength;
				if (pixelDistanceToEnd <= minTickPixelDistanceFromEnd)
					break;

				s->ticks.push_back(t);
			}
		}
	}

	// build hitobjects from the data we loaded from the beatmap file
	OsuBeatmapStandard *beatmapStandard = dynamic_cast<OsuBeatmapStandard*>(beatmap);
	OsuBeatmapMania *beatmapMania = dynamic_cast<OsuBeatmapMania*>(beatmap);
	if (beatmapStandard != NULL)
		buildStandardHitObjects(beatmapStandard, hitobjects);
	if (beatmapMania != NULL)
		buildManiaHitObjects(beatmapMania, hitobjects);

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
	std::sort(hitobjects->begin(), hitobjects->end(), HitObjectSortComparator());

	// precalculate Score v2 combo portion maximum
	if (beatmapStandard != NULL)
	{
		if (hitobjects->size() > 0)
			m_fScoreV2ComboPortionMaximum = 0;

		int combo = 0;
		for (int i=0; i<hitobjects->size(); i++)
		{
			int scoreComboMultiplier = std::max(combo-1, 0);

			OsuCircle *circlePointer = dynamic_cast<OsuCircle*>((*(hitobjects))[i]);
			OsuSlider *sliderPointer = dynamic_cast<OsuSlider*>((*(hitobjects))[i]);
			OsuSpinner *spinnerPointer = dynamic_cast<OsuSpinner*>((*(hitobjects))[i]);

			if (circlePointer != NULL || spinnerPointer != NULL)
			{
				m_fScoreV2ComboPortionMaximum += (unsigned long long)(300.0 * (1.0 + (double)scoreComboMultiplier / 10.0));
				combo++;
			}
			else if (sliderPointer != NULL)
			{
				combo += 1 + sliderPointer->getClicks().size();
				scoreComboMultiplier = std::max(combo-1, 0);
				m_fScoreV2ComboPortionMaximum += (unsigned long long)(300.0 * (1.0 + (double)scoreComboMultiplier / 10.0));
				combo++;
			}
		}
	}

	// special rule for first hitobject (for 1 approach circle with HD)
	if (osu_show_approach_circle_on_first_hidden_object.getBool())
	{
		if (hitobjects->size() > 0)
			(*hitobjects)[0]->setForceDrawApproachCircle(true);
	}

	debugLog("OsuBeatmapDifficulty::loadRaw() loaded %i hitobjects.\n", hitobjects->size());

	loaded = true;
	return true;
}

void OsuBeatmapDifficulty::deleteBackgroundImagePathLoader()
{
	SAFE_DELETE(m_backgroundImagePathLoader);
}

void OsuBeatmapDifficulty::loadBackgroundImage()
{
	if (!osu_load_beatmap_background_images.getBool()) return;

	m_bShouldBackgroundImageBeLoaded = true;

	if (m_backgroundImagePathLoader != NULL) // handle loader cleanup
	{
		if (m_backgroundImagePathLoader->isReady())
			deleteBackgroundImagePathLoader();
	}
	else if (backgroundImageName.length() < 1 || starsNoMod == 0.0) // dynamically load background image path and star rating from osu file if it wasn't set by the database
	{
		m_backgroundImagePathLoader = new BackgroundImagePathLoader(this);
		engine->getResourceManager()->requestNextLoadAsync();
		engine->getResourceManager()->loadResource(m_backgroundImagePathLoader);
		return;
	}

	if (backgroundImage != NULL || backgroundImageName.length() < 1) return;

	UString fullBackgroundImageFilePath = m_sFolder;
	fullBackgroundImageFilePath.append(backgroundImageName);
	if (env->fileExists(fullBackgroundImageFilePath)) // to prevent resource manager warnings
	{
		UString uniqueResourceName = fullBackgroundImageFilePath;
		uniqueResourceName.append(name);
		uniqueResourceName.append(UString::format("%i%i%i", ID, setID, rand())); // added rand for sanity
		engine->getResourceManager()->requestNextLoadAsync();
		backgroundImage = engine->getResourceManager()->loadImageAbs(fullBackgroundImageFilePath, uniqueResourceName);
	}
}

void OsuBeatmapDifficulty::unloadBackgroundImage()
{
	m_bShouldBackgroundImageBeLoaded = false;

	if (Osu::debug->getBool() && backgroundImage != NULL)
		debugLog("Unloading %s\n", backgroundImage->getFilePath().toUtf8());

	Image *tempPointer = backgroundImage;
	backgroundImage = NULL;
	engine->getResourceManager()->destroyResource(tempPointer);
}

void OsuBeatmapDifficulty::loadBackgroundImagePath()
{
	if (backgroundImage != NULL || backgroundImageName.length() > 0) return;

	// open osu file
	File file(m_sFilePath);
	if (!file.canRead())
	{
		if (Osu::debug->getBool())
			debugLog("OsuBeatmapDifficulty::loadBackgroundImagePath() couldn't read %s\n", m_sFilePath.toUtf8());
		return;
	}

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
				curBlock = 0;
			else if (curLine.find("[Colours]") != std::string::npos)
				break; // stop early

			bool found = false;
			switch (curBlock)
			{
			case 0: // Events
				{
					memset(stringBuffer, '\0', 1024);
					float temp;
					if (sscanf(curLineChar, " %f , %f , \"%1023[^\"]\"", &temp, &temp, stringBuffer) == 3)
					{
						backgroundImageName = UString(stringBuffer);
						found = true;
					}
				}
				break;
			}
			if (found)
				break;
		}
	}
}

bool OsuBeatmapDifficulty::isBackgroundLoaderActive()
{
	 return m_backgroundImagePathLoader != NULL && !m_backgroundImagePathLoader->isReady();
}

void OsuBeatmapDifficulty::buildStandardHitObjects(OsuBeatmapStandard *beatmap, std::vector<OsuHitObject*> *hitobjects)
{
	if (beatmap == NULL || hitobjects == NULL) return;

	// also calculate max combo
	m_iMaxCombo = 0;

	for (int i=0; i<hitcircles.size(); i++)
	{
		OsuBeatmapDifficulty::HITCIRCLE *c = &hitcircles[i];

		if (osu_mod_random.getBool())
		{
			c->x = clamp<int>(c->x - (rand() % OsuGameRules::OSU_COORD_WIDTH) / 8, 0, OsuGameRules::OSU_COORD_WIDTH);
			c->y = clamp<int>(c->y - (rand() & OsuGameRules::OSU_COORD_HEIGHT) / 8, 0, OsuGameRules::OSU_COORD_HEIGHT);
		}

		hitobjects->push_back(new OsuCircle(c->x, c->y, c->time, c->sampleType, c->number, c->colorCounter, beatmap));
	}
	m_iMaxCombo += hitcircles.size();

	for (int i=0; i<sliders.size(); i++)
	{
		OsuBeatmapDifficulty::SLIDER *s = &sliders[i];

		int repeats = std::max((s->repeat - 1), 0);
		m_iMaxCombo += 2 + repeats + (repeats+1)*s->ticks.size(); // start/end + repeat arrow + ticks

		if (osu_mod_random.getBool())
		{
			for (int p=0; p<s->points.size(); p++)
			{
				s->points[p].x = clamp<int>(s->points[p].x - (rand() % OsuGameRules::OSU_COORD_WIDTH) / 3, 0, OsuGameRules::OSU_COORD_WIDTH);
				s->points[p].y = clamp<int>(s->points[p].y - (rand() % OsuGameRules::OSU_COORD_HEIGHT) / 3, 0, OsuGameRules::OSU_COORD_HEIGHT);
			}
		}

		hitobjects->push_back(new OsuSlider(s->type, s->repeat, s->pixelLength, s->points, s->hitSounds, s->ticks, s->sliderTime, s->sliderTimeWithoutRepeats, s->time, s->sampleType, s->number, s->colorCounter, beatmap));
	}

	for (int i=0; i<spinners.size(); i++)
	{
		OsuBeatmapDifficulty::SPINNER *s = &spinners[i];

		if (osu_mod_random.getBool())
		{
			s->x = clamp<int>(s->x - ((rand() % OsuGameRules::OSU_COORD_WIDTH) / 1.25f) * (rand() % 2 == 0 ? 1.0f : -1.0f), 0, OsuGameRules::OSU_COORD_WIDTH);
			s->y = clamp<int>(s->y - ((rand() & OsuGameRules::OSU_COORD_HEIGHT) / 1.25f) * (rand() % 2 == 0 ? 1.0f : -1.0f), 0, OsuGameRules::OSU_COORD_HEIGHT);
		}

		hitobjects->push_back(new OsuSpinner(s->x, s->y, s->time, s->sampleType, s->endTime, beatmap));
	}
	m_iMaxCombo += spinners.size();

	// debugging
	if (m_osu_debug_pp->getBool())
	{
		double aim = 0.0;
		double speed = 0.0;
		double stars = calculateStarDiff(beatmap, &aim, &speed);
		double pp = calculatePPv2(beatmap->getOsu(), beatmap, aim, speed, hitobjects->size(), hitcircles.size(), m_iMaxCombo);

		engine->showMessageInfo("PP", UString::format("pp = %f, stars = %f, aimstars = %f, speedstars = %f, %i circles, %i sliders, %i spinners, %i hitobjects, maxcombo = %i", pp, stars, aim, speed, hitcircles.size(), sliders.size(), spinners.size(), (hitcircles.size() + sliders.size() + spinners.size()), m_iMaxCombo));
	}
}

void OsuBeatmapDifficulty::buildManiaHitObjects(OsuBeatmapMania *beatmap, std::vector<OsuHitObject*> *hitobjects)
{
	if (beatmap == NULL || hitobjects == NULL) return;

	int availableColumns = beatmap->getNumColumns();

	for (int i=0; i<hitcircles.size(); i++)
	{
		OsuBeatmapDifficulty::HITCIRCLE *c = &hitcircles[i];
		hitobjects->push_back(new OsuManiaNote(OsuManiaHelper::getColumn(availableColumns, c->x), c->maniaEndTime > 0 ? (c->maniaEndTime - c->time) : 0, c->time, c->sampleType, c->number, c->colorCounter, beatmap));
	}

	for (int i=0; i<sliders.size(); i++)
	{
		OsuBeatmapDifficulty::SLIDER *s = &sliders[i];
		hitobjects->push_back(new OsuManiaNote(OsuManiaHelper::getColumn(availableColumns, s->x), s->sliderTime, s->time, s->sampleType, s->number, s->colorCounter, beatmap));
	}
}

float OsuBeatmapDifficulty::getSliderTickDistance()
{
	return ((100.0f * sliderMultiplier) / sliderTickRate);
}

float OsuBeatmapDifficulty::getSliderTimeForSlider(SLIDER *slider)
{
	const float duration = getTimingInfoForTime(slider->time).beatLength * (slider->pixelLength / sliderMultiplier) / 100.0f;
	return duration >= 1.0f ? duration : 1.0f; // sanity check
}

float OsuBeatmapDifficulty::getSliderVelocity(SLIDER *slider)
{
	const float beatLength = getTimingInfoForTime(slider->time).beatLength;
	if (beatLength > 0.0f)
		return (getSliderTickDistance() * sliderTickRate * (1000.0f / beatLength));
	else
		return getSliderTickDistance() * sliderTickRate;
}

float OsuBeatmapDifficulty::getTimingPointMultiplierForSlider(SLIDER *slider)
{
	const TIMING_INFO t = getTimingInfoForTime(slider->time);
	float beatLengthBase = t.beatLengthBase;
	if (beatLengthBase == 0.0f) // sanity check
		beatLengthBase = 1.0f;

	return t.beatLength / beatLengthBase;
}

OsuBeatmapDifficulty::TIMING_INFO OsuBeatmapDifficulty::getTimingInfoForTime(unsigned long positionMS)
{
	TIMING_INFO ti;
	ti.offset = 0;
	ti.beatLengthBase = 1;
	ti.beatLength = 1;
	ti.volume = 100;
	ti.sampleType = 0;
	ti.sampleSet = 0;

	if (timingpoints.size() <= 0)
		return ti;

	// initial values
	ti.offset = timingpoints[0].offset;
	ti.volume = timingpoints[0].volume;
	ti.sampleSet = timingpoints[0].sampleSet;
	ti.sampleType = timingpoints[0].sampleType;

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
			ti.beatLength = ti.beatLengthBase * (clamp<float>(std::abs(t->msPerBeat), 10, 1000) / 100.0f); // sliderMultiplier of a timingpoint = (t->velocity / -100.0f)
		}

		ti.volume = t->volume;
		ti.sampleType = t->sampleType;
		ti.sampleSet = t->sampleSet;
	}

	return ti;
}

unsigned long OsuBeatmapDifficulty::getBreakDuration(unsigned long positionMS)
{
	for (int i=0; i<breaks.size(); i++)
	{
		if ((int)positionMS > breaks[i].startTime && (int)positionMS < breaks[i].endTime)
			return (unsigned long)(breaks[i].endTime - breaks[i].startTime);
	}
	return 0;
}

bool OsuBeatmapDifficulty::isInBreak(unsigned long positionMS)
{
	for (int i=0; i<breaks.size(); i++)
	{
		if ((int)positionMS > breaks[i].startTime && (int)positionMS < breaks[i].endTime)
			return true;
	}
	return false;
}



//***********************************************************************************//
//	(c) and thanks to Tom94 & Francesco149 @ https://github.com/Francesco149/oppai/  //
//***********************************************************************************//

void OsuBeatmapDifficulty::rebuildStarCacheForUpToHitObjectIndex(OsuBeatmap *beatmap, std::atomic<bool> &kys, std::atomic<int> &progress)
{
	// precalculate cut star values for live pp

	// reset
	m_aimStarsForNumHitObjects.clear();
	m_speedStarsForNumHitObjects.clear();

	std::vector<PPHitObject> hitObjects = generatePPHitObjectsForBeatmap(beatmap);
	const float CS = beatmap->getCS();

	for (int i=0; i<hitObjects.size(); i++)
	{
		double aimStars = 0.0;
		double speedStars = 0.0;

		calculateStarDiffForHitObjects(hitObjects, CS, &aimStars, &speedStars, i);

		m_aimStarsForNumHitObjects.push_back(aimStars);
		m_speedStarsForNumHitObjects.push_back(speedStars);

		progress = i;

		if (kys.load())
		{
			//printf("OsuBeatmapDifficulty::rebuildStarCacheForUpToHitObjectIndex() killed\n");
			m_aimStarsForNumHitObjects.clear();
			m_speedStarsForNumHitObjects.clear();
			return; // stop everything
		}
	}
}

std::vector<OsuBeatmapDifficulty::PPHitObject> OsuBeatmapDifficulty::generatePPHitObjectsForBeatmap(OsuBeatmap *beatmap)
{
	// build generalized PPHitObject structs from the vectors (hitcircles, sliders, spinners)
	// the PPHitObject struct is the one getting used in all pp/star calculations, it encompasses every object type for simplicity

	std::vector<OsuBeatmapDifficulty::PPHitObject> ppHitObjects;

	unsigned long long sortHackCounter = 0;

	for (int i=0; i<hitcircles.size(); i++)
	{
		PPHitObject ho;
		ho.pos = Vector2(hitcircles[i].x, hitcircles[i].y);
		ho.time = (long)hitcircles[i].time;
		ho.type = PP_HITOBJECT_TYPE::circle;
		ho.end_time = ho.time;
		ho.sortHack = sortHackCounter++;
		ppHitObjects.push_back(ho);
	}

	for (int i=0; i<sliders.size(); i++)
	{
		PPHitObject ho;
		ho.pos = Vector2(sliders[i].x, sliders[i].y);
		ho.time = sliders[i].time;
		ho.type = PP_HITOBJECT_TYPE::slider;
		ho.end_time = sliders[i].time + sliders[i].sliderTime; // start + duration
		ho.sortHack = sortHackCounter++;
		ppHitObjects.push_back(ho);
	}

	for (int i=0; i<spinners.size(); i++)
	{
		PPHitObject ho;
		ho.pos = Vector2(spinners[i].x, spinners[i].y);
		ho.time = spinners[i].time;
		ho.type = PP_HITOBJECT_TYPE::spinner;
		ho.end_time = spinners[i].endTime;
		ho.sortHack = sortHackCounter++;
		ppHitObjects.push_back(ho);
	}

	// sort hitobjects by time
	struct HitObjectSortComparator
	{
	    bool operator() (PPHitObject const a, PPHitObject const b) const
	    {
	    	// strict weak ordering!
	    	if (a.time == b.time)
	    		return a.sortHack < b.sortHack;
	    	else
	    		return a.time < b.time;
	    }
	};
	std::sort(ppHitObjects.begin(), ppHitObjects.end(), HitObjectSortComparator());

	// apply speed multiplier
	if (beatmap != NULL)
	{
		if (beatmap->getOsu()->getSpeedMultiplier() != 1.0f && beatmap->getOsu()->getSpeedMultiplier() > 0.0f)
		{
			const double speedMultiplier = 1.0 / (double)beatmap->getOsu()->getSpeedMultiplier();
			for (int i=0; i<ppHitObjects.size(); i++)
			{
				ppHitObjects[i].time = (long)((double)ppHitObjects[i].time * speedMultiplier);
				ppHitObjects[i].end_time = (long)((double)ppHitObjects[i].end_time * speedMultiplier);
			}
		}
	}

	return ppHitObjects;
}

double OsuBeatmapDifficulty::calculateStarDiffForHitObjects(std::vector<PPHitObject> &hitObjects, float CS, double *aim, double *speed, int upToObjectIndex)
{
	// depends on speed multiplier + CS

	// NOTE: upToObjectIndex is applied way below, during the construction of the 'dobjects'

	if (hitObjects.size() < 1)
		return 0.0;

	// ****************************************************************************************************************************************** //

	// based on tom94's osu!tp aimod

	// how much strains decay per interval (if the previous interval's peak
	// strains after applying decay are still higher than the current one's,
	// they will be used as the peak strains).
	static const double decay_base[] = {0.3, 0.15};

	// almost the normalized circle diameter (104px)
	static const double almost_diameter = 90;

	// arbitrary tresholds to determine when a stream is spaced enough that is
	// becomes hard to alternate.
	static const double stream_spacing = 110;
	static const double single_spacing = 125;

	// used to keep speed and aim balanced between eachother
	static const double weight_scaling[] = {1400, 26.25};

	// non-normalized diameter where the circlesize buff starts
	static const float circlesize_buff_treshold = 30;

	class cdiff
	{
	public:
		enum diff
		{
			speed = 0,
			aim = 1
		};

		static unsigned int diffToIndex(diff d)
		{
			switch (d)
			{
			case speed:
				return 0;
			case aim:
				return 1;
			}

			return 0;
		}
	};

	// diffcalc hit object
	class DiffObject
	{
	public:
		PPHitObject *ho;

		double strains[2];

		// start/end positions normalized on radius
		Vector2 norm_start;
		Vector2 norm_end;

		DiffObject(PPHitObject *base_object, float radius)
		{
			ho = base_object;

			// strains start at 1
			strains[0] = 1.0;
			strains[1] = 1.0;

			// positions are normalized on circle radius so that we can calc as
			// if everything was the same circlesize
			float scaling_factor = 52.0f / radius;

			// cs buff (credits to osuElements, I have confirmed that this is indeed accurate)
			if (radius < circlesize_buff_treshold)
				scaling_factor *= 1.0f + std::min((circlesize_buff_treshold - radius), 5.0f) / 50.f;

			norm_start = ho->pos * scaling_factor;

			norm_end = norm_start;
			// ignoring slider lengths doesn't seem to affect star rating too much and speeds up the calculation exponentially
		}

		void calculate_strains(DiffObject *prev)
		{
			calculate_strain(prev, cdiff::diff::speed);
			calculate_strain(prev, cdiff::diff::aim);
		}

		void calculate_strain(DiffObject *prev, cdiff::diff dtype)
		{
			double res = 0;
			long time_elapsed = ho->time - prev->ho->time;
			double decay = std::pow(decay_base[cdiff::diffToIndex(dtype)], (double)time_elapsed / 1000.0);
			double scaling = weight_scaling[cdiff::diffToIndex(dtype)];

			switch (ho->type) {
				case PP_HITOBJECT_TYPE::slider: // we don't use sliders in this implementation
				case PP_HITOBJECT_TYPE::circle:
					res = spacing_weight(distance(prev), dtype) * scaling;
					break;

				case PP_HITOBJECT_TYPE::spinner:
					break;

				case PP_HITOBJECT_TYPE::invalid:
					// NOTE: silently ignore
					return;
			}

			res /= (double)std::max(time_elapsed, (long)50);
			strains[cdiff::diffToIndex(dtype)] = prev->strains[cdiff::diffToIndex(dtype)] * decay + res;
		}

		double spacing_weight(double distance, cdiff::diff diff_type)
		{
			switch (diff_type)
			{
				case cdiff::diff::speed:
					if (distance > single_spacing)
					{
						return 2.5;
					}
					else if (distance > stream_spacing)
					{
						return 1.6 + 0.9 *
							(distance - stream_spacing) /
							(single_spacing - stream_spacing);
					}
					else if (distance > almost_diameter)
					{
						return 1.2 + 0.4 * (distance - almost_diameter)
							/ (stream_spacing - almost_diameter);
					}
					else if (distance > almost_diameter / 2.0)
					{
						return 0.95 + 0.25 *
							(distance - almost_diameter / 2.0) /
							(almost_diameter / 2.0);
					}
					return 0.95;

				case cdiff::diff::aim:
					return std::pow(distance, 0.99);

				default:
					return 0.0;
			}
		}

		double distance(DiffObject *prev)
		{
			return (norm_start - prev->norm_end).length();
		}
	};

	// ****************************************************************************************************************************************** //

	static const double star_scaling_factor = 0.0675;
	static const double extreme_scaling_factor = 0.5;

	// strains are calculated by analyzing the map in chunks and then taking the peak strains in each chunk.
	// this is the length of a strain interval in milliseconds.
	static const long strain_step = 400;

	// max strains are weighted from highest to lowest, and this is how much the weight decays.
	static const double decay_weight = 0.9;

	class DiffCalc
	{
	public:
		static double calculate_difficulty(cdiff::diff type, std::vector<DiffObject> &dobjects)
		{
			std::vector<double> highestStrains;
			long interval_end = strain_step;
			double max_strain = 0.0;

			DiffObject *prev = nullptr;
			for (size_t i=0; i<dobjects.size(); i++)
			{
				DiffObject *o = &dobjects[i];

				// make previous peak strain decay until the current object
				while (o->ho->time > interval_end)
				{
					highestStrains.push_back(max_strain);

					if (!prev)
						max_strain = 0.0;
					else
					{
						double decay = std::pow(decay_base[cdiff::diffToIndex(type)], (interval_end - prev->ho->time) / 1000.0);
						max_strain = prev->strains[cdiff::diffToIndex(type)] * decay;
					}

					interval_end += strain_step;
				}

				// calculate max strain for this interval
				max_strain = std::max(max_strain, o->strains[cdiff::diffToIndex(type)]);
				prev = o;
			}

			double difficulty = 0.0;
			double weight = 1.0;

			// sort strains from greatest to lowest
			std::sort(highestStrains.begin(), highestStrains.end(), std::greater<double>());

			// weigh the top strains
			for (size_t i=0; i<highestStrains.size(); i++)
			{
				difficulty += weight * highestStrains[i];
				weight *= decay_weight;
			}

			return difficulty;
		}
	};

	// ****************************************************************************************************************************************** //

	float circleRadiusInOsuPixels = OsuGameRules::getRawHitCircleDiameter(CS) / 2.0f;

	// initialize dobjects
	std::vector<DiffObject> diffObjects;
	diffObjects.reserve(hitObjects.size());
	for (size_t i=0; i<hitObjects.size() && (upToObjectIndex < 0 || i < upToObjectIndex+1); i++) // respect upToObjectIndex
	{
		DiffObject dobj(&(hitObjects[i]), circleRadiusInOsuPixels);
		diffObjects.push_back(dobj);
	}

	// calculate strains
	DiffObject *prev = &diffObjects[0];
	for (size_t i=1; i<diffObjects.size(); i++)
	{
		DiffObject *o = &diffObjects[i];
		o->calculate_strains(prev);
		prev = o;
	}

	// calculate diff
	*aim = DiffCalc::calculate_difficulty(cdiff::diff::aim, diffObjects);
	*speed = DiffCalc::calculate_difficulty(cdiff::diff::speed, diffObjects);
	*aim = std::sqrt(*aim) * star_scaling_factor;
	*speed = std::sqrt(*speed) * star_scaling_factor;

	// calculate stars
	const double stars = *aim + *speed + std::abs(*speed - *aim) * extreme_scaling_factor;
	return stars;
}

double OsuBeatmapDifficulty::calculateStarDiff(OsuBeatmap *beatmap, double *aim, double *speed, int upToObjectIndex)
{
	std::vector<PPHitObject> hitObjects = generatePPHitObjectsForBeatmap(beatmap);
	return calculateStarDiffForHitObjects(hitObjects, (beatmap != NULL ? beatmap->getCS() : CS), aim, speed, upToObjectIndex);
}

double OsuBeatmapDifficulty::calculatePPv2(Osu *osu, OsuBeatmap *beatmap, double aim, double speed, int numHitObjects, int numCircles, int maxPossibleCombo, int combo, int misses, int c300, int c100, int c50/*, SCORE_VERSION scoreVersion*/)
{
	// depends on active mods + OD + AR

	SCORE_VERSION scoreVersion = (m_osu_slider_scorev2->getBool() || osu->getModScorev2()) ? SCORE_VERSION::SCORE_V2 : SCORE_VERSION::SCORE_V1;

	// not sure what's going on here, but osu is using some strange incorrect rounding (e.g. 13.5 ms for OD 11.08333 (for OD 10 with DT), which is incorrect because the 13.5 should get rounded down to 13 ms)
	/*
	// these would be the raw unrounded values (would need an extra function in OsuGameRules to calculate this with rounding for the pp calculation)
	double od = OsuGameRules::getOverallDifficultyForSpeedMultiplier(beatmap);
	double ar = OsuGameRules::getApproachRateForSpeedMultiplier(beatmap);
	*/
	// so to get the correct pp values that players expect, with certain mods we use the incorrect method of calculating ar/od so that the final pp value will be """correct"""
	// thankfully this was already included in the oppai code. note that these incorrect values are only used while map-changing mods are active!
	double od = beatmap->getOD();
	double ar = beatmap->getAR();
	if (osu->getSpeedMultiplier() != 1.0f || osu->getModEZ() || osu->getModHR() || osu->getModDT() || osu->getModNC() || osu->getModHT() || osu->getModDC()) // if map-changing mods are active, use incorrect calculations
	{
		const float	od0_ms = OsuGameRules::getMinHitWindow300() - 0.5f,
					od10_ms = OsuGameRules::getMaxHitWindow300() - 0.5f,
					ar0_ms = OsuGameRules::getMinApproachTime(),
					ar5_ms = OsuGameRules::getMidApproachTime(),
					ar10_ms = OsuGameRules::getMaxApproachTime();

		const float	od_ms_step = (od0_ms-od10_ms)/10.0f,
					ar_ms_step1 = (ar0_ms-ar5_ms)/5.0f,
					ar_ms_step2 = (ar5_ms-ar10_ms)/5.0f;

		// stats must be capped to 0-10 before HT/DT which bring them to a range of -4.42 to 11.08 for OD and -5 to 11 for AR
		float odms = od0_ms - std::ceil(od_ms_step * beatmap->getOD());
		float arms = beatmap->getAR() <= 5 ? (ar0_ms - ar_ms_step1 *  beatmap->getAR()) : (ar5_ms - ar_ms_step2 * (beatmap->getAR() - 5));
		odms = std::min(od0_ms, std::max(od10_ms, odms));
		arms = std::min(ar0_ms, std::max(ar10_ms, arms));

		// apply speed-changing mods
		odms /= osu->getSpeedMultiplier();
		arms /= osu->getSpeedMultiplier();

		// convert OD and AR back into their stat form
		od = (od0_ms - odms) / od_ms_step;
		ar = arms > ar5_ms ? ((-(arms - ar0_ms)) / ar_ms_step1) : (5.0f + (-(arms - ar5_ms)) / ar_ms_step2);
	}

	if (c300 < 0)
		c300 = numHitObjects - c100 - c50 - misses;

	if (combo < 0)
		combo = maxPossibleCombo;

	if (combo < 1)
		return 0.0f;

	int total_hits = c300 + c100 + c50 + misses;

	// accuracy (between 0 and 1)
	double acc = calculateAcc(c300, c100, c50, misses);

	// aim pp ------------------------------------------------------------------
	double aim_value = calculateBaseStrain(aim);

	// length bonus (reused in speed pp)
	double total_hits_over_2k = (double)total_hits / 2000.0;
	double length_bonus = 0.95
		+ 0.4 * std::min(1.0, total_hits_over_2k)
		+ (total_hits > 2000 ? std::log10(total_hits_over_2k) * 0.5 : 0.0);

	// miss penalty (reused in speed pp)
	double miss_penality = std::pow(0.97, misses);

	// combo break penalty (reused in speed pp)
	double combo_break = std::pow((double)combo, 0.8) / std::pow((double)maxPossibleCombo, 0.8);

	aim_value *= length_bonus;
	aim_value *= miss_penality;
	aim_value *= combo_break;

	double ar_bonus = 1.0;

	// high AR bonus
	if (ar > 10.33)
		ar_bonus += 0.45 * (ar - 10.33);
	else if (ar < 8.0) // low ar bonus
	{
		double low_ar_bonus = 0.01 * (8.0 - ar);

		if (osu->getModHD())
			low_ar_bonus *= 2.0;

		ar_bonus += low_ar_bonus;
	}

	aim_value *= ar_bonus;

	// hidden
	if (osu->getModHD())
		aim_value *= 1.03; // https://github.com/ppy/osu-performance/pull/42

	// flashlight
	// TODO: not yet implemented
	/*
	if (osu->getModFL())
		aim_value *= 1.45 * length_bonus;
	*/

	// acc bonus (bad aim can lead to bad acc, reused in speed for same reason)
	double acc_bonus = 0.5 + acc / 2.0;

	// od bonus (low od is easy to acc even with shit aim, reused in speed ...)
	double od_bonus = 0.98 + std::pow(od, 2) / 2500.0;

	aim_value *= acc_bonus;
	aim_value *= od_bonus;

	// speed pp ----------------------------------------------------------------
	double speed_value = calculateBaseStrain(speed);

	speed_value *= length_bonus;
	speed_value *= miss_penality;
	speed_value *= combo_break;

	// hidden
	if (osu->getModHD())
		speed_value *= 1.18; // https://github.com/ppy/osu-performance/pull/42

	speed_value *= acc_bonus;
	speed_value *= od_bonus;

	// acc pp ------------------------------------------------------------------
	double real_acc = 0.0; // accuracy calculation changes from scorev1 to scorev2

	if (scoreVersion == SCORE_VERSION::SCORE_V2)
	{
		numCircles = total_hits;
		real_acc = acc;
	}
	else
	{
		// scorev1 ignores sliders since they are free 300s
		if (numCircles)
		{
			real_acc = (
					(c300 - (total_hits - numCircles)) * 300.0
					+ c100 * 100.0
					+ c50 * 50.0
					)
					/ (numCircles * 300);
		}

		// can go negative if we miss everything
		real_acc = std::max(0.0, real_acc);
	}

	// arbitrary values tom crafted out of trial and error
	double acc_value = std::pow(1.52163, od) * std::pow(real_acc, 24.0) * 2.83;

	// length bonus (not the same as speed/aim length bonus)
	acc_value *= std::min(1.15, std::pow(numCircles / 1000.0, 0.3));

	// hidden bonus
	if (osu->getModHD())
		acc_value *= 1.02;

	// flashlight bonus
	// TODO: not yet implemented
	/*
	if (osu->getModFL())
		acc_value *= 1.02;
	*/

	// total pp ----------------------------------------------------------------
	double final_multiplier = 1.12;

	// nofail
	if (osu->getModNF())
		final_multiplier *= 0.90;

	// spunout
	if (osu->getModSpunout())
		final_multiplier *= 0.95;

	return	std::pow(
			std::pow(aim_value, 1.1) +
			std::pow(speed_value, 1.1) +
			std::pow(acc_value, 1.1),
			1.0 / 1.1
		) * final_multiplier;
}

double OsuBeatmapDifficulty::calculatePPv2Acc(Osu *osu, OsuBeatmap *beatmap, double aim, double speed, double acc, int numHitObjects, int numCircles, int maxPossibleCombo, int combo, int misses/*, SCORE_VERSION scoreVersion = SCORE_VERSION::SCORE_V1*/)
{
	acc *= 100.0;

	// cap misses to num objects
	misses = std::min(numHitObjects, misses);

	// cap acc to max acc with the given amount of misses
	int max300 = (numHitObjects - misses);

	acc = std::max(0.0, std::min(calculateAcc(max300, 0, 0, misses) * 100.0, acc));

	// round acc to the closest amount of 100s or 50s
	int c50 = 0;
	int c100 = std::round(-3.0 * ((acc * 0.01 - 1.0) * numHitObjects + misses) * 0.5);

	if (c100 > numHitObjects - misses)
	{
		// acc lower than all 100s, use 50s
		c100 = 0;
		c50 = std::round(-6.0 * ((acc * 0.01 - 1.0) * numHitObjects + misses) * 0.2);

		c50 = std::min(max300, c50);
	}
	else
		c100 = std::min(max300, c100);

	int c300 = numHitObjects - c100 - c50 - misses;

	return calculatePPv2(osu, beatmap, aim, speed, numHitObjects, numCircles, maxPossibleCombo, combo, misses, c300, c100, c50);
}

double OsuBeatmapDifficulty::calculateAcc(int c300, int c100, int c50, int misses)
{
	int total_hits = c300 + c100 + c50 + misses;
	double acc = 0.0f;

	if (total_hits > 0)
		acc = ((double)c50 * 50.0 + (double)c100 * 100.0 + (double)c300 * 300.0) / ((double)total_hits * 300.0);

	return acc;
}

double OsuBeatmapDifficulty::calculateBaseStrain(double strain)
{
	return std::pow(5.0 * std::max(1.0, strain / 0.0675) - 4.0, 3.0) / 100000.0;
}
