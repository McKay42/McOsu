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
#include "OsuNotificationOverlay.h"

#include "OsuHitObject.h"
#include "OsuCircle.h"
#include "OsuSlider.h"
#include "OsuSpinner.h"

ConVar osu_mod_random("osu_mod_random", false);
ConVar osu_show_approach_circle_on_first_hidden_object("osu_show_approach_circle_on_first_hidden_object", true);



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

		m_diff->loadBackgroundImagePath();
		m_bAsyncReady = true;
	}
	virtual void destroy() {;}

private:
	OsuBeatmapDifficulty *m_diff;
	bool m_bDead;
};



ConVar *OsuBeatmapDifficulty::m_osu_slider_scorev2 = NULL;

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
	starsNoMod = 0;
	ID = 0;
	setID = 0;

	m_backgroundImagePathLoader = NULL;

	m_iMaxCombo = 0;
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
}

bool OsuBeatmapDifficulty::loadMetadataRaw()
{
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
	while (file.canRead())
	{
		UString uCurLine = file.readLine();
		const char *curLineChar = uCurLine.toUtf8();
		std::string curLine(curLineChar);

		if (curLine.find("//") == std::string::npos) // ignore comments
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
				break; // stop early

			switch (curBlock)
			{
			case 0: // General
				{
					char stringBuffer[1024];
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
					char stringBuffer[1024];
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
					char stringBuffer[1024];
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
				if (sscanf(curLineChar, " %lf , %f , %i , %i , %i , %i", &tpOffset, &tpMSPerBeat, &tpMeter, &tpSampleType, &tpSampleSet, &tpVolume) == 6)
				{
					TIMINGPOINT t;
					t.offset = (long)std::round(tpOffset);
					t.msPerBeat = tpMSPerBeat;
					t.sampleType = tpSampleType;
					t.sampleSet = tpSampleSet;
					t.volume = tpVolume;

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

					timingpoints.push_back(t);
				}
				break;
			}
		}
	}

	// only allow osu!standard diffs for now
	if (mode != 0)
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
		struct TimingPointSortComparator
		{
		    bool operator() (TIMINGPOINT const &a, TIMINGPOINT const &b) const
		    {
		    	// first condition: offset
		    	// second condition: if offset is the same, non-inherited timingpoints go before inherited timingpoints
		        return (a.offset < b.offset) || (a.offset == b.offset && a.msPerBeat >= 0 && b.msPerBeat < 0);
		    }
		};
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
		UString errorMessage = "Error: Couldn't load beatmap file";
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

	while (file.canRead())
	{
		UString uCurLine = file.readLine();
		const char *curLineChar = uCurLine.toUtf8();
		std::string curLine(curLineChar);

		if (curLine.find("//") == std::string::npos) // ignore comments
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
				if (sscanf(curLineChar, " %lf , %f , %i , %i , %i , %i", &tpOffset, &tpMSPerBeat, &tpMeter, &tpSampleType, &tpSampleSet, &tpVolume) == 6)
				{
					TIMINGPOINT t;
					t.offset = (long)std::round(tpOffset);
					t.msPerBeat = tpMSPerBeat;
					t.sampleType = tpSampleType;
					t.sampleSet = tpSampleSet;
					t.volume = tpVolume;

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
	struct TimingPointSortComparator
	{
	    bool operator() (TIMINGPOINT const &a, TIMINGPOINT const &b) const
	    {
	    	// first condition: offset
	    	// second condition: if offset is the same, non-inherited timingpoints go before inherited timingpoints
	        return (a.offset < b.offset) || (a.offset == b.offset && a.msPerBeat >= 0 && b.msPerBeat < 0);
	    }
	};
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
		float tickLengthDiv = ((100.0f * sliderMultiplier) / sliderTickRate) / getTimingPointMultiplierForSlider(s);
		int tickCount = (int) std::ceil(s->pixelLength / tickLengthDiv) - 1;
		if (tickCount > 0)
		{
			float tickTOffset = 1.0f / (tickCount + 1);
			float t = tickTOffset;
			for (int i=0; i<tickCount; i++, t+=tickTOffset)
			{
				s->ticks.push_back(t);
			}
		}
	}

	// build hitobjects from the data we loaded from the beatmap file
	OsuBeatmapStandard *beatmapStandard = dynamic_cast<OsuBeatmapStandard*>(beatmap);
	if (beatmapStandard != NULL)
		buildStandardHitObjects(beatmapStandard, hitobjects);

	// sort hitobjects by starttime
	struct HitObjectSortComparator
	{
	    bool operator() (OsuHitObject const *a, OsuHitObject const *b) const
	    {
	        return a->getTime() < b->getTime();
	    }
	};
	std::sort(hitobjects->begin(), hitobjects->end(), HitObjectSortComparator());

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
	m_bShouldBackgroundImageBeLoaded = true;

	if (m_backgroundImagePathLoader != NULL) // handle loader cleanup
	{
		if (m_backgroundImagePathLoader->isReady())
			deleteBackgroundImagePathLoader();
	}
	else if (backgroundImageName.length() < 1) // dynamically load background image path from osu file if it wasn't set by the database
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
	while (file.canRead())
	{
		UString uCurLine = file.readLine();
		const char *curLineChar = uCurLine.toUtf8();
		std::string curLine(curLineChar);

		if (curLine.find("//") == std::string::npos) // ignore comments
		{
			if (curLine.find("[Events]") != std::string::npos)
				curBlock = 0;
			else if (curLine.find("[Colours]") != std::string::npos)
				break; // stop early

			switch (curBlock)
			{
			case 0: // Events
				{
					char stringBuffer[1024];
					memset(stringBuffer, '\0', 1024);
					float temp;
					if (sscanf(curLineChar, " %f , %f , \"%1023[^\"]\"", &temp, &temp, stringBuffer) == 3)
					{
						backgroundImageName = UString(stringBuffer);
					}
				}
				break;
			}
		}
	}
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
	/*
	double aim = 0.0;
	double speed = 0.0;
	double rhythm_awkwardness = 0.0;
	double stars = calculateStarDiff(beatmap, &aim, &speed, &rhythm_awkwardness);
	double pp = calculatePPv2(beatmap, aim, speed);

	engine->showMessageInfo("PP", UString::format("stars = %f, pp = %f, aim = %f, speed = %f, awk = %f, %i circles, %i sliders, %i spinners", stars, pp, aim, speed, rhythm_awkwardness, hitcircles.size(), sliders.size(), spinners.size()));
	*/
}

float OsuBeatmapDifficulty::getSliderTimeForSlider(SLIDER *slider)
{
	const float duration = getTimingInfoForTime(slider->time).beatLength * (slider->pixelLength / sliderMultiplier) / 100.0f;
	return duration >= 0.0f ? duration : 0.001f; // sanity check
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
	ti.volume = timingpoints[0].volume;
	ti.sampleSet = timingpoints[0].sampleSet;
	ti.sampleType = timingpoints[0].sampleType;

	// initial timing values (get first non-inherited timingpoint as base)
	for (int i=0; i<timingpoints.size(); i++)
	{
		TIMINGPOINT *t = &timingpoints[i];
		if (t->msPerBeat >= 0)
		{
			ti.beatLength = std::abs(t->msPerBeat);
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

		if (t->msPerBeat >= 0) // NOT inherited
		{
			ti.beatLengthBase = ti.beatLength = t->msPerBeat;
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

double OsuBeatmapDifficulty::calculateStarDiff(OsuBeatmap *beatmap, double *aim, double *speed, double *rhythm_awkwardness)
{
	// TODO: compiling the exact same algorithm with mingw-w64 g++ somehow corrupts it (incorrect values, even with 100% the original code!), still have to find out why. first bumping the linux release to 28.6 though
	/*
	// build generalized hit_object structs from the vectors (hitcircles, sliders, spinners)
	// the hit_object struct is the one getting used in all calculations, it encompasses every object type
	enum class obj : char
	{
		invalid = 0,
		circle,
		spinner,
		slider,
	};

	struct hit_object
	{
		Vector2 pos;
		long time = 0;
		obj type = obj::invalid;
		long end_time = 0; // for spinners and sliders
		//slider_data slider;
	};

	std::vector<hit_object> objects;

	for (int i=0; i<hitcircles.size(); i++)
	{
		hit_object ho;
		ho.pos = Vector2(hitcircles[i].x, hitcircles[i].y);
		ho.time = (long)hitcircles[i].time;
		ho.type = obj::circle;
		ho.end_time = (long)ho.time;
		objects.push_back(ho);
	}

	for (int i=0; i<sliders.size(); i++)
	{
		hit_object ho;
		ho.pos = Vector2(sliders[i].x, sliders[i].y);
		ho.time = sliders[i].time;
		ho.type = obj::slider;
		ho.end_time = sliders[i].time + sliders[i].sliderTime; // start + duration
		objects.push_back(ho);
	}

	for (int i=0; i<spinners.size(); i++)
	{
		hit_object ho;
		ho.pos = Vector2(spinners[i].x, spinners[i].y);
		ho.time = spinners[i].time;
		ho.type = obj::spinner;
		ho.end_time = spinners[i].endTime;
		objects.push_back(ho);
	}

	if (objects.size() < 1)
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
	};

	// diffcalc hit object
	struct d_obj
	{
		hit_object *ho;

		// strains start at 1
		double strains[2] = {1, 1};

		// start/end positions normalized on radius
		Vector2 norm_start;
		Vector2 norm_end;

		void init(hit_object *base_object, float radius)
		{
			this->ho = base_object;

			// positions are normalized on circle radius so that we can calc as
			// if everything was the same circlesize
			float scaling_factor = 52.0f / radius;

			// cs buff (credits to osuElements, I have confirmed that this is
			// indeed accurate)
			if (radius < circlesize_buff_treshold)
				scaling_factor *= 1.f + std::min((circlesize_buff_treshold - radius), 5.f) / 50.f;

			norm_start = ho->pos * scaling_factor;

			norm_end = norm_start;
			// ignoring slider lengths doesn't seem to affect star rating too much and speeds up the calculation exponentially
		}

		void calculate_strains(d_obj *prev)
		{
			calculate_strain(prev, cdiff::diff::speed);
			calculate_strain(prev, cdiff::diff::aim);
		}

		void calculate_strain(d_obj *prev, cdiff::diff dtype)
		{
			double res = 0;
			long time_elapsed = ho->time - prev->ho->time;
			double decay = std::pow(decay_base[dtype], time_elapsed / 1000.0);
			double scaling = weight_scaling[dtype];

			switch (ho->type) {
				case obj::slider: // we don't use sliders in this implementation
				case obj::circle:
					res = spacing_weight(distance(prev), dtype) * scaling;
					break;

				case obj::spinner:
					break;

				case obj::invalid:
					// NOTE: silently error out
					return;
			}

			res /= (double)std::max(time_elapsed, (long)50);
			strains[dtype] = prev->strains[dtype] * decay + res;
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

		double distance(d_obj *prev)
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
		static double calculate_difficulty(cdiff::diff type, std::vector<d_obj> *dobjects)
		{
			std::vector<double> highest_strains;
			long interval_end = strain_step;
			double max_strain = 0.0;

			d_obj *prev = nullptr;
			for (size_t i=0; i<dobjects->size(); i++)
			{
				d_obj *o = &(*dobjects)[i];

				// make previous peak strain decay until the current object
				while (o->ho->time > interval_end)
				{
					highest_strains.push_back(max_strain);

					if (!prev)
						max_strain = 0.0;
					else
					{
						double decay = std::pow(decay_base[type], (interval_end - prev->ho->time) / 1000.0);
						max_strain = prev->strains[type] * decay;
					}

					interval_end += strain_step;
				}

				// calculate max strain for this interval
				max_strain = std::max(max_strain, o->strains[type]);
				prev = o;
			}

			double difficulty = 0.0;
			double weight = 1.0;

			// sort strains from greatest to lowest
			std::sort(highest_strains.begin(), highest_strains.end(), std::greater<double>()); // TODO: get rid of std::greater (y tho)

			// weigh the top strains
			for (const double strain : highest_strains)
			{
				difficulty += weight * strain;
				weight *= decay_weight;
			}

			return difficulty;
		}
	};

	// ****************************************************************************************************************************************** //

	float circle_radius = OsuGameRules::getRawHitCircleDiameter(beatmap->getCS()) / 2.0f;
	int numObjects = hitcircles.size() + sliders.size() + spinners.size();

	// initialize dobjects
	std::vector<d_obj> dobjects;
	for (size_t i=0; i<numObjects && i<objects.size(); i++)
	{
		d_obj dobj;
		dobj.init(&objects[i], circle_radius);
		dobjects.push_back(dobj);
	}

	std::vector<long> intervals;

	d_obj *prev = &dobjects[0];
	for (size_t i=1; i<dobjects.size(); i++)
	{
		d_obj *o = &dobjects[i];

		o->calculate_strains(prev);

		intervals.push_back(o->ho->time - prev->ho->time);

		prev = o;
	}

	std::vector<long> group;

	unsigned long noffsets = 0;
	*rhythm_awkwardness = 0;
	for (size_t i=0; i<intervals.size(); ++i)
	{
		// TODO: actually compute break time length for the map's AR
		bool isbreak = intervals[i] >= 1200;

		if (!isbreak)
			group.push_back(intervals[i]);

		if (isbreak || group.size() >= 5 || i == intervals.size() - 1)
		{
			for (size_t j=0; j<group.size(); ++j)
			{
				for (size_t k = 1; k < group.size(); ++k)
				{
					if (k == j)
						continue;

					double ratio = group[j] > group[k] ?
						(double)group[j] / (double)group[k] :
						(double)group[k] / (double)group[j];

					double closest_pot = std::pow(2, std::round(std::log(ratio)/std::log(2)));

					double offset = std::abs(closest_pot - ratio);
					offset /= closest_pot;

					*rhythm_awkwardness += offset * offset;

					++noffsets;
				}
			}
			group.clear();
		}
	}

	*rhythm_awkwardness /= noffsets;
	*rhythm_awkwardness *= 82;

	*aim = DiffCalc::calculate_difficulty(cdiff::diff::aim, &dobjects);
	*speed = DiffCalc::calculate_difficulty(cdiff::diff::speed, &dobjects);
	*aim = std::sqrt(*aim) * star_scaling_factor;
	*speed = std::sqrt(*speed) * star_scaling_factor;

	double stars = *aim + *speed + std::abs(*speed - *aim) * extreme_scaling_factor;
	return stars;
	*/

	return 0.0;
}

double OsuBeatmapDifficulty::calculatePPv2(OsuBeatmap *beatmap, double aim, double speed, int combo, int misses, int c300, int c100, int c50/*, SCORE_VERSION scoreVersion*/)
{
	SCORE_VERSION scoreVersion = m_osu_slider_scorev2->getBool() ? SCORE_VERSION::SCORE_V2 : SCORE_VERSION::SCORE_V1;

	double od = beatmap->getOD();
	double ar = beatmap->getAR();

	int circles = hitcircles.size();
	int numObjects = hitcircles.size() + sliders.size() + spinners.size();

	if (c300 < 0)
		c300 = numObjects - c100 - c50 - misses;

	if (combo < 0)
		combo = getMaxCombo();

	if (combo < 1)
		return 0.0f;

	int total_hits = c300 + c100 + c50 + misses;
	///if (total_hits != numObjects)
	///	debugLog("OsuBeatmapDifficulty::calculatePPv2() WARNING : Total hits %i don't match object count %i!\n", total_hits, numObjects);

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
	double combo_break = std::pow((double)combo, 0.8) / std::pow((double)getMaxCombo(), 0.8);

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

		if (m_osu->getModHD())
			low_ar_bonus *= 2.0;

		ar_bonus += low_ar_bonus;
	}

	aim_value *= ar_bonus;

	// hidden
	if (m_osu->getModHD())
		aim_value *= 1.18;

	// flashlight
	// TODO: not yet implemented
	/*
	if (used_mods & mods::fl)
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
	speed_value *= acc_bonus;
	speed_value *= od_bonus;

	// acc pp ------------------------------------------------------------------
	double real_acc = 0.0; // accuracy calculation changes from scorev1 to scorev2

	if (scoreVersion == SCORE_VERSION::SCORE_V2)
	{
		circles = total_hits;
		real_acc = acc;
	}
	else
	{
		// scorev1 ignores sliders since they are free 300s
		if (circles)
		{
			real_acc = (
					(c300 - (total_hits - circles)) * 300.0
					+ c100 * 100.0
					+ c50 * 50.0
					)
					/ (circles * 300);
		}

		// can go negative if we miss everything
		real_acc = std::max(0.0, real_acc);
	}

	// arbitrary values tom crafted out of trial and error
	double acc_value = std::pow(1.52163, od) * std::pow(real_acc, 24.0) * 2.83;

	// length bonus (not the same as speed/aim length bonus)
	acc_value *= std::min(1.15, std::pow(circles / 1000.0, 0.3));

	// hidden bonus
	if (m_osu->getModHD()) {
		acc_value *= 1.02;
	}

	// flashlight bonus
	// TODO: not yet implemented
	/*
	if (used_mods & mods::fl) {
		acc_value *= 1.02;
	}
	*/

	// total pp ----------------------------------------------------------------
	double final_multiplier = 1.12;

	// nofail
	if (m_osu->getModNF())
		final_multiplier *= 0.90;

	// spunout
	if (m_osu->getModSpunout())
		final_multiplier *= 0.95;

	return	std::pow(
			std::pow(aim_value, 1.1) +
			std::pow(speed_value, 1.1) +
			std::pow(acc_value, 1.1),
			1.0 / 1.1
		) * final_multiplier;
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
