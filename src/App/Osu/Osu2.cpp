//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		unfinished spectator client
//
// $NoKeywords: $
//===============================================================================//

#include "Osu2.h"

#include "Engine.h"
#include "ConVar.h"
#include "Mouse.h"
#include "ResourceManager.h"

#include "OsuBeatmap.h"
#include "OsuMultiplayer.h"
#include "OsuSongBrowser2.h"
#include "OsuDatabase.h"

// TODO: make offset and resolution calculation entirely in here, and then set it on all instances
// TODO: generalize mouse offset hack for draw/update into some function
// TODO: forward input to instance which currently contains the mouse cursor, instead of always 1 (rect.contains())
// TODO: playing the same map again breaks in all slaves except master, containing no hitobjects, wtf is happening there (probably due to missing deselects)?
// TODO: all instances need custom m_osu->setScore() object which is managed by Osu2, the main spectator client will only be in server mode (and not connect to itself as a client)
// TODO: sync cursor positions to music position, group cursor packets and send at cl_cmdrate
// TODO: sample cursor at fixed interval, force sample at every key delta. (except if everything is event based, then force would not be necessary)
// TODO: server currently doesn't keep track of game state (it only forwards e.g. score packets to all other clients)

Osu2::Osu2()
{
	const int numSlaves = 3;
	for (int i=2; i<2+numSlaves; i++)
	{
		Osu *slaveInstance = new Osu(this, i);
		m_slaves.push_back(slaveInstance);
	}

	// instantiate master last to steal all convar callbacks
	m_osu = new Osu(this, 1);

	// collect all instances
	m_instances.push_back(m_osu);
	for (int i=0; i<m_slaves.size(); i++)
	{
		m_instances.push_back(m_slaves[i]);
	}

	// hijack some convar callbacks to override behaviour globally
	convar->getConVarByName("osu_skin")->setCallback( fastdelegate::MakeDelegate(this, &Osu2::onSkinChange) );

	// force db load on all slaves
	m_bSlavesLoaded = false;
	for (int i=0; i<m_slaves.size(); i++)
	{
		m_slaves[i]->toggleSongBrowser();
	}

	// vars
	m_prevBeatmap = NULL;
	m_prevBeatmapDifficulty = NULL;

	m_bPrevPlayingState = false;
}

Osu2::~Osu2()
{
	for (int i=0; i<m_slaves.size(); i++)
	{
		delete m_slaves[i];
	}
	m_slaves.clear();

	SAFE_DELETE(m_osu);
}

void Osu2::draw(Graphics *g)
{
	for (int i=0; i<m_instances.size(); i++)
	{
		Osu *osu = m_instances[i];
		const Vector2 resolution = osu->getScreenSize();
		const int instanceID = osu->getInstanceID();

		Vector2 offset;
		if (instanceID > 0)
		{
			float emptySpaceX = engine->getGraphics()->getResolution().x - 2*resolution.x;
			float emptySpaceY = engine->getGraphics()->getResolution().y - 2*resolution.y;

			switch (instanceID)
			{
			case 1:
				offset.x = emptySpaceX/4;
				offset.y = emptySpaceY/4;
				break;
			case 2:
				offset.x = emptySpaceX/4;
				offset.y = emptySpaceY/4 + engine->getGraphics()->getResolution().y/2;
				break;
			case 3:
				offset.x = emptySpaceX/4 + engine->getGraphics()->getResolution().x/2;
				offset.y = emptySpaceY/4;
				break;
			case 4:
				offset.x = emptySpaceX/4 + engine->getGraphics()->getResolution().x/2;
				offset.y = emptySpaceY/4 + engine->getGraphics()->getResolution().y/2;
				break;
			}
		}

		Vector2 prevOffset;
		Vector2 prevPosWithoutOffset;
		//if (instanceID == 1)
		{
			prevOffset = engine->getMouse()->getOffset();
			prevPosWithoutOffset = engine->getMouse()->getPos() - prevOffset;
			engine->getMouse()->setOffset(-offset);
			engine->getMouse()->onPosChange(prevPosWithoutOffset);
		}

		m_instances[i]->draw(g);

		//if (instanceID == 1)
		{
			engine->getMouse()->setOffset(prevOffset);
			engine->getMouse()->onPosChange(prevPosWithoutOffset);
		}
	}
}

