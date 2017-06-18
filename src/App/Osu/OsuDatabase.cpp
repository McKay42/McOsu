//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		osu!.db + collection.db + raw loader + scores etc.
//
// $NoKeywords: $osubdb
//===============================================================================//

#include "OsuDatabase.h"

#include "Engine.h"
#include "ConVar.h"
#include "Timer.h"
#include "ResourceManager.h"

#include "Osu.h"
#include "OsuFile.h"
#include "OsuNotificationOverlay.h"
#include "OsuBeatmap.h"
#include "OsuBeatmapDifficulty.h"
#include "OsuBeatmapStandard.h"
#include "OsuBeatmapExample.h"

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__CYGWIN__) || defined(__CYGWIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)

ConVar osu_folder("osu_folder", "C:/Program Files (x86)/osu!/");

#elif defined __linux__

ConVar osu_folder("osu_folder", "/media/pg/Win7/Program Files (x86)/osu!/");

#elif defined __APPLE__

ConVar osu_folder("osu_folder", "/osu!/");

#else

#error "put correct default folder convar here"

#endif

ConVar osu_database_enabled("osu_database_enabled", true);

class OsuDatabaseLoader : public Resource
{
public:
	OsuDatabaseLoader(OsuDatabase *db) : Resource()
	{
		m_db = db;
		m_bNeedRawLoad = false;

		m_bAsyncReady = false;
		m_bReady = false;
	};

protected:
	virtual void init()
	{
		// legacy loading, if db is not found or by convar
		if (m_bNeedRawLoad)
			m_db->loadRaw();

		m_bReady = true;
		delete this; // commit sudoku
	}

	virtual void initAsync()
	{
		// check if osu database exists, load file completely
		UString filePath = osu_folder.getString();
		filePath.append("osu!.db");
		OsuFile *db = new OsuFile(filePath);

		// load database
		if (db->isReady() && osu_database_enabled.getBool())
		{
			m_db->m_fLoadingProgress = 0.25f;
			m_db->loadDB(db);
		}
		else
			m_bNeedRawLoad = true;

		// cleanup
		SAFE_DELETE(db);

		m_bAsyncReady = true;
	}

	virtual void destroy() {;}

private:
	OsuDatabase *m_db;
	bool m_bNeedRawLoad;
};

OsuDatabase::OsuDatabase(Osu *osu)
{
	m_importTimer = new Timer();
	m_bIsFirstLoad = true;
	m_bFoundChanges = true;

	m_iNumBeatmapsToLoad = 0;
	m_fLoadingProgress = 0.0f;
	m_bKYS = false;

	m_osu = osu;
	m_iVersion = 0;
	m_iFolderCount = 0;
	m_sPlayerName = "";

	m_iCurRawBeatmapLoadIndex = 0;
	m_bRawBeatmapLoadScheduled = false;
}

OsuDatabase::~OsuDatabase()
{
	for (int i=0; i<m_beatmaps.size(); i++)
	{
		delete m_beatmaps[i];
	}
}

void OsuDatabase::reset()
{
	m_collections.clear();
	for (int i=0; i<m_beatmaps.size(); i++)
	{
		delete m_beatmaps[i];
	}
	m_beatmaps.clear();

	m_bIsFirstLoad = true;
	m_bFoundChanges = true;

	m_osu->getNotificationOverlay()->addNotification("Rebuilding.", 0xff00ff00);
}

void OsuDatabase::update()
{
	// loadRaw() logic
	if (m_bRawBeatmapLoadScheduled)
	{
		Timer t;
		t.start();

		while (t.getElapsedTime() < 0.033f)
		{
			if (m_bKYS.load()) break; // cancellation point

			if (m_rawLoadBeatmapFolders.size() > 0)
			{
				UString curBeatmap = m_rawLoadBeatmapFolders[m_iCurRawBeatmapLoadIndex++];
				m_rawBeatmapFolders.push_back(curBeatmap); // for future incremental loads, so that we know what's been loaded already

				UString fullBeatmapPath = m_sRawBeatmapLoadOsuSongFolder;
				fullBeatmapPath.append(curBeatmap);
				fullBeatmapPath.append("/");

				// try to load it
				OsuBeatmap *bm = loadRawBeatmap(fullBeatmapPath);

				// if successful, add it
				if (bm != NULL)
					m_beatmaps.push_back(bm);
			}

			// update progress
			m_fLoadingProgress = (float)m_iCurRawBeatmapLoadIndex / (float)m_iNumBeatmapsToLoad;

			// check if we are finished
			if (m_iCurRawBeatmapLoadIndex >= m_iNumBeatmapsToLoad)
			{
				m_rawLoadBeatmapFolders.clear();
				m_bRawBeatmapLoadScheduled = false;
				m_importTimer->update();
				debugLog("Refresh finished, added %i beatmaps in %f seconds.\n", m_beatmaps.size(), m_importTimer->getElapsedTime());
				break;
			}

			t.update();
		}
	}
}

