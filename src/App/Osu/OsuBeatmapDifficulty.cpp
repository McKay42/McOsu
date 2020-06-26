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
#include "Timer.h"
#include "MD5.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuReplay.h"
#include "OsuGameRules.h"
#include "OsuDifficultyCalculator.h"
#include "OsuBeatmapStandard.h"
#include "OsuBeatmapMania.h"
#include "OsuNotificationOverlay.h"

#include "OsuHitObject.h"
#include "OsuCircle.h"
#include "OsuSlider.h"
#include "OsuSpinner.h"
#include "OsuManiaNote.h"

ConVar osu_mod_random("osu_mod_random", false);
ConVar osu_mod_random_circle_offset_x_percent("osu_mod_random_circle_offset_x_percent", 1.0f, "how much the randomness affects things");
ConVar osu_mod_random_circle_offset_y_percent("osu_mod_random_circle_offset_y_percent", 1.0f, "how much the randomness affects things");
ConVar osu_mod_random_slider_offset_x_percent("osu_mod_random_slider_offset_x_percent", 1.0f, "how much the randomness affects things");
ConVar osu_mod_random_slider_offset_y_percent("osu_mod_random_slider_offset_y_percent", 1.0f, "how much the randomness affects things");
ConVar osu_mod_random_spinner_offset_x_percent("osu_mod_random_spinner_offset_x_percent", 1.0f, "how much the randomness affects things");
ConVar osu_mod_random_spinner_offset_y_percent("osu_mod_random_spinner_offset_y_percent", 1.0f, "how much the randomness affects things");
ConVar osu_mod_reverse_sliders("osu_mod_reverse_sliders", false);
ConVar osu_show_approach_circle_on_first_hidden_object("osu_show_approach_circle_on_first_hidden_object", true);
ConVar osu_load_beatmap_background_images("osu_load_beatmap_background_images", true);

ConVar osu_slider_curve_max_length("osu_slider_curve_max_length", 65536/2, "maximum slider length in osu!pixels (i.e. pixelLength). also used to clamp all controlpoint coordinates to sane values.");

ConVar osu_stars_stacking("osu_stars_stacking", true, "respect hitobject stacking before calculating stars/pp");



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

		const float localXDivisor = 512.0f / availableColumns;
		return clamp<int>((int)std::floor(position / localXDivisor), 0, availableColumns - 1);
	}
};



class BackgroundImagePathLoader : public Resource
{
public:
	BackgroundImagePathLoader(OsuBeatmapDifficulty *diff) : Resource()
	{
		m_diff = diff;

		m_bAsyncReady = false;
		m_bReady = false;

		m_bDead = false;
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

		// HACKHACK: thread sync, see OsuSongBrowser2.cpp, see OsuBeatmap.cpp, wait until potential calculation going on is finished
		while (m_diff->semaphore)
		{
			m_timer.sleep(10*1000);
		}

		if (OsuBeatmapDifficulty::m_osu_database_dynamic_star_calculation->getBool())
		{
			m_diff->semaphore = true;
			{
				m_diff->loadMetadataRaw(true); // for accurate stars
			}
			m_diff->semaphore = false;
		}

		m_diff->loadBackgroundImagePath();
		m_bAsyncReady = true;
	}

	virtual void destroy() {;}

private:
	static Timer m_timer;

	OsuBeatmapDifficulty *m_diff;
	std::atomic<bool> m_bDead;
};

Timer BackgroundImagePathLoader::m_timer;



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
ConVar *OsuBeatmapDifficulty::m_osu_slider_end_inside_check_offset = NULL;
ConVar *OsuBeatmapDifficulty::m_osu_stars_xexxar_angles_sliders = NULL;
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
	if (m_osu_slider_end_inside_check_offset == NULL)
		m_osu_slider_end_inside_check_offset = convar->getConVarByName("osu_slider_end_inside_check_offset");
	if (m_osu_stars_xexxar_angles_sliders == NULL)
		m_osu_stars_xexxar_angles_sliders = convar->getConVarByName("osu_stars_xexxar_angles_sliders");

	// default values
	version = 14;
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
	numCircles = 0;
	numSliders = 0;
	starsNoMod = 0.0f;
	ID = 0;
	setID = -1;
	starsWereCalculatedAccurately = false;
	semaphore = false;

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

		if (engine->getResourceManager()->isLoadingResource(m_backgroundImagePathLoader))
			while (!m_backgroundImagePathLoader->isAsyncReady()) {;}

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
	/*
	m_aimStrains = std::vector<double>();
	m_speedStrains = std::vector<double>();
	*/
}