void Osu2::update()
{
	// main update
	for (int i=0; i<m_instances.size(); i++)
	{
		Osu *osu = m_instances[i];
		const Vector2 resolution = osu->getScreenSize();
		const int instanceID = osu->getInstanceID();

		Vector2 offset;
		if (instanceID > 0)
		{
			float emptySpaceX = engine->getGraphics()->getResolution().x - 2*resolution.x;
			float emptySpaceY = engine->getGraphics()->getResolution().y - 2*resolution.y;

			switch (instanceID)
			{
			case 1:
				offset.x = emptySpaceX/4;
				offset.y = emptySpaceY/4;
				break;
			case 2:
				offset.x = emptySpaceX/4;
				offset.y = emptySpaceY/4 + engine->getGraphics()->getResolution().y/2;
				break;
			case 3:
				offset.x = emptySpaceX/4 + engine->getGraphics()->getResolution().x/2;
				offset.y = emptySpaceY/4;
				break;
			case 4:
				offset.x = emptySpaceX/4 + engine->getGraphics()->getResolution().x/2;
				offset.y = emptySpaceY/4 + engine->getGraphics()->getResolution().y/2;
				break;
			}
		}

		Vector2 prevOffset;
		Vector2 prevPosWithoutOffset;
		//if (instanceID == 1)
		{
			prevOffset = engine->getMouse()->getOffset();
			prevPosWithoutOffset = engine->getMouse()->getPos() - prevOffset;
			engine->getMouse()->setOffset(-offset);
			engine->getMouse()->onPosChange(prevPosWithoutOffset);
		}

		m_instances[i]->update();

		//if (instanceID == 1)
		{
			engine->getMouse()->setOffset(prevOffset);
			engine->getMouse()->onPosChange(prevPosWithoutOffset);
		}
	}

	// beatmap select sync
	if (m_osu->getSelectedBeatmap() != m_prevBeatmap || (m_osu->getSelectedBeatmap() != NULL && m_osu->getSelectedBeatmap()->getSelectedDifficulty() != m_prevBeatmapDifficulty))
	{
		m_prevBeatmap = m_osu->getSelectedBeatmap();
		m_prevBeatmapDifficulty = m_osu->getSelectedBeatmap() != NULL ? m_osu->getSelectedBeatmap()->getSelectedDifficulty() : NULL;

		for (int i=0; i<m_slaves.size(); i++)
		{
			m_slaves[i]->getMultiplayer()->setBeatmap(m_prevBeatmap);
		}

		m_osu->getMultiplayer()->setBeatmap(m_prevBeatmap);
	}

	// db load sync
	if (!m_bSlavesLoaded)
	{
		bool finished = true;
		for (int i=0; i<m_slaves.size(); i++)
		{
			if (m_slaves[i]->getSongBrowser()->getDatabase()->getProgress() < 1.0f)
				finished = false;
		}
		m_bSlavesLoaded = finished;

		if (finished)
		{
			for (int i=0; i<m_slaves.size(); i++)
			{
				m_slaves[i]->toggleSongBrowser();
			}
		}
	}

	// beatmap start/stop sync
	if (m_osu->getSongBrowser()->hasSelectedAndIsPlaying() != m_bPrevPlayingState)
	{
		if (engine->getResourceManager()->getSound("OSU_BEATMAP_MUSIC") != NULL)
		{
			m_bPrevPlayingState = m_osu->getSongBrowser()->hasSelectedAndIsPlaying();

			for (int i=0; i<m_slaves.size(); i++)
			{
				if (m_bPrevPlayingState)
					m_slaves[i]->getSongBrowser()->onDifficultySelected(m_slaves[i]->getSongBrowser()->getSelectedBeatmap(), m_slaves[i]->getSongBrowser()->getSelectedBeatmap()->getSelectedDifficulty(), true, false);
				else
					m_slaves[i]->getSongBrowser()->getSelectedBeatmap()->stop(true);
			}
		}
	}
}

void Osu2::onKeyDown(KeyboardEvent &key)
{
	m_osu->onKeyDown(key);
}

void Osu2::onKeyUp(KeyboardEvent &key)
{
	m_osu->onKeyUp(key);
}

void Osu2::onChar(KeyboardEvent &charCode)
{
	m_osu->onChar(charCode);
}

void Osu2::onResolutionChanged(Vector2 newResolution)
{
	m_osu->onResolutionChanged(newResolution);

	for (int i=0; i<m_slaves.size(); i++)
	{
		m_slaves[i]->onResolutionChanged(newResolution);
	}
}

void Osu2::onFocusGained()
{
	m_osu->onFocusGained();
}

void Osu2::onFocusLost()
{
	m_osu->onFocusLost();
}

void Osu2::onMinimized()
{
	m_osu->onMinimized();
}

void Osu2::onRestored()
{
	m_osu->onRestored();
}

bool Osu2::onShutdown()
{
	return m_osu->onShutdown();
}

void Osu2::onSkinChange(UString oldValue, UString newValue)
{
	for (int i=0; i<m_slaves.size(); i++)
	{
		m_slaves[i]->setSkin(newValue);
	}

	m_osu->setSkin(newValue);
}