void OsuDatabase::load()
{
	m_bKYS = false;
	m_fLoadingProgress = 0.0f;
	OsuDatabaseLoader *loader = new OsuDatabaseLoader(this);
	engine->getResourceManager()->requestNextLoadAsync();
	engine->getResourceManager()->loadResource(loader);
}

void OsuDatabase::cancel()
{
	m_bKYS = true;
	m_bRawBeatmapLoadScheduled = false;
	m_fLoadingProgress = 1.0f; // force finished
	m_bFoundChanges = true;
}

void OsuDatabase::loadRaw()
{
	m_sRawBeatmapLoadOsuSongFolder = osu_folder.getString();
	m_sRawBeatmapLoadOsuSongFolder.append("Songs/");

	m_rawLoadBeatmapFolders = env->getFoldersInFolder(m_sRawBeatmapLoadOsuSongFolder);
	m_iNumBeatmapsToLoad = m_rawLoadBeatmapFolders.size();

	// if this isn't the first load, only load the differences
	if (!m_bIsFirstLoad)
	{
		std::vector<UString> toLoad;
		for (int i=0; i<m_iNumBeatmapsToLoad; i++)
		{
			if (i < m_rawBeatmapFolders.size())
			{
				if (m_rawBeatmapFolders[i] == m_rawLoadBeatmapFolders[i])
					continue;
				else
					toLoad.push_back(m_rawLoadBeatmapFolders[i]);
			}
			else
				toLoad.push_back(m_rawLoadBeatmapFolders[i]);
		}

		// only load differences
		m_rawLoadBeatmapFolders = toLoad;
		m_iNumBeatmapsToLoad = m_rawLoadBeatmapFolders.size();

		debugLog("Database: Found %i new/changed beatmaps.\n", m_iNumBeatmapsToLoad);

		m_bFoundChanges = m_iNumBeatmapsToLoad > 0;
		if (m_bFoundChanges)
			m_osu->getNotificationOverlay()->addNotification(UString::format(m_iNumBeatmapsToLoad == 1 ? "Adding %i new beatmap." : "Adding %i new beatmaps.", m_iNumBeatmapsToLoad), 0xff00ff00);
		//else
		//	m_osu->getNotificationOverlay()->addNotification(UString::format("No new beatmaps detected.", m_iNumBeatmapsToLoad), 0xff00ff00);
	}

	debugLog("Database: Building beatmap database ...\n");
	debugLog("Database: Found %i folders to load.\n", m_rawLoadBeatmapFolders.size());

	// only start loading if we have something to load
	if (m_rawLoadBeatmapFolders.size() > 0)
	{
		m_fLoadingProgress = 0.0f;
		m_iCurRawBeatmapLoadIndex = 0;

		m_bRawBeatmapLoadScheduled = true;
		m_importTimer->start();
	}
	else
		m_fLoadingProgress = 1.0f;

	m_bIsFirstLoad = false;
}

