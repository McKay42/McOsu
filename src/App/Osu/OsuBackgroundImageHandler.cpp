//================ Copyright (c) 2020, PG, All rights reserved. =================//
//
// Purpose:		handles thumbnail/background image loading async, cache
//
// $NoKeywords: $osubgldr
//===============================================================================//

#include "OsuBackgroundImageHandler.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "ConVar.h"

#include "OsuDatabaseBeatmap.h"

ConVar osu_load_beatmap_background_images("osu_load_beatmap_background_images", true);

ConVar osu_background_image_cache_size("osu_background_image_cache_size", 32, "how many images can stay loaded in parallel");
ConVar osu_background_image_loading_delay("osu_background_image_loading_delay", 0.1f, "how many seconds to wait until loading background images for visible beatmaps starts");
ConVar osu_background_image_eviction_delay("osu_background_image_eviction_delay", 0.05f, "how many seconds to keep stale background images in the cache before deleting them");

OsuBackgroundImageHandler::OsuBackgroundImageHandler()
{
	m_bFrozen = false;
}

OsuBackgroundImageHandler::~OsuBackgroundImageHandler()
{
	for (size_t i=0; i<m_cache.size(); i++)
	{
		engine->getResourceManager()->destroyResource(m_cache[i].backgroundImagePathLoader);
		engine->getResourceManager()->destroyResource(m_cache[i].image);
	}
	m_cache.clear();
}

void OsuBackgroundImageHandler::update(bool allowEviction)
{
	for (size_t i=0; i<m_cache.size(); i++)
	{
		ENTRY &entry = m_cache[i];

		// NOTE: avoid load/unload jitter if framerate is below eviction delay
		const bool wasUsedLastFrame = entry.wasUsedLastFrame;
		entry.wasUsedLastFrame = false;

		// check and handle evictions
		if (!wasUsedLastFrame && engine->getTime() >= entry.evictionTime)
		{
			if (allowEviction)
			{
				if (!m_bFrozen && !engine->isMinimized())
				{
					if (entry.backgroundImagePathLoader != NULL)
						entry.backgroundImagePathLoader->interruptLoad();
					if (entry.image != NULL)
						entry.image->interruptLoad();

					engine->getResourceManager()->destroyResource(entry.backgroundImagePathLoader);
					engine->getResourceManager()->destroyResource(entry.image);

					m_cache.erase(m_cache.begin() + i);
					i--;
					continue;
				}
			}
			else
				entry.evictionTime = engine->getTime() + osu_background_image_eviction_delay.getFloat();
		}
		else if (wasUsedLastFrame)
		{
			// check and handle scheduled loads
			if (entry.isLoadScheduled)
			{
				if (engine->getTime() >= entry.loadingTime)
				{
					entry.isLoadScheduled = false;

					if (entry.backgroundImageFileName.length() < 2)
					{
						// if the backgroundImageFileName is not loaded, then we have to create a full OsuDatabaseBeatmapBackgroundImagePathLoader
						entry.image = NULL;
						handleLoadPathForEntry(entry);
					}
					else
					{
						// if backgroundImageFileName is already loaded/valid, then we can directly load the image
						entry.backgroundImagePathLoader = NULL;
						handleLoadImageForEntry(entry);
					}
				}
			}
			else
			{
				// no load scheduled (potential load-in-progress if it was necessary), handle backgroundImagePathLoader loading finish
				if (entry.image == NULL && entry.backgroundImagePathLoader != NULL && entry.backgroundImagePathLoader->isReady())
				{
					if (entry.backgroundImagePathLoader->getLoadedBackgroundImageFileName().length() > 1)
					{
						entry.backgroundImageFileName = entry.backgroundImagePathLoader->getLoadedBackgroundImageFileName();
						handleLoadImageForEntry(entry);
					}

					engine->getResourceManager()->destroyResource(entry.backgroundImagePathLoader);
					entry.backgroundImagePathLoader = NULL;
				}
			}
		}
	}

	// reset flags
	m_bFrozen = false;

	// DEBUG:
	//debugLog("m_cache.size() = %i\n", (int)m_cache.size());
}