bool OsuBeatmapDifficulty::loadMetadataRaw(bool calculateStars, bool calculateStarsInaccurately)
{
	if (env->getOS() == Environment::OS::OS_HORIZON) // causes too much lag on the switch (1)
		calculateStarsInaccurately = true;

	// to avoid double access/calculation by the background image loader and the songbrowser
	bool forceCalculateStars = false;
	if (calculateStars && (starsNoMod == 0.0f || (!starsWereCalculatedAccurately && !calculateStarsInaccurately)))
	{
		if (starsNoMod < 0.0001f) // keep inaccurate stars temporarily to avoid UI flicker
			starsNoMod = 0.0001f;

		forceCalculateStars = true;
	}
	else
		calculateStars = false;

	long rawLengthMS = 0;
	if (!loaded) // NOTE: as long as a diff is fully "loaded" (i.e. while playing), we do not have permission to modify any data (reading is fine though)
	{
		// reset
		unload();
		timingpoints.clear(); // see unload()
		combocolors = std::vector<Color>();

		if (Osu::debug->getBool())
			debugLog("OsuBeatmapDifficulty::loadMetadata() : %s\n", m_sFilePath.toUtf8());

		// generate MD5 hash (loads entire file, very slow)
		if (md5hash.length() < 1)
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
						md5hash += hexDigits[(rawMD5Hash[i] >> 4) & 0xf];	// md5hash[i] / 16
						md5hash += hexDigits[rawMD5Hash[i] & 0xf];			// md5hash[i] % 16
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
				debugLog("Osu Error: Couldn't fopen() file %s\n", m_sFilePath.toUtf8());
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
					case -1: // header (e.g. "osu file format v12")
						{
							sscanf(curLineChar, " osu file format v %i \n", &version);
						}
						break;
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

							sscanf(curLineChar, " BeatmapID : %ld \n", &beatmapId);
							sscanf(curLineChar, " BeatmapSetID : %i \n", &setID);
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
						// new beatmaps: Offset, Milliseconds per Beat, Meter, Sample Type, Sample Set, Volume, !Inherited, Kiai Mode

						double tpOffset;
						float tpMSPerBeat;
						int tpMeter;
						int tpSampleType,tpSampleSet;
						int tpVolume;
						int tpTimingChange;
						int tpKiai;
						if (sscanf(curLineChar, " %lf , %f , %i , %i , %i , %i , %i , %i", &tpOffset, &tpMSPerBeat, &tpMeter, &tpSampleType, &tpSampleSet, &tpVolume, &tpTimingChange, &tpKiai) == 8)
						{
							TIMINGPOINT t;
							t.offset = (long)std::round(tpOffset);
							t.msPerBeat = tpMSPerBeat;
							t.sampleType = tpSampleType;
							t.sampleSet = tpSampleSet;
							t.volume = tpVolume;
							t.timingChange = tpTimingChange == 1;
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

							t.timingChange = true;
							t.kiai = false;

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
							if (time > rawLengthMS)
								rawLengthMS = time;

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
								// NOTE: loading the entire slider is only necessary since the latest pp changes (Xexxar)
								if (m_osu_stars_xexxar_angles_sliders->getBool() && !calculateStarsInaccurately) // NOTE: ignore slider curves when calculating inaccurately
								{
									// HACKHACK: code duplication (see loadRaw())
									UString curLineString = UString(curLineChar);
									std::vector<UString> tokens = curLineString.split(",");
									if (tokens.size() < 8)
										continue;

									std::vector<UString> sliderTokens = tokens[5].split("|");
									if (sliderTokens.size() < 1)
										continue;

									const float sanityRange = osu_slider_curve_max_length.getFloat();
									std::vector<Vector2> points;
									points.push_back(Vector2(clamp<float>(x, -sanityRange, sanityRange), clamp<float>(y, -sanityRange, sanityRange)));
									for (int i=1; i<sliderTokens.size(); i++)
									{
										std::vector<UString> sliderXY = sliderTokens[i].split(":");
										if (sliderXY.size() != 2 || sliderXY[0].find("E") != -1 || sliderXY[0].find("e") != -1 || sliderXY[1].find("E") != -1 || sliderXY[1].find("e") != -1)
											continue;

										points.push_back(Vector2((int)clamp<float>(sliderXY[0].toFloat(), -sanityRange, sanityRange), (int)clamp<float>(sliderXY[1].toFloat(), -sanityRange, sanityRange)));
									}

									if (sliderTokens.size() < 2 && points.size() > 0)
										points.push_back(points[0]);

									SLIDER s;

									s.x = x;
									s.y = y;
									s.time = time;
									s.type = sliderTokens[0][0];
									s.repeat = (int)tokens[6].toFloat();
									s.repeat = s.repeat >= 0 ? s.repeat : 0;
									s.pixelLength = clamp<float>(tokens[7].toFloat(), -sanityRange, sanityRange);

									s.points = points;

									sliders.push_back(s);
								}
								else
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
							}
							else if (type & 0x8) // spinner
							{
								SPINNER s;

								s.x = x;
								s.y = y;
								s.time = time;
								s.endTime = time; // unused for star calculation

								spinners.push_back(s);
							}
						}
						break;
					}
				}
			}
		}

		// gamemode filter
		if ((mode != 0 && m_osu->getGamemode() == Osu::GAMEMODE::STD) || (mode != 0x03 && m_osu->getGamemode() == Osu::GAMEMODE::MANIA))
			return false;

		// build sound file path
		fullSoundFilePath = m_sFolder;
		fullSoundFilePath.append(audioFileName);

		// sort timingpoints and calculate BPM range
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
				const TIMINGPOINT &t = timingpoints[i];

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

			minBPM = (int)std::round(tempMinBPM);
			maxBPM = (int)std::round(tempMaxBPM);
		}

		// old beatmaps: AR = OD, there is no ApproachRate stored
		if (!foundAR)
			AR = OD;
	}

	// calculate default nomod standard stars, and immediately unload everything unnecessary after that
	// also update numObjects in that case since we have them loaded for the star calculation anyway
	if (calculateStars && m_osu_database_dynamic_star_calculation->getBool())
	{
		if (starsNoMod == 0.0f || forceCalculateStars)
		{
			if (!loaded) // NOTE: as long as a diff is fully "loaded" (i.e. while playing), we do not have permission to modify any data (reading is fine though)
			{
				numObjects = hitcircles.size() + sliders.size() + spinners.size();
				numCircles = hitcircles.size();
				numSliders = sliders.size();

				if (lengthMS == 0)
					lengthMS = rawLengthMS;

				// calculate sliderTimes, and build slider clicks and ticks
				// NOTE: only necessary since the latest pp changes (Xexxar)
				if (m_osu_stars_xexxar_angles_sliders->getBool() && !calculateStarsInaccurately) // NOTE: ignore sliders when calculating inaccurately
					calculateAllSliderTimesAndClicksTicks();
			}

			double aimStars = 0.0;
			double speedStars = 0.0;

			/*
			std::vector<double> aimStrains;
			std::vector<double> speedStrains;
			*/

			//starsNoMod = calculateStarDiff(NULL, &aimStars, &speedStars, -1, calculateStarsInaccurately, &aimStrains, &speedStrains);
			starsNoMod = calculateStarDiff(NULL, &aimStars, &speedStars, -1, calculateStarsInaccurately);
			starsWereCalculatedAccurately = (!calculateStarsInaccurately || env->getOS() == Environment::OS::OS_HORIZON); // causes too much lag on the switch (2)

			// reset
			if (!loaded) // NOTE: as long as a diff is fully "loaded" (i.e. while playing), we do not have permission to modify any data (reading is fine though)
			{
				unload();
				combocolors = std::vector<Color>();

				/*
				m_aimStrains.swap(aimStrains);
				m_speedStrains.swap(speedStrains);
				*/
			}

			if (starsNoMod == 0.0f)
				starsNoMod = 0.0001f; // quick hack to avoid reloading endlessly on beatmaps which simply have zero stars (or where the calculation fails)
		}
	}

	return true;
}