void OsuDatabase::loadDB(OsuFile *db)
{
	// reset
	m_collections.clear();
	for (int i=0; i<m_beatmaps.size(); i++)
	{
		delete m_beatmaps[i];
	}
	m_beatmaps.clear();

	if (!db->isReady())
	{
		debugLog("Database: Couldn't read, database not ready!\n");
		return;
	}

	m_importTimer->start();

	// read header
	m_iVersion = db->readInt();
	m_iFolderCount = db->readInt();
	db->readBool();
	db->readDateTime();
	m_sPlayerName = db->readString();
	m_iNumBeatmapsToLoad = db->readInt();

	debugLog("Database: version = %i, folderCount = %i, playerName = %s, numDiffs = %i\n", m_iVersion, m_iFolderCount, m_sPlayerName.toUtf8(), m_iNumBeatmapsToLoad);
	if (m_iVersion < 20140609)
	{
		debugLog("Database: Version is below 20140609, not supported.\n");
		m_osu->getNotificationOverlay()->addNotification("osu!.db version too old, update osu! and try again!", 0xffff0000);
		return;
	}
	if (m_iVersion < 20170222)
	{
		debugLog("Database: Version is quite old, below 20170222 ...\n");
		m_osu->getNotificationOverlay()->addNotification("WARNING: osu!.db version is old, please update osu!", 0xffff0000);
	}

	// read beatmapInfos
	struct BeatmapSet
	{
		int setID;
		UString path;
		std::vector<OsuBeatmapDifficulty*> diffs;
	};
	UString songFolder = osu_folder.getString();
	songFolder.append("Songs/");
	std::vector<BeatmapSet> beatmapSets;
	for (int i=0; i<m_iNumBeatmapsToLoad; i++)
	{
		if (m_bKYS.load()) break; // cancellation point

		if (Osu::debug->getBool())
			debugLog("Database: Reading beatmap %i/%i ...\n", (i+1), m_iNumBeatmapsToLoad);

		m_fLoadingProgress = 0.25f + 0.5f*((float)(i+1)/(float)m_iNumBeatmapsToLoad);

		unsigned int size = db->readInt();
		UString artistName = db->readString();
		UString artistNameUnicode = db->readString();
		UString songTitle = db->readString();
		UString songTitleUnicode = db->readString();
		UString creatorName = db->readString();
		UString difficultyName = db->readString();
		UString audioFileName = db->readString();
		UString md5hash = db->readString();
		UString osuFileName = db->readString();
		unsigned char rankedStatus = db->readByte();
		unsigned short numCircles = db->readShort();
		unsigned short numSliders = db->readShort();
		unsigned short numSpinners = db->readShort();
		long long lastModificationTime = db->readLongLong();
		float AR = db->readFloat();
		float CS = db->readFloat();
		float HP = db->readFloat();
		float OD = db->readFloat();
		double sliderMultiplier = db->readDouble();

		//debugLog("Database: Entry #%i: size = %u, artist = %s, songtitle = %s, creator = %s, diff = %s, audiofilename = %s, md5hash = %s, osufilename = %s\n", i, size, artistName.toUtf8(), songTitle.toUtf8(), creatorName.toUtf8(), difficultyName.toUtf8(), audioFileName.toUtf8(), md5hash.toUtf8(), osuFileName.toUtf8());
		//debugLog("rankedStatus = %i, numCircles = %i, numSliders = %i, numSpinners = %i, lastModificationTime = %ld\n", (int)rankedStatus, numCircles, numSliders, numSpinners, lastModificationTime);
		//debugLog("AR = %f, CS = %f, HP = %f, OD = %f, sliderMultiplier = %f\n", AR, CS, HP, OD, sliderMultiplier);

		unsigned int numOsuStandardStarRatings = db->readInt();
		//debugLog("%i star ratings for osu!standard\n", numOsuStandardStarRatings);
		float numOsuStandardStars = 0.0f;
		for (int s=0; s<numOsuStandardStarRatings; s++)
		{
			db->readByte(); // ObjType
			unsigned int mods = db->readInt();
			db->readByte(); // ObjType
			double starRating = db->readDouble();
			//debugLog("%f stars for %u\n", starRating, mods);

			if (mods == 0)
				numOsuStandardStars = starRating;
		}

		unsigned int numTaikoStarRatings = db->readInt();
		//debugLog("%i star ratings for taiko\n", numTaikoStarRatings);
		for (int s=0; s<numTaikoStarRatings; s++)
		{
			db->readByte(); // ObjType
			db->readInt();
			db->readByte(); // ObjType
			db->readDouble();
		}

		unsigned int numCtbStarRatings = db->readInt();
		//debugLog("%i star ratings for ctb\n", numCtbStarRatings);
		for (int s=0; s<numCtbStarRatings; s++)
		{
			db->readByte(); // ObjType
			db->readInt();
			db->readByte(); // ObjType
			db->readDouble();
		}

		unsigned int numManiaStarRatings = db->readInt();
		//debugLog("%i star ratings for mania\n", numManiaStarRatings);
		for (int s=0; s<numManiaStarRatings; s++)
		{
			db->readByte(); // ObjType
			db->readInt();
			db->readByte(); // ObjType
			db->readDouble();
		}

		unsigned int drainTime = db->readInt(); // seconds
		int duration = db->readInt(); // milliseconds
		duration = duration >= 0 ? duration : 0; // sanity clamp
		int previewTime = db->readInt();
		previewTime = previewTime >= 0 ? previewTime : 0; // sanity clamp

		//debugLog("drainTime = %i sec, duration = %i ms, previewTime = %i ms\n", drainTime, duration, previewTime);

		unsigned int numTimingPoints = db->readInt();
		//debugLog("%i timingpoints\n", numTimingPoints);
		std::vector<OsuFile::TIMINGPOINT> timingPoints;
		for (int t=0; t<numTimingPoints; t++)
		{
			timingPoints.push_back(db->readTimingPoint());
		}

		unsigned int beatmapID = db->readInt();
		int beatmapSetID = db->readInt(); // fucking bullshit, this is NOT an unsigned integer as is described on the wiki, it can and is -1 sometimes
		unsigned int threadID = db->readInt();

		unsigned char osuStandardGrade = db->readByte();
		unsigned char taikoGrade = db->readByte();
		unsigned char ctbGrade = db->readByte();
		unsigned char maniaGrade = db->readByte();
		//debugLog("beatmapID = %i, beatmapSetID = %i, threadID = %i, osuStandardGrade = %i, taikoGrade = %i, ctbGrade = %i, maniaGrade = %i\n", beatmapID, beatmapSetID, threadID, osuStandardGrade, taikoGrade, ctbGrade, maniaGrade);

		short localOffset = db->readShort();
		float stackLeniency = db->readFloat();
		unsigned char mode = db->readByte();
		//debugLog("localOffset = %i, stackLeniency = %f, mode = %i\n", localOffset, stackLeniency, mode);

		UString songSource = db->readString();
		UString songTags = db->readString();
		//debugLog("songSource = %s, songTags = %s\n", songSource.toUtf8(), songTags.toUtf8());

		short onlineOffset = db->readShort();
		UString songTitleFont = db->readString();
		bool unplayed = db->readBool();
		long long lastTimePlayed = db->readLongLong();
		bool isOsz2 = db->readBool();
		UString path = db->readString().trim(); // somehow, some beatmaps may have spaces at the start/end of their path, breaking the Windows API (e.g. https://osu.ppy.sh/s/215347), therefore the trim
		long long lastOnlineCheck = db->readLongLong();
		//debugLog("onlineOffset = %i, songTitleFont = %s, unplayed = %i, lastTimePlayed = %lu, isOsz2 = %i, path = %s, lastOnlineCheck = %lu\n", onlineOffset, songTitleFont.toUtf8(), (int)unplayed, lastTimePlayed, (int)isOsz2, path.toUtf8(), lastOnlineCheck);

		bool ignoreBeatmapSounds = db->readBool();
		bool ignoreBeatmapSkin = db->readBool();
		bool disableStoryboard = db->readBool();
		bool disableVideo = db->readBool();
		bool visualOverride = db->readBool();
		int lastEditTime = db->readInt();
		unsigned char maniaScrollSpeed = db->readByte();
		//debugLog("ignoreBeatmapSounds = %i, ignoreBeatmapSkin = %i, disableStoryboard = %i, disableVideo = %i, visualOverride = %i, maniaScrollSpeed = %i\n", (int)ignoreBeatmapSounds, (int)ignoreBeatmapSkin, (int)disableStoryboard, (int)disableVideo, (int)visualOverride, maniaScrollSpeed);

		// build beatmap & diffs from all the data
		UString beatmapPath = songFolder;
		beatmapPath.append(path);
		beatmapPath.append("/");
		UString fullFilePath = beatmapPath;
		fullFilePath.append(osuFileName);

		// fill diff with data
		if (mode == 0) // only use osu!standard diffs
		{
			OsuBeatmapDifficulty *diff = new OsuBeatmapDifficulty(m_osu, fullFilePath, beatmapPath);

			diff->title = songTitle;
			diff->audioFileName = audioFileName;
			diff->lengthMS = duration;

			diff->stackLeniency = stackLeniency;

			diff->artist = artistName;
			diff->creator = creatorName;
			diff->name = difficultyName;
			diff->source = songSource;
			diff->tags = songTags;
			diff->md5hash = md5hash;
			diff->beatmapId = beatmapID;

			diff->AR = AR;
			diff->CS = CS;
			diff->HP = HP;
			diff->OD = OD;
			diff->sliderMultiplier = sliderMultiplier;

			diff->backgroundImageName = "";

			diff->previewTime = previewTime;
			diff->lastModificationTime = lastModificationTime;

			diff->fullSoundFilePath = beatmapPath;
			diff->fullSoundFilePath.append(diff->audioFileName);
			diff->localoffset = localOffset;
			diff->onlineOffset = (long)onlineOffset;
			diff->numObjects = numCircles + numSliders + numSpinners;
			diff->starsNoMod = numOsuStandardStars;
			diff->ID = beatmapID;
			diff->setID = beatmapSetID;

			// calculate bpm range
			float minBeatLength = 0;
			float maxBeatLength = std::numeric_limits<float>::max();
			for (int t=0; t<timingPoints.size(); t++)
			{
				if (timingPoints[t].msPerBeat >= 0)
				{
					if (timingPoints[t].msPerBeat > minBeatLength)
						minBeatLength = timingPoints[t].msPerBeat;
					if (timingPoints[t].msPerBeat < maxBeatLength)
						maxBeatLength = timingPoints[t].msPerBeat;
				}
			}

			// convert from msPerBeat to BPM
			const float msPerMinute = 1 * 60 * 1000;
			if (minBeatLength != 0)
				minBeatLength = msPerMinute / minBeatLength;
			if (maxBeatLength != 0)
				maxBeatLength = msPerMinute / maxBeatLength;

			diff->minBPM = (int)std::round(minBeatLength);
			diff->maxBPM = (int)std::round(maxBeatLength);

			// build temp partial timingpoints, only used for menu animations
			for (int t=0; t<timingPoints.size(); t++)
			{
				OsuBeatmapDifficulty::TIMINGPOINT tp;
				tp.offset = timingPoints[t].offset;
				tp.msPerBeat = timingPoints[t].msPerBeat;
				diff->timingpoints.push_back(tp);
			}

			// now, search if the current set (to which this diff would belong) already exists and add it there, or if it doesn't exist then create the set
			bool beatmapSetExists = false;
			for (int s=0; s<beatmapSets.size(); s++)
			{
				if (beatmapSets[s].setID == beatmapSetID)
				{
					beatmapSetExists = true;
					beatmapSets[s].diffs.push_back(diff);
					break;
				}
			}
			if (!beatmapSetExists)
			{
				BeatmapSet s;
				s.setID = beatmapSetID;
				s.path = beatmapPath;
				s.diffs.push_back(diff);
				beatmapSets.push_back(s);
			}
		}
	}

	// we now have a collection of BeatmapSets (where one set is equal to one beatmap and all of its diffs), build the actual OsuBeatmap objects
	// first, build all beatmaps which have a valid setID (trusting the values from the osu database)
	for (int i=0; i<beatmapSets.size(); i++)
	{
		if (m_bKYS.load()) break; // cancellation point

		if (beatmapSets[i].diffs.size() > 0) // sanity check
		{
			if (beatmapSets[i].setID > 0)
			{
				OsuBeatmap *bm = new OsuBeatmapStandard(m_osu);
				bm->setDifficulties(beatmapSets[i].diffs);
				m_beatmaps.push_back(bm);
			}
		}
	}
	// second, handle all diffs which have an invalid setID, and group them exclusively by artist and title (diffs with the same artist and title will end up in the same beatmap object)
	// this goes through every individual diff in a "set" (not really a set because its ID is either 0 or -1) instead of trusting the ID values from the osu database
	for (int i=0; i<beatmapSets.size(); i++)
	{
		if (m_bKYS.load()) break; // cancellation point

		if (beatmapSets[i].diffs.size() > 0) // sanity check
		{
			if (beatmapSets[i].setID < 1)
			{
				for (int b=0; b<beatmapSets[i].diffs.size(); b++)
				{
					if (m_bKYS.load()) break; // cancellation point

					OsuBeatmapDifficulty *diff = beatmapSets[i].diffs[b];

					// try finding an already existing beatmap with matching artist and title
					bool existsAlready = false;
					for (int e=0; e<m_beatmaps.size(); e++)
					{
						if (m_bKYS.load()) break; // cancellation point

						if (m_beatmaps[e]->getTitle() == diff->title && m_beatmaps[e]->getArtist() == diff->artist)
						{
							existsAlready = true;

							// we have found a matching beatmap, add ourself to its diffs
							m_beatmaps[e]->getDifficultiesPointer()->push_back(diff);

							break;
						}
					}

					// if we couldn't find any beatmap with our title and artist, create a new one
					if (!existsAlready)
					{
						OsuBeatmap *bm = new OsuBeatmapStandard(m_osu);
						std::vector<OsuBeatmapDifficulty*> diffs;
						diffs.push_back(beatmapSets[i].diffs[b]);
						bm->setDifficulties(diffs);
						m_beatmaps.push_back(bm);
					}
				}
			}
		}
	}

	m_importTimer->update();
	debugLog("Refresh finished, added %i beatmaps in %f seconds.\n", m_beatmaps.size(), m_importTimer->getElapsedTime());

	// signal that we are almost done
	m_fLoadingProgress = 0.75f;

	// load collection.db
	UString collectionFilePath = osu_folder.getString();
	collectionFilePath.append("collection.db");
	OsuFile collectionFile(collectionFilePath);
	if (collectionFile.isReady())
	{
		struct RawCollection
		{
			UString name;
			std::vector<UString> hashes;
		};

		int version = collectionFile.readInt();
		int numCollections = collectionFile.readInt();

		debugLog("Collection: version = %i, numCollections = %i\n", version, numCollections);
		for (int i=0; i<numCollections; i++)
		{
			if (m_bKYS.load()) break; // cancellation point

			m_fLoadingProgress = 0.75f + 0.24f*((float)(i+1)/(float)numCollections);

			UString name = collectionFile.readString();
			int numBeatmaps = collectionFile.readInt();

			RawCollection rc;
			rc.name = name;

			if (Osu::debug->getBool())
				debugLog("Raw Collection #%i: name = %s, numBeatmaps = %i\n", i, name.toUtf8(), numBeatmaps);

			for (int b=0; b<numBeatmaps; b++)
			{
				if (m_bKYS.load()) break; // cancellation point

				UString md5hash = collectionFile.readString();
				rc.hashes.push_back(md5hash);
			}

			if (rc.hashes.size() > 0)
			{
				// collect OsuBeatmaps corresponding to this collection
				Collection c;
				c.name = rc.name;

				// go through every hash of the collection
				std::vector<OsuBeatmapDifficulty*> matchingDiffs;
				for (int h=0; h<rc.hashes.size(); h++)
				{
					if (m_bKYS.load()) break; // cancellation point

					// for every hash, go through every beatmap
					for (int b=0; b<m_beatmaps.size(); b++)
					{
						if (m_bKYS.load()) break; // cancellation point

						// for every beatmap, go through every diff and check if a diff hash matches, store those matching diffs
						std::vector<OsuBeatmapDifficulty*> diffs = m_beatmaps[b]->getDifficulties();
						for (int d=0; d<diffs.size(); d++)
						{
							if (m_bKYS.load()) break; // cancellation point

							if (diffs[d]->md5hash == rc.hashes[h])
								matchingDiffs.push_back(diffs[d]);
						}
					}
				}

				// we now have an array of all OsuBeatmapDifficulty objects within this collection

				// go through every found OsuBeatmapDifficulty
				if (matchingDiffs.size() > 0)
				{
					for (int md=0; md<matchingDiffs.size(); md++)
					{
						if (m_bKYS.load()) break; // cancellation point

						OsuBeatmapDifficulty *diff = matchingDiffs[md];

						// find the OsuBeatmap object corresponding to this diff
						OsuBeatmap *beatmap = NULL;
						for (int b=0; b<m_beatmaps.size(); b++)
						{
							if (m_bKYS.load()) break; // cancellation point

							std::vector<OsuBeatmapDifficulty*> diffs = m_beatmaps[b]->getDifficulties();
							for (int d=0; d<diffs.size(); d++)
							{
								if (m_bKYS.load()) break; // cancellation point

								if (diffs[d] == diff)
								{
									beatmap = m_beatmaps[b];
									break;
								}
							}
						}

						// we now have one matching OsuBeatmap and OsuBeatmapDifficulty, add either of them if they don't exist yet
						bool beatmapIsAlreadyInCollection = false;
						for (int m=0; m<c.beatmaps.size(); m++)
						{
							if (m_bKYS.load()) break; // cancellation point

							if (c.beatmaps[m].first == beatmap)
							{
								beatmapIsAlreadyInCollection = true;

								// the beatmap already exists, check if we have to add the current diff
								bool diffIsAlreadyInCollection = false;
								for (int d=0; d<c.beatmaps[m].second.size(); d++)
								{
									if (m_bKYS.load()) break; // cancellation point

									if (c.beatmaps[m].second[d] == diff)
									{
										diffIsAlreadyInCollection = true;
										break;
									}
								}

								// add diff
								if (!diffIsAlreadyInCollection && diff != NULL)
									c.beatmaps[m].second.push_back(diff);

								break;
							}
						}

						// add beatmap
						if (!beatmapIsAlreadyInCollection && beatmap != NULL && diff != NULL)
						{
							std::vector<OsuBeatmapDifficulty*> diffs;
							diffs.push_back(diff);
							c.beatmaps.push_back(std::pair<OsuBeatmap*, std::vector<OsuBeatmapDifficulty*>>(beatmap, diffs));
						}
					}
				}

				// add the collection
				if (c.beatmaps.size() > 0) // sanity check
					m_collections.push_back(c);
			}
		}

		if (Osu::debug->getBool())
		{
			for (int i=0; i<m_collections.size(); i++)
			{
				debugLog("Collection #%i: name = %s, numBeatmaps = %i\n", i, m_collections[i].name.toUtf8(), m_collections[i].beatmaps.size());
			}
		}
	}
	else
		debugLog("OsuBeatmapDatabase::loadDB() : Couldn't load collection.db");

	// signal that we are done
	m_fLoadingProgress = 1.0f;
}