void OsuBackgroundImageHandler::handleLoadPathForEntry(ENTRY &entry)
{
	entry.backgroundImagePathLoader = new OsuDatabaseBeatmapBackgroundImagePathLoader(entry.osuFilePath);

	// start path load
	engine->getResourceManager()->requestNextLoadAsync();
	engine->getResourceManager()->loadResource(entry.backgroundImagePathLoader);
}

void OsuBackgroundImageHandler::handleLoadImageForEntry(ENTRY &entry)
{
	UString fullBackgroundImageFilePath = entry.folder;
	fullBackgroundImageFilePath.append(entry.backgroundImageFileName);

	// start image load
	engine->getResourceManager()->requestNextLoadAsync();
	engine->getResourceManager()->requestNextLoadUnmanaged();
	entry.image = engine->getResourceManager()->loadImageAbsUnnamed(fullBackgroundImageFilePath);
}

Image *OsuBackgroundImageHandler::getLoadBackgroundImage(const OsuDatabaseBeatmap *beatmap)
{
	if (beatmap == NULL || !osu_load_beatmap_background_images.getBool()) return NULL;

	// NOTE: no references to beatmap are kept anywhere (database can safely be deleted/reloaded without having to notify the OsuBackgroundImageHandler)

	const float newLoadingTime = engine->getTime() + osu_background_image_loading_delay.getFloat();
	const float newEvictionTime = engine->getTime() + osu_background_image_eviction_delay.getFloat();

	// 1) if the path or image is already loaded, return image ref immediately (which may still be NULL) and keep track of when it was last requested
	for (size_t i=0; i<m_cache.size(); i++)
	{
		ENTRY &entry = m_cache[i];

		if (entry.osuFilePath == beatmap->getFilePath())
		{
			entry.wasUsedLastFrame = true;
			entry.evictionTime = newEvictionTime;

			// HACKHACK: to improve future loading speed, if we have already loaded the backgroundImageFileName, force update the database backgroundImageFileName and fullBackgroundImageFilePath
			// this is similar to how it worked before the rework, but 100% safe(r) since we are not async
			if (m_cache[i].image != NULL && m_cache[i].backgroundImageFileName.length() > 1 && beatmap->getBackgroundImageFileName().length() < 2)
			{
				const_cast<OsuDatabaseBeatmap*>(beatmap)->m_sBackgroundImageFileName = m_cache[i].backgroundImageFileName;
				const_cast<OsuDatabaseBeatmap*>(beatmap)->m_sFullBackgroundImageFilePath = beatmap->getFolder();
				const_cast<OsuDatabaseBeatmap*>(beatmap)->m_sFullBackgroundImageFilePath.append(m_cache[i].backgroundImageFileName);
			}

			return entry.image;
		}
	}

	// 2) not found in cache, so create a new entry which will get handled in the next update
	{
		// try evicting stale not-yet-loaded-nor-started-loading entries on overflow
		const int maxCacheEntries = osu_background_image_cache_size.getInt();
		{
			if (m_cache.size() >= maxCacheEntries)
			{
				for (size_t i=0; i<m_cache.size(); i++)
				{
					if (m_cache[i].isLoadScheduled && !m_cache[i].wasUsedLastFrame)
					{
						m_cache.erase(m_cache.begin() + i);
						i--;
						continue;
					}
				}
			}
		}

		// create entry
		ENTRY entry;
		{
			entry.isLoadScheduled = true;
			entry.wasUsedLastFrame = true;
			entry.loadingTime = newLoadingTime;
			entry.evictionTime = newEvictionTime;

			entry.osuFilePath = beatmap->getFilePath();
			entry.folder = beatmap->getFolder();
			entry.backgroundImageFileName = beatmap->getBackgroundImageFileName();

			entry.backgroundImagePathLoader = NULL;
			entry.image = NULL;
		}
		if (m_cache.size() < maxCacheEntries)
			m_cache.push_back(entry);
	}

	return NULL;
}