bool OsuBeatmapDifficulty::loadRaw(OsuBeatmap *beatmap, std::vector<OsuHitObject*> *hitobjects)
{
	// reset
	unload();
	combocolors = std::vector<Color>();

	// reload metadata
	loadMetadataRaw();
	timingpoints = std::vector<TIMINGPOINT>(); // delete "basic" timingpoints which just got loaded by loadMetadataRaw(), just so we have a clean start

	// open osu file for parsing
	{
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
		int hitobjectsWithoutSpinnerCounter = 0;
		int colorCounter = 1;
		int colorOffset = 0;
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
					// new beatmaps: Offset, Milliseconds per Beat, Meter, Sample Type, Sample Set, Volume, !Inherited, Kiai Mode

					double tpOffset;
					float tpMSPerBeat;
					int tpMeter;
					int tpSampleType,tpSampleSet;
					int tpVolume;
					int tpTimingChange;
					int tpKiai;
					if (sscanf(curLineChar, " %lf , %f , %i , %i , %i , %i , %i , %i", &tpOffset, &tpMSPerBeat, &tpMeter, &tpSampleType, &tpSampleSet, &tpVolume, &tpTimingChange, &tpKiai) == 8)
					{
						TIMINGPOINT t;
						t.offset = (long)std::round(tpOffset);
						t.msPerBeat = tpMSPerBeat;
						t.sampleType = tpSampleType;
						t.sampleSet = tpSampleSet;
						t.volume = tpVolume;
						t.timingChange = tpTimingChange == 1;
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

						t.timingChange = true;
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

					// TODO: calculating combo numbers and color offsets based on the parsing order is dangerous.
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
							HITCIRCLE c;

							c.x = x;
							c.y = y;
							c.time = time;
							c.sampleType = hitSound;
							c.number = comboNumber++;
							c.colorCounter = colorCounter;
							c.colorOffset = colorOffset;
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
							if (sliderTokens.size() < 1) // partially allow bullshit sliders (no controlpoints!), e.g. https://osu.ppy.sh/beatmapsets/791900#osu/1676490
							{
								debugLog("Invalid slider tokens: %s\n\nIn beatmap: %s\n", curLineChar, m_sFilePath.toUtf8());
								continue;
								//engine->showMessageError("Error", UString::format("Invalid slider tokens: %s\n\nIn beatmap: %s", curLineChar, m_sFilePath.toUtf8()));
								//return false;
							}

							const float sanityRange = osu_slider_curve_max_length.getInt(); // infinity sanity check, same as before
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

							// partially allow bullshit sliders (add second point to make valid), e.g. https://osu.ppy.sh/beatmapsets/791900#osu/1676490
							if (sliderTokens.size() < 2 && points.size() > 0)
								points.push_back(points[0]);

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
							c.colorOffset = colorOffset;
							c.clicked = false;
							c.maniaEndTime = holdNoteTokens[0].toLong();

							hitcircles.push_back(c);
						}
					}
					break;
				}
			}
		}
	}

	// check if we have any timingpoints at all
	if (timingpoints.size() == 0)
	{
		UString errorMessage = "Error: No timingpoints in beatmap!";
		debugLog("Osu Error: No timingpoints in beatmap (%s)!\n", m_sFilePath.toUtf8());
		if (m_osu != NULL)
			m_osu->getNotificationOverlay()->addNotification(errorMessage, 0xffff0000);

		return false;
	}

	// update numObjects
	if (numObjects == 0)
		numObjects = hitcircles.size() + sliders.size() + spinners.size();
	if (numCircles == 0)
		numCircles = hitcircles.size();
	if (numSliders == 0)
		numSliders = sliders.size();

	// sort timingpoints by time
	std::sort(timingpoints.begin(), timingpoints.end(), TimingPointSortComparator());

	// calculate sliderTimes, and build slider clicks and ticks
	calculateAllSliderTimesAndClicksTicks();

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

	if (lengthMS == 0 && hitobjects->size() > 0)
		lengthMS = (*hitobjects)[hitobjects->size() - 1]->getTime();

	// set isEndOfCombo + precalculate Score v2 combo portion maximum
	if (beatmapStandard != NULL)
	{
		if (hitobjects->size() > 0)
			m_fScoreV2ComboPortionMaximum = 0;

		int combo = 0;
		for (int i=0; i<hitobjects->size(); i++)
		{
			OsuHitObject *currentHitObject = (*(hitobjects))[i];
			const OsuHitObject *nextHitObject = (i + 1 < hitobjects->size() ? (*(hitobjects))[i + 1] : NULL);

			const OsuCircle *circlePointer = dynamic_cast<OsuCircle*>(currentHitObject);
			const OsuSlider *sliderPointer = dynamic_cast<OsuSlider*>(currentHitObject);
			const OsuSpinner *spinnerPointer = dynamic_cast<OsuSpinner*>(currentHitObject);

			int scoreComboMultiplier = std::max(combo-1, 0);

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

			if (nextHitObject == NULL || nextHitObject->getComboNumber() == 1)
				currentHitObject->setIsEndOfCombo(true);
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

void OsuBeatmapDifficulty::loadBackgroundImage()
{
	m_bShouldBackgroundImageBeLoaded = osu_load_beatmap_background_images.getBool();

	if (m_backgroundImagePathLoader != NULL) // handle loader cleanup
	{
		if (m_backgroundImagePathLoader->isReady())
			deleteBackgroundImagePathLoader();
	}
	else if (backgroundImageName.length() < 1 || (starsNoMod == 0.0f || !starsWereCalculatedAccurately)) // dynamically load background image path and star rating from osu file if it wasn't set by the database
	{
		m_backgroundImagePathLoader = new BackgroundImagePathLoader(this);
		engine->getResourceManager()->requestNextLoadAsync();
		engine->getResourceManager()->loadResource(m_backgroundImagePathLoader);

		return; // we're done here
	}

	if (backgroundImage != NULL || backgroundImageName.length() < 1 || !osu_load_beatmap_background_images.getBool()) return;

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
	 return (m_backgroundImagePathLoader != NULL && !m_backgroundImagePathLoader->isReady());
}

void OsuBeatmapDifficulty::deleteBackgroundImagePathLoader()
{
	SAFE_DELETE(m_backgroundImagePathLoader);
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
			c->x = clamp<int>(c->x - (int)(((rand() % OsuGameRules::OSU_COORD_WIDTH) / 8.0f) * osu_mod_random_circle_offset_x_percent.getFloat()), 0, OsuGameRules::OSU_COORD_WIDTH);
			c->y = clamp<int>(c->y - (int)(((rand() % OsuGameRules::OSU_COORD_HEIGHT) / 8.0f) * osu_mod_random_circle_offset_y_percent.getFloat()), 0, OsuGameRules::OSU_COORD_HEIGHT);
		}

		hitobjects->push_back(new OsuCircle(c->x, c->y, c->time, c->sampleType, c->number, false, c->colorCounter, c->colorOffset, beatmap));

		// potential convert-all-circles-to-sliders mod, have to play around more with this
		/*
		if (i+1 < hitcircles.size())
		{
			std::vector<Vector2> points;
			Vector2 p1 = Vector2(hitcircles[i].x, hitcircles[i].y);
			Vector2 p2 = Vector2(hitcircles[i+1].x, hitcircles[i+1].y);
			points.push_back(p1);
			points.push_back(p2 - (p2 - p1).normalize()*35);
			const float pixelLength = (p2 - p1).length();
			const unsigned long time = hitcircles[i].time;
			const unsigned long timeEnd = hitcircles[i+1].time;
			const unsigned long sliderTime = timeEnd - time;

			bool blocked = false;
			for (int s=0; s<sliders.size(); s++)
			{
				if (sliders[s].time > time && sliders[s].time < timeEnd)
				{
					blocked = true;
					break;
				}
			}
			for (int s=0; s<spinners.size(); s++)
			{
				if (spinners[s].time > time && spinners[s].time < timeEnd)
				{
					blocked = true;
					break;
				}
			}

			blocked |= pixelLength < 45;

			if (!blocked)
				hitobjects->push_back(new OsuSlider(OsuSlider::SLIDERTYPE::SLIDERTYPE_LINEAR, 1, pixelLength, points, std::vector<int>(), std::vector<float>(), sliderTime, sliderTime, time, c->sampleType, c->number, c->colorCounter, c->colorOffset, beatmap));
			else
				hitobjects->push_back(new OsuCircle(c->x, c->y, c->time, c->sampleType, c->number, c->colorCounter, c->colorOffset, beatmap));
		}
		*/
	}
	m_iMaxCombo += hitcircles.size();

	for (int i=0; i<sliders.size(); i++)
	{
		OsuBeatmapDifficulty::SLIDER *s = &sliders[i];

		if (osu_mod_random.getBool())
		{
			for (int p=0; p<s->points.size(); p++)
			{
				s->points[p].x = clamp<int>(s->points[p].x - (int)(((rand() % OsuGameRules::OSU_COORD_WIDTH) / 3.0f) * osu_mod_random_slider_offset_x_percent.getFloat()), 0, OsuGameRules::OSU_COORD_WIDTH);
				s->points[p].y = clamp<int>(s->points[p].y - (int)(((rand() % OsuGameRules::OSU_COORD_HEIGHT) / 3.0f) * osu_mod_random_slider_offset_y_percent.getFloat()), 0, OsuGameRules::OSU_COORD_HEIGHT);
			}
		}

		if (osu_mod_reverse_sliders.getBool())
			std::reverse(s->points.begin(), s->points.end());

		hitobjects->push_back(new OsuSlider(s->type, s->repeat, s->pixelLength, s->points, s->hitSounds, s->ticks, s->sliderTime, s->sliderTimeWithoutRepeats, s->time, s->sampleType, s->number, false, s->colorCounter, s->colorOffset, beatmap));

		const int repeats = std::max((s->repeat - 1), 0);
		m_iMaxCombo += 2 + repeats + (repeats+1)*s->ticks.size(); // start/end + repeat arrow + ticks
	}

	for (int i=0; i<spinners.size(); i++)
	{
		OsuBeatmapDifficulty::SPINNER *s = &spinners[i];

		if (osu_mod_random.getBool())
		{
			s->x = clamp<int>(s->x - (int)(((rand() % OsuGameRules::OSU_COORD_WIDTH) / 1.25f) * (rand() % 2 == 0 ? 1.0f : -1.0f) * osu_mod_random_spinner_offset_x_percent.getFloat()), 0, OsuGameRules::OSU_COORD_WIDTH);
			s->y = clamp<int>(s->y - (int)(((rand() % OsuGameRules::OSU_COORD_HEIGHT) / 1.25f) * (rand() % 2 == 0 ? 1.0f : -1.0f) * osu_mod_random_spinner_offset_y_percent.getFloat()), 0, OsuGameRules::OSU_COORD_HEIGHT);
		}

		hitobjects->push_back(new OsuSpinner(s->x, s->y, s->time, s->sampleType, false, s->endTime, beatmap));
	}
	m_iMaxCombo += spinners.size();

	// debug
	if (m_osu_debug_pp->getBool())
	{
		double aim = 0.0;
		double speed = 0.0;
		double stars = calculateStarDiff(beatmap, &aim, &speed);
		double pp = OsuDifficultyCalculator::calculatePPv2(beatmap->getOsu(), beatmap, aim, speed, hitobjects->size(), hitcircles.size(), m_iMaxCombo);

		engine->showMessageInfo("PP", UString::format("pp = %f, stars = %f, aimstars = %f, speedstars = %f, %i circles, %i sliders, %i spinners, %i hitobjects, maxcombo = %i", pp, stars, aim, speed, hitcircles.size(), sliders.size(), spinners.size(), (hitcircles.size() + sliders.size() + spinners.size()), m_iMaxCombo));
	}
}