OsuBeatmap *OsuDatabase::loadRawBeatmap(UString beatmapPath)
{
	if (Osu::debug->getBool())
		debugLog("OsuBeatmapDatabase::loadRawBeatmap() : %s\n", beatmapPath.toUtf8());

	OsuBeatmap *result = NULL;

	// try loading all diffs
	std::vector<OsuBeatmapDifficulty*> diffs;
	std::vector<UString> beatmapFiles = env->getFilesInFolder(beatmapPath);
	for (int i=0; i<beatmapFiles.size(); i++)
	{
		UString ext = env->getFileExtensionFromFilePath(beatmapFiles[i]);

		UString fullFilePath = beatmapPath;
		fullFilePath.append(beatmapFiles[i]);

		// load diffs
		if (ext == "osu")
		{
			OsuBeatmapDifficulty *diff = new OsuBeatmapDifficulty(m_osu, fullFilePath, beatmapPath);

			// try to load it. if successful, save it, else cleanup and continue to the next osu file
			if (!diff->loadMetadataRaw())
			{
				if (Osu::debug->getBool())
				{
					debugLog("OsuBeatmapDatabase::loadRawBeatmap() : Couldn't loadMetadata(), deleting object.\n");
					if (diff->mode == 0)
						engine->showMessageWarning("OsuBeatmapDatabase::loadRawBeatmap()", "Couldn't loadMetadata()\n");
				}
				SAFE_DELETE(diff);
				continue;
			}

			diffs.push_back(diff);
		}
	}

	// if we found any valid diffs, create beatmap
	if (diffs.size() > 0)
	{
		result = new OsuBeatmapStandard(m_osu);
		result->setDifficulties(diffs);
	}

	return result;
}