void OsuBeatmapDifficulty::buildManiaHitObjects(OsuBeatmapMania *beatmap, std::vector<OsuHitObject*> *hitobjects)
{
	if (beatmap == NULL || hitobjects == NULL) return;

	const int availableColumns = beatmap->getNumColumns();

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

void OsuBeatmapDifficulty::calculateAllSliderTimesAndClicksTicks()
{
	if (timingpoints.size() < 1) return;

	const long osuSliderEndInsideCheckOffset = (long)m_osu_slider_end_inside_check_offset->getInt();

	for (int i=0; i<sliders.size(); i++)
	{
		SLIDER &s = sliders[i];

		// sanity reset
		s.ticks.clear();
		s.scoringTimesForStarCalc.clear();

		// calculate duration
		const TIMING_INFO timingInfo = getTimingInfoForTime(s.time);
		s.sliderTimeWithoutRepeats = getSliderTimeForSlider(s, timingInfo);
		s.sliderTime = s.sliderTimeWithoutRepeats * s.repeat;

		// calculate ticks
		// TODO: validate https://github.com/ppy/osu/pull/3595/files
		const float minTickPixelDistanceFromEnd = 0.01f * getSliderVelocity(s, timingInfo);
		const float tickPixelLength = getSliderTickDistance() / getTimingPointMultiplierForSlider(s, timingInfo);
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
		if (m_osu_stars_xexxar_angles_sliders->getBool())
		{
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

float OsuBeatmapDifficulty::getSliderTickDistance()
{
	return ((100.0f * sliderMultiplier) / sliderTickRate);
}

float OsuBeatmapDifficulty::getSliderTimeForSlider(const SLIDER &slider, const TIMING_INFO &timingInfo)
{
	const float duration = timingInfo.beatLength * (slider.pixelLength / sliderMultiplier) / 100.0f;
	return duration >= 1.0f ? duration : 1.0f; // sanity check
}

float OsuBeatmapDifficulty::getSliderVelocity(const SLIDER &slider, const TIMING_INFO &timingInfo)
{
	const float beatLength = timingInfo.beatLength;
	if (beatLength > 0.0f)
		return (getSliderTickDistance() * sliderTickRate * (1000.0f / beatLength));
	else
		return getSliderTickDistance() * sliderTickRate;
}

float OsuBeatmapDifficulty::getTimingPointMultiplierForSlider(const SLIDER &slider, const TIMING_INFO &timingInfo)
{
	float beatLengthBase = timingInfo.beatLengthBase;
	if (beatLengthBase == 0.0f) // sanity check
		beatLengthBase = 1.0f;

	return timingInfo.beatLength / beatLengthBase;
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

unsigned long OsuBeatmapDifficulty::getBreakDurationTotal()
{
	unsigned long breakDurationTotal = 0;
	for (int i=0; i<breaks.size(); i++)
	{
		breakDurationTotal += (unsigned long)(breaks[i].endTime - breaks[i].startTime);
	}

	return breakDurationTotal;
}

OsuBeatmapDifficulty::BREAK OsuBeatmapDifficulty::getBreakForTimeRange(long startMS, long positionMS, long endMS)
{
	BREAK curBreak;

	curBreak.startTime = -1;
	curBreak.endTime = -1;

	for (int i=0; i<breaks.size(); i++)
	{
		if (breaks[i].startTime >= (int)startMS && breaks[i].endTime <= (int)endMS)
		{
			if ((int)positionMS >= curBreak.startTime)
				curBreak = breaks[i];
		}
	}

	return curBreak;
}

std::vector<std::shared_ptr<OsuDifficultyHitObject>> OsuBeatmapDifficulty::generateDifficultyHitObjectsForBeatmap(OsuBeatmap *beatmap, bool calculateStarsInaccurately)
{
	// build generalized OsuDifficultyHitObjects from the vectors (hitcircles, sliders, spinners)
	// the OsuDifficultyHitObject class is the one getting used in all pp/star calculations, it encompasses every object type for simplicity

	std::vector<std::shared_ptr<OsuDifficultyHitObject>> diffHitObjects;
	diffHitObjects.reserve(hitcircles.size() + sliders.size() + spinners.size());

	for (int i=0; i<hitcircles.size(); i++)
	{
		diffHitObjects.push_back(std::make_shared<OsuDifficultyHitObject>(
				OsuDifficultyHitObject::TYPE::CIRCLE,
				Vector2(hitcircles[i].x, hitcircles[i].y),
				(long)hitcircles[i].time));
	}

	for (int i=0; i<sliders.size(); i++)
	{
		if (!calculateStarsInaccurately)
		{
			diffHitObjects.push_back(std::make_shared<OsuDifficultyHitObject>(
					OsuDifficultyHitObject::TYPE::SLIDER,
					Vector2(sliders[i].x, sliders[i].y),
					sliders[i].time,
					sliders[i].time + (long)sliders[i].sliderTime,
					sliders[i].sliderTimeWithoutRepeats,
					sliders[i].type,
					sliders[i].points,
					sliders[i].pixelLength,
					sliders[i].scoringTimesForStarCalc));
		}
		else
		{
			diffHitObjects.push_back(std::make_shared<OsuDifficultyHitObject>(
					OsuDifficultyHitObject::TYPE::SLIDER,
					Vector2(sliders[i].x, sliders[i].y),
					sliders[i].time,
					sliders[i].time + (long)sliders[i].sliderTime,
					sliders[i].sliderTimeWithoutRepeats,
					sliders[i].type,
					std::vector<Vector2>(),	// NOTE: ignore curve when calculating inaccurately
					sliders[i].pixelLength,
					std::vector<long>()));	// NOTE: ignore curve when calculating inaccurately
		}
	}

	for (int i=0; i<spinners.size(); i++)
	{
		diffHitObjects.push_back(std::make_shared<OsuDifficultyHitObject>(
				OsuDifficultyHitObject::TYPE::SPINNER,
				Vector2(spinners[i].x, spinners[i].y),
				(long)spinners[i].time,
				(long)spinners[i].endTime));
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
	if (osu_stars_stacking.getBool() && !calculateStarsInaccurately) // NOTE: ignore stacking when calculating inaccurately
	{
		const float finalAR = (beatmap != NULL ? beatmap->getAR() : AR);
		const float finalCS = (beatmap != NULL ? beatmap->getCS() : CS);
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

	// apply speed multiplier
	if (beatmap != NULL)
	{
		if (beatmap->getOsu()->getSpeedMultiplier() != 1.0f && beatmap->getOsu()->getSpeedMultiplier() > 0.0f)
		{
			const double speedMultiplier = 1.0 / (double)beatmap->getOsu()->getSpeedMultiplier();
			for (int i=0; i<diffHitObjects.size(); i++)
			{
				diffHitObjects[i]->time = (long)((double)diffHitObjects[i]->time * speedMultiplier);
				diffHitObjects[i]->endTime = (long)((double)diffHitObjects[i]->endTime * speedMultiplier);

				if (!calculateStarsInaccurately) // NOTE: ignore slider curves when calculating inaccurately
				{
					diffHitObjects[i]->spanDuration = (double)diffHitObjects[i]->spanDuration * speedMultiplier;
					for (int s=0; s<diffHitObjects[i]->scoringTimes.size(); s++)
					{
						diffHitObjects[i]->scoringTimes[s] = (long)((double)diffHitObjects[i]->scoringTimes[s] * speedMultiplier);
					}
				}
			}
		}
	}

	return diffHitObjects;
}

double OsuBeatmapDifficulty::calculateStarDiff(OsuBeatmap *beatmap, double *aim, double *speed, int upToObjectIndex, bool calculateStarsInaccurately, std::vector<double> *outAimStrains, std::vector<double> *outSpeedStrains)
{
	std::vector<std::shared_ptr<OsuDifficultyHitObject>> hitObjects = generateDifficultyHitObjectsForBeatmap(beatmap, calculateStarsInaccurately);
	return OsuDifficultyCalculator::calculateStarDiffForHitObjects(hitObjects, (beatmap != NULL ? beatmap->getCS() : CS), aim, speed, upToObjectIndex, outAimStrains, outSpeedStrains);
}

void OsuBeatmapDifficulty::rebuildStarCacheForUpToHitObjectIndex(OsuBeatmap *beatmap, std::atomic<bool> &interruptLoad, std::atomic<int> &progress)
{
	// precalculate cut star values for live pp

	// reset
	m_aimStarsForNumHitObjects.clear();
	m_speedStarsForNumHitObjects.clear();
	/*
	m_aimStrains.clear();
	m_speedStrains.clear();
	*/

	std::vector<std::shared_ptr<OsuDifficultyHitObject>> hitObjects = generateDifficultyHitObjectsForBeatmap(beatmap);
	const float CS = beatmap->getCS();

	for (int i=0; i<hitObjects.size(); i++)
	{
		double aimStars = 0.0;
		double speedStars = 0.0;

		//OsuDifficultyCalculator::calculateStarDiffForHitObjects(hitObjects, CS, &aimStars, &speedStars, i, (i == (hitObjects.size() - 1) ? &m_aimStrains : NULL), (i == (hitObjects.size() - 1) ? &m_speedStrains : NULL));
		OsuDifficultyCalculator::calculateStarDiffForHitObjects(hitObjects, CS, &aimStars, &speedStars, i);

		m_aimStarsForNumHitObjects.push_back(aimStars);
		m_speedStarsForNumHitObjects.push_back(speedStars);

		progress = i;

		if (interruptLoad.load())
		{
			m_aimStarsForNumHitObjects.clear();
			m_speedStarsForNumHitObjects.clear();
			/*
			m_aimStrains.clear();
			m_speedStrains.clear();
			*/

			return; // stop everything
		}
	}
}
