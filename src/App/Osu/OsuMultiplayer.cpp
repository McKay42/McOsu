//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		network play
//
// $NoKeywords: $osump
//===============================================================================//

#include "OsuMultiplayer.h"

#include "Engine.h"
#include "ConVar.h"
#include "Console.h"
#include "Mouse.h"
#include "NetworkHandler.h"
#include "ResourceManager.h"
#include "Environment.h"
#include "File.h"

#include "Osu.h"
#include "OsuDatabase.h"
#include "OsuDatabaseBeatmap.h"

#include "OsuBeatmap.h"

#include "OsuMainMenu.h"
#include "OsuSongBrowser2.h"
#include "OsuNotificationOverlay.h"

#include "OsuUISongBrowserInfoLabel.h"

ConVar osu_mp_freemod("osu_mp_freemod", false);
ConVar osu_mp_freemod_all("osu_mp_freemod_all", true, "allow everything, or only standard osu mods");
ConVar osu_mp_win_condition_accuracy("osu_mp_win_condition_accuracy", false);
ConVar osu_mp_allow_client_beatmap_select("osu_mp_sv_allow_client_beatmap_select", true);
ConVar osu_mp_broadcastcommand("osu_mp_broadcastcommand");
ConVar osu_mp_clientcastcommand("osu_mp_clientcastcommand");
ConVar osu_mp_broadcastforceclientbeatmapdownload("osu_mp_broadcastforceclientbeatmapdownload");
ConVar osu_mp_select_beatmap("osu_mp_select_beatmap");
ConVar osu_mp_request_beatmap_download("osu_mp_request_beatmap_download");

unsigned long long OsuMultiplayer::sortHackCounter = 0;
ConVar *OsuMultiplayer::m_cl_cmdrate = NULL;

OsuMultiplayer::OsuMultiplayer(Osu *osu)
{
	m_osu = osu;

	if (m_cl_cmdrate == NULL)
		m_cl_cmdrate = convar->getConVarByName("cl_cmdrate", false);

	// engine callbacks
	engine->getNetworkHandler()->setOnClientReceiveServerPacketListener( fastdelegate::MakeDelegate(this, &OsuMultiplayer::onClientReceive) );
	engine->getNetworkHandler()->setOnServerReceiveClientPacketListener( fastdelegate::MakeDelegate(this, &OsuMultiplayer::onServerReceive) );

	engine->getNetworkHandler()->setOnClientConnectedToServerListener( fastdelegate::MakeDelegate(this, &OsuMultiplayer::onClientConnectedToServer) );
	engine->getNetworkHandler()->setOnClientDisconnectedFromServerListener( fastdelegate::MakeDelegate(this, &OsuMultiplayer::onClientDisconnectedFromServer) );

	engine->getNetworkHandler()->setOnServerClientChangeListener( fastdelegate::MakeDelegate(this, &OsuMultiplayer::onServerClientChange) );
	engine->getNetworkHandler()->setOnLocalServerStartedListener( fastdelegate::MakeDelegate(this, &OsuMultiplayer::onLocalServerStarted) );
	engine->getNetworkHandler()->setOnLocalServerStoppedListener( fastdelegate::MakeDelegate(this, &OsuMultiplayer::onLocalServerStopped) );

	// convar callbacks
	osu_mp_broadcastcommand.setCallback( fastdelegate::MakeDelegate(this, &OsuMultiplayer::onBroadcastCommand) );
	osu_mp_clientcastcommand.setCallback( fastdelegate::MakeDelegate(this, &OsuMultiplayer::onClientcastCommand) );

	osu_mp_broadcastforceclientbeatmapdownload.setCallback( fastdelegate::MakeDelegate(this, &OsuMultiplayer::onMPForceClientBeatmapDownload) );
	osu_mp_select_beatmap.setCallback( fastdelegate::MakeDelegate(this, &OsuMultiplayer::onMPSelectBeatmap) );
	osu_mp_request_beatmap_download.setCallback( fastdelegate::MakeDelegate(this, &OsuMultiplayer::onMPRequestBeatmapDownload) );

	m_fNextPlayerCmd = 0.0f;

	m_bMPSelectBeatmapScheduled = false;

	m_iLastClientBeatmapSelectID = 0;

	/*
	PLAYER ply;
	ply.name = "Player1";
	ply.missingBeatmap = false;
	ply.score = 1000000;
	ply.accuracy = 0.9123456f;
	ply.combo = 420;
	ply.dead = false;
	m_clientPlayers.push_back(ply);
	ply.name = "Player2";
	ply.missingBeatmap = true;
	ply.combo = 1;
	m_clientPlayers.push_back(ply);
	ply.name = "Player420";
	ply.missingBeatmap = false;
	ply.dead = true;
	ply.combo = 23;
	m_clientPlayers.push_back(ply);
	*/
}

OsuMultiplayer::~OsuMultiplayer()
{
}

void OsuMultiplayer::update()
{
	if (m_bMPSelectBeatmapScheduled)
	{
		// wait for songbrowser db load
		if (m_osu->getSongBrowser()->getDatabase()->isFinished())
		{
			setBeatmap(m_sMPSelectBeatmapScheduledMD5Hash);
			m_bMPSelectBeatmapScheduled = false; // NOTE: resetting flag below to avoid endless loops
		}
	}

	if (!isInMultiplayer()) return;

	// handle running uploads
	const size_t uploadChunkSize = 1024 * 16; // 16 KB
	for (size_t i=0; i<m_uploads.size(); i++)
	{
		// send one chunk per filetype per upload tick
		for (uint8_t fileType=1; fileType<4; fileType++)
		{
			// collect relevant vars
			char *buffer = NULL;
			size_t fileSize = 0;
			size_t alreadyUploadedDataBytes = 0;
			bool finished = false;
			switch (fileType)
			{
			case 1:
				buffer = (char*)m_uploads[i].osuFile->readFile();
				fileSize = m_uploads[i].osuFile->getFileSize();
				alreadyUploadedDataBytes = m_uploads[i].numUploadedOsuFileBytes;
				if (m_uploads[i].numUploadedOsuFileBytes >= fileSize)
					finished = true;
				break;
			case 2:
				buffer = (char*)m_uploads[i].musicFile->readFile();
				fileSize = m_uploads[i].musicFile->getFileSize();
				alreadyUploadedDataBytes = m_uploads[i].numUploadedMusicFileBytes;
				if (m_uploads[i].numUploadedMusicFileBytes >= fileSize)
					finished = true;
				break;
			case 3:
				buffer = (char*)m_uploads[i].backgroundFile->readFile();
				fileSize = m_uploads[i].backgroundFile->getFileSize();
				alreadyUploadedDataBytes = m_uploads[i].numUploadedBackgroundFileBytes;
				if (m_uploads[i].numUploadedBackgroundFileBytes >= fileSize)
					finished = true;
				break;
			}

			BEATMAP_DOWNLOAD_CHUNK_PACKET pp;
			pp.serial = m_uploads[i].serial;
			pp.fileType = fileType;
			pp.numDataBytes = (uint32_t)std::min(uploadChunkSize, fileSize - alreadyUploadedDataBytes);
			size_t size = sizeof(BEATMAP_DOWNLOAD_CHUNK_PACKET);

			if (!finished)
			{
				//debugLog("sending chunk fileType = %i, fileSize = %i, alreadyUploaded = %i, numDataBytes = %i\n", (int)fileType, (int)fileSize, (int)alreadyUploadedDataBytes, (int)pp.numDataBytes);

				if (buffer == NULL)
				{
					//debugLog("buffer == NULL!\n");
					continue;
				}

				PACKET_TYPE wrap = BEATMAP_DOWNLOAD_CHUNK_TYPE;
				const int wrapperSize = sizeof(PACKET_TYPE);
				char wrappedPacket[(wrapperSize + size + pp.numDataBytes)];
				memcpy(&wrappedPacket, &wrap, wrapperSize);
				memcpy((void*)(((char*)&wrappedPacket) + wrapperSize), &pp, size);
				memcpy((void*)(((char*)&wrappedPacket) + wrapperSize + size), buffer + alreadyUploadedDataBytes, pp.numDataBytes);

				if (m_uploads[i].id == 0)
					engine->getNetworkHandler()->servercast(wrappedPacket, pp.numDataBytes + size + wrapperSize, true);
				else
					engine->getNetworkHandler()->clientcast(wrappedPacket, pp.numDataBytes + size + wrapperSize, m_uploads[i].id, true);

				// update counter (2)
				switch (fileType)
				{
				case 1:
					m_uploads[i].numUploadedOsuFileBytes += pp.numDataBytes;
					break;
				case 2:
					m_uploads[i].numUploadedMusicFileBytes += pp.numDataBytes;
					break;
				case 3:
					m_uploads[i].numUploadedBackgroundFileBytes += pp.numDataBytes;
					break;
				}
			}
		}

		// check if an upload is done, and if so remove it from the list
		const size_t totalUploadedSize = m_uploads[i].numUploadedOsuFileBytes + m_uploads[i].numUploadedMusicFileBytes + m_uploads[i].numUploadedBackgroundFileBytes;
		const size_t totalFilesSize = m_uploads[i].osuFile->getFileSize() + m_uploads[i].musicFile->getFileSize() + m_uploads[i].backgroundFile->getFileSize();
		if (totalUploadedSize >= totalFilesSize)
		{
			delete m_uploads[i].osuFile;
			delete m_uploads[i].musicFile;
			delete m_uploads[i].backgroundFile;

			m_uploads.erase(m_uploads.begin() + i);
			i--;

			debugLog("remaining uploads: %i\n", (int)m_uploads.size());

			continue;
		}
	}

	// handle running downloads
	for (size_t i=0; i<m_downloads.size(); i++)
	{
		const size_t numTotalDownloadedBytes = m_downloads[i].numDownloadedOsuFileBytes + m_downloads[i].numDownloadedMusicFileBytes + m_downloads[i].numDownloadedBackgroundFileBytes;
		if (numTotalDownloadedBytes >= m_downloads[i].totalDownloadBytes)
		{
			onBeatmapDownloadFinished(m_downloads[i]);
			m_downloads.erase(m_downloads.begin() + i);
			i--;

			debugLog("remaining downloads: %i\n", (int)m_downloads.size());

			continue;
		}
	}

	/*
	if (!isServer())
	{
		// clientside update
		// TODO: only generate input cmd in fixed time steps, not for every frame

		PLAYER_INPUT_PACKET input;
		input.time = m_osu->getSongBrowser()->hasSelectedAndIsPlaying() ? m_osu->getSelectedBeatmap()->getCurMusicPosWithOffsets() : 0;
		// TODO: convert to resolution independent (osu?) coordinates
		input.cursorPosX = (short)engine->getMouse()->getPos().x;
		input.cursorPosY = (short)engine->getMouse()->getPos().y;
		input.keys = 0; // TODO:
		m_playerInputBuffer.push_back(input);

		if (engine->getTime() >= m_fNextPlayerCmd)
		{
			m_fNextPlayerCmd = engine->getTime() + 1.0f/m_cl_cmdrate->getFloat();
			onClientCmd();
			m_playerInputBuffer.clear();
		}
	}
	else
	{
		// serverside update

		// TODO: define reasonable max inputBuffer length (this also indirectly defines the max waiting delay for other players)
		// TODO: always keep most recent full packet to be able to jump around
		for (int i=0; i<m_serverPlayers.size(); i++)
		{
			debugLog("inputBuffer size = %i\n", m_serverPlayers[i].inputBuffer.size());
			if (m_serverPlayers[i].inputBuffer.size() > 2048)
			{
				const int overflow = m_serverPlayers[i].inputBuffer.size() - 2048;
				m_serverPlayers[i].inputBuffer.erase(m_serverPlayers[i].inputBuffer.begin(), m_serverPlayers[i].inputBuffer.begin() + overflow);
			}

			// TEMP:
			if (m_serverPlayers[i].inputBuffer.size() > 0)
				m_serverPlayers[i].input.cursorPos = m_serverPlayers[i].inputBuffer[m_serverPlayers[i].inputBuffer.size()-1].cursorPos;
		}
	}
	*/
}

bool OsuMultiplayer::onClientReceive(uint32_t id, void *data, uint32_t size)
{
	return onClientReceiveInt(id, data, size);
}

bool OsuMultiplayer::onClientReceiveInt(uint32_t id, void *data, uint32_t size, bool forceAcceptOnServer)
{
	const int wrapperSize = sizeof(PACKET_TYPE);
	if (size <= wrapperSize) return false;

	// unwrap the packet
	char *unwrappedPacket = (char*)data;
	unwrappedPacket += wrapperSize;
	const uint32_t unwrappedSize = (size - wrapperSize);

	const PACKET_TYPE type = (*(PACKET_TYPE*)data);
	switch (type)
	{
	case PLAYER_CHANGE_TYPE:
		if (unwrappedSize >= sizeof(PLAYER_CHANGE_PACKET))
		{
			// execute
			PLAYER_CHANGE_PACKET *pp = (struct PLAYER_CHANGE_PACKET*)unwrappedPacket;
			bool exists = false;
			for (int i=0; i<m_clientPlayers.size(); i++)
			{
				if (m_clientPlayers[i].id == pp->id)
				{
					exists = true;
					if (!pp->connected) // player disconnect
					{
						m_clientPlayers.erase(m_clientPlayers.begin() + i);

						debugLog("Player %i left the game.\n", pp->id);
					}
					else // player update
					{
					}
					break;
				}
			}

			if (!exists) // player connect
			{
				PLAYER ply;
				ply.id = pp->id;
				ply.name = UString(pp->name).substr(0, pp->size);

				ply.missingBeatmap = false;
				ply.downloadingBeatmap = false;
				ply.waiting = true;

				ply.combo = 0;
				ply.accuracy = 0.0f;
				ply.score = 0;
				ply.dead = false;

				ply.input.mouse1down = false;
				ply.input.mouse2down = false;
				ply.input.key1down = false;
				ply.input.key2down = false;

				ply.sortHack = sortHackCounter++;

				m_clientPlayers.push_back(ply);

				debugLog("Player %i joined the game.\n", pp->id);
			}
		}
		return true;

	case PLAYER_STATE_TYPE:
		if (unwrappedSize >= sizeof(PLAYER_STATE_PACKET))
		{
			// execute
			PLAYER_STATE_PACKET *pp = (struct PLAYER_STATE_PACKET*)unwrappedPacket;
			for (int i=0; i<m_clientPlayers.size(); i++)
			{
				if (m_clientPlayers[i].id == pp->id)
				{
					// player state update
					m_clientPlayers[i].missingBeatmap = pp->missingBeatmap;
					m_clientPlayers[i].downloadingBeatmap = pp->downloadingBeatmap;
					m_clientPlayers[i].waiting = pp->waiting;
					break;
				}
			}
		}
		return true;

	case CONVAR_TYPE:
		if (unwrappedSize >= sizeof(CONVAR_PACKET))
		{
			if (!isServer() || forceAcceptOnServer)
			{
				// execute
				CONVAR_PACKET *pp = (struct CONVAR_PACKET*)unwrappedPacket;
				Console::processCommand(UString(pp->str).substr(0, pp->len));
			}
		}
		return true;

	case STATE_TYPE:
		if (unwrappedSize >= sizeof(GAME_STATE_PACKET))
		{
			if (!isServer() || forceAcceptOnServer)
			{
				// execute
				GAME_STATE_PACKET *pp = (struct GAME_STATE_PACKET*)unwrappedPacket;
				if (pp->state == SELECT)
				{
					bool found = false;
					const std::vector<OsuDatabaseBeatmap*> &beatmaps = m_osu->getSongBrowser()->getDatabase()->getDatabaseBeatmaps();
					for (int i=0; i<beatmaps.size(); i++)
					{
						OsuDatabaseBeatmap *beatmap = beatmaps[i];
						const std::vector<OsuDatabaseBeatmap*> &diffs = beatmap->getDifficulties();
						for (int d=0; d<diffs.size(); d++)
						{
							OsuDatabaseBeatmap *diff = diffs[d];
							bool uuidMatches = (diff->getMD5Hash().length() > 0);
							for (int u=0; u<32 && u<diff->getMD5Hash().length(); u++)
							{
								if (diff->getMD5Hash()[u] != pp->beatmapMD5Hash[u])
								{
									uuidMatches = false;
									break;
								}
							}

							if (uuidMatches)
							{
								found = true;
								m_osu->getSongBrowser()->selectBeatmapMP(diff);
								break;
							}
						}

						if (found)
							break;
					}

					if (!found)
					{
						m_sMPSelectBeatmapScheduledMD5Hash = std::string(pp->beatmapMD5Hash, sizeof(pp->beatmapMD5Hash));

						if (beatmaps.size() < 1)
						{
							if (m_osu->getMainMenu()->isVisible() && !m_bMPSelectBeatmapScheduled)
							{
								m_bMPSelectBeatmapScheduled = true;

								m_osu->toggleSongBrowser();
							}
							else
								m_osu->getNotificationOverlay()->addNotification("Database not yet loaded ...", 0xffffff00);
						}
						else
							m_osu->getNotificationOverlay()->addNotification((pp->beatmapId > 0 ? "Missing beatmap! -> ^^^ Click top left [Web] ^^^" : "Missing Beatmap!"), 0xffff0000);

						m_osu->getSongBrowser()->getInfoLabel()->setFromMissingBeatmap(pp->beatmapId);
					}

					// send status update to everyone
					onClientStatusUpdate(!found);
				}
				else if (m_osu->getSelectedBeatmap() != NULL)
				{
					if (pp->state == START)
					{
						if (m_osu->isInPlayMode())
							m_osu->getSelectedBeatmap()->stop(true);

						// only start if we actually have the correct beatmap
						OsuDatabaseBeatmap *diff2 = m_osu->getSelectedBeatmap()->getSelectedDifficulty2();
						if (diff2 != NULL)
						{
							bool uuidMatches = (diff2->getMD5Hash().length() > 0);
							for (int u=0; u<32 && u<diff2->getMD5Hash().length(); u++)
							{
								if (diff2->getMD5Hash()[u] != pp->beatmapMD5Hash[u])
								{
									uuidMatches = false;
									break;
								}
							}

							if (uuidMatches)
								m_osu->getSongBrowser()->onDifficultySelectedMP(m_osu->getSelectedBeatmap()->getSelectedDifficulty2(), true);
						}
					}
					else if (m_osu->isInPlayMode())
					{
						if (pp->state == SEEK)
							m_osu->getSelectedBeatmap()->seekMS(pp->seekMS);
						else if (pp->state == SKIP)
							m_osu->getSelectedBeatmap()->skipEmptySection();
						else if (pp->state == PAUSE)
							m_osu->getSelectedBeatmap()->pause(false);
						else if (pp->state == UNPAUSE)
							m_osu->getSelectedBeatmap()->pause(false);
						else if (pp->state == RESTART)
							m_osu->getSelectedBeatmap()->restart(pp->quickRestart);
						else if (pp->state == STOP)
							m_osu->getSelectedBeatmap()->stop();
					}
				}
			}
		}
		return true;

	case SCORE_TYPE:
		if (unwrappedSize >= sizeof(SCORE_PACKET))
		{
			// execute
			SCORE_PACKET *pp = (struct SCORE_PACKET*)unwrappedPacket;
			for (int i=0; i<m_clientPlayers.size(); i++)
			{
				if (m_clientPlayers[i].id == pp->id)
				{
					m_clientPlayers[i].combo = pp->combo;
					m_clientPlayers[i].accuracy = pp->accuracy;
					m_clientPlayers[i].score = pp->score;
					m_clientPlayers[i].dead = pp->dead;
					break;
				}
			}

			// sort players
			struct PlayerSortComparator
			{
			    bool operator() (PLAYER const &a, PLAYER const &b) const
			    {
			    	// strict weak ordering!
			    	if (osu_mp_win_condition_accuracy.getBool())
			    	{
			    		if (a.accuracy == b.accuracy)
			    		{
			    			if (a.dead == b.dead)
			    				return (a.sortHack > b.sortHack);
			    			else
			    				return b.dead;
			    		}
			    		else
			    		{
			    			if (a.dead == b.dead)
			    				return (a.accuracy > b.accuracy);
			    			else
			    				return b.dead;
			    		}
			    	}
			    	else
			    	{
			    		if (a.score == b.score)
			    		{
			    			if (a.dead == b.dead)
			    				return (a.sortHack > b.sortHack);
			    			else
			    				return b.dead;
			    		}
			    		else
			    		{
			    			if (a.dead == b.dead)
			    				return (a.score > b.score);
			    			else
			    				return b.dead;
			    		}
			    	}
			    }
			};
			std::sort(m_clientPlayers.begin(), m_clientPlayers.end(), PlayerSortComparator());
		}
		return true;

	case BEATMAP_DOWNLOAD_REQUEST_TYPE:
		if (unwrappedSize >= sizeof(BEATMAP_DOWNLOAD_REQUEST_PACKET))
		{
			// execute
			///BEATMAP_DOWNLOAD_REQUEST_PACKET *pp = (struct BEATMAP_DOWNLOAD_REQUEST_PACKET*)unwrappedPacket;

			if (m_osu->getSongBrowser()->getSelectedBeatmap() != NULL && m_osu->getSongBrowser()->getSelectedBeatmap()->getSelectedDifficulty2() != NULL)
			{
				// TODO: add support for requesting a specific beatmap download, and not just whatever the peer currently has selected
				OsuDatabaseBeatmap *diff = m_osu->getSongBrowser()->getSelectedBeatmap()->getSelectedDifficulty2();

				// HACKHACK: TODO: TEMP, force load background image (so local downloaded /tmp/ beatmaps are at least complete, even if background image loader is not finished yet)
				diff->setLoadBeatmapMetadata();
				engine->getResourceManager()->loadResource(diff);

				BeatmapUploadState uploadState;
				{
					uploadState.id = (isServer() ? id : 0); // NOTE: this tells the chunk uploader which function to use
					uploadState.serial = 0; // NOTE: this is set later below

					uploadState.numUploadedOsuFileBytes = 0;
					uploadState.numUploadedMusicFileBytes = 0;
					uploadState.numUploadedBackgroundFileBytes = 0;

					uploadState.osuFile = new File(diff->getFilePath());
					uploadState.musicFile = new File(diff->getFullSoundFilePath());
					uploadState.backgroundFile = new File(diff->getFullBackgroundImageFilePath());
				}

				if (uploadState.osuFile->canRead() && uploadState.musicFile->canRead()
					&& uploadState.osuFile->getFileSize() > 0 && uploadState.musicFile->getFileSize() > 0)
				{
					// send acknowledgement to whoever requested the download
					BEATMAP_DOWNLOAD_ACK_PACKET pp;
					{
						pp.serial = (uint32_t)rand(); // generate serial
						pp.osuFileSizeBytes = uploadState.osuFile->getFileSize();
						pp.musicFileSizeBytes = uploadState.musicFile->getFileSize();
						pp.backgroundFileSizeBytes = uploadState.backgroundFile->getFileSize();
						for (size_t i=0; i<32; i++)
						{
							pp.osuFileMD5Hash[i] = i < diff->getMD5Hash().length() ? diff->getMD5Hash()[i] : '\0';
						}
						for (int i=0; i<1023; i++)
						{
							pp.musicFileName[i] = i < diff->getAudioFileName().length() ? diff->getAudioFileName()[i] : '\0';
						}
						pp.musicFileName[1023] = '\0';
						for (int i=0; i<1023; i++)
						{
							pp.backgroundFileName[i] = i < diff->getBackgroundImageFileName().length() ? diff->getBackgroundImageFileName()[i] : '\0';
						}
						pp.backgroundFileName[1023] = '\0';
						size_t size = sizeof(BEATMAP_DOWNLOAD_ACK_PACKET);

						PACKET_TYPE wrap = BEATMAP_DOWNLOAD_ACK_TYPE;
						const int wrapperSize = sizeof(PACKET_TYPE);
						char wrappedPacket[(wrapperSize + size)];
						memcpy(&wrappedPacket, &wrap, wrapperSize);
						memcpy((void*)(((char*)&wrappedPacket) + wrapperSize), &pp, size);

						if (uploadState.id != 0)
							engine->getNetworkHandler()->clientcast(wrappedPacket, size + wrapperSize, uploadState.id, true);
						else
							engine->getNetworkHandler()->servercast(wrappedPacket, size + wrapperSize, true);
					}

					// remember sent serial for uploader
					uploadState.serial = pp.serial;

					// start upload
					m_uploads.push_back(uploadState);
				}
				else
				{
					delete uploadState.osuFile;
					delete uploadState.musicFile;
				}
			}
		}
		return true;

	case BEATMAP_DOWNLOAD_ACK_TYPE:
		if (unwrappedSize >= sizeof(BEATMAP_DOWNLOAD_ACK_PACKET))
		{
			// execute
			BEATMAP_DOWNLOAD_ACK_PACKET *pp = (struct BEATMAP_DOWNLOAD_ACK_PACKET*)unwrappedPacket;

			// we have received acknowledgement for the download request, chunks will start being received soon
			BeatmapDownloadState downloadState;
			{
				downloadState.serial = pp->serial;

				downloadState.totalDownloadBytes = pp->osuFileSizeBytes + pp->musicFileSizeBytes + pp->backgroundFileSizeBytes;

				downloadState.numDownloadedOsuFileBytes = 0;
				downloadState.numDownloadedMusicFileBytes = 0;
				downloadState.numDownloadedBackgroundFileBytes = 0;

				for (int i=0; i<32; i++)
				{
					downloadState.osuFileMD5Hash.append((wchar_t)(pp->osuFileMD5Hash[i]));
				}

				pp->musicFileName[1023] = '\0';
				for (size_t i=0; i<1023; i++)
				{
					if (pp->musicFileName[i] == '\0')
						break;
					else
						downloadState.musicFileName.append(pp->musicFileName[i]);
				}

				pp->backgroundFileName[1023] = '\0';
				for (size_t i=0; i<1023; i++)
				{
					if (pp->backgroundFileName[i] == '\0')
						break;
					else
						downloadState.backgroundFileName.append(pp->backgroundFileName[i]);
				}
				downloadState.backgroundFileName = UString((const wchar_t*)pp->backgroundFileName);

				debugLog("Started downloading %i %s %s %s\n", (int)downloadState.serial, downloadState.osuFileMD5Hash.toUtf8(), downloadState.musicFileName.toUtf8(), downloadState.backgroundFileName.toUtf8());
			}
			m_downloads.push_back(downloadState);

			// update client state to downloading
			onClientStatusUpdate(true, true, true);
		}
		return true;

	case BEATMAP_DOWNLOAD_CHUNK_TYPE:
		if (unwrappedSize >= sizeof(BEATMAP_DOWNLOAD_CHUNK_PACKET))
		{
			// execute
			BEATMAP_DOWNLOAD_CHUNK_PACKET *pp = (struct BEATMAP_DOWNLOAD_CHUNK_PACKET*)unwrappedPacket;

			//debugLog("received BEATMAP_DOWNLOAD_CHUNK_TYPE (serial = %i, fileType = %i)\n", (int)pp->serial, (int)pp->fileType);

			if (unwrappedSize - sizeof(BEATMAP_DOWNLOAD_CHUNK_PACKET) >= pp->numDataBytes)
			{
				for (size_t i=0; i<m_downloads.size(); i++)
				{
					if (m_downloads[i].serial == pp->serial)
					{
						// append data to vectors
						for (uint32_t c=0; c<pp->numDataBytes; c++)
						{
							switch (pp->fileType)
							{
							case 1:
								m_downloads[i].osuFileBytes.push_back(*(unwrappedPacket + sizeof(BEATMAP_DOWNLOAD_CHUNK_PACKET) + c));
								break;
							case 2:
								m_downloads[i].musicFileBytes.push_back(*(unwrappedPacket + sizeof(BEATMAP_DOWNLOAD_CHUNK_PACKET) + c));
								break;
							case 3:
								m_downloads[i].backgroundFileBytes.push_back(*(unwrappedPacket + sizeof(BEATMAP_DOWNLOAD_CHUNK_PACKET) + c));
								break;
							}
						}

						// update counters
						m_downloads[i].numDownloadedOsuFileBytes = m_downloads[i].osuFileBytes.size();
						m_downloads[i].numDownloadedMusicFileBytes = m_downloads[i].musicFileBytes.size();
						m_downloads[i].numDownloadedBackgroundFileBytes = m_downloads[i].backgroundFileBytes.size();
					}
				}
			}
			else
				debugLog("invalid beatmap chunk numDataBytes ...\n");
		}
		return true;

	default:
		return false;
	}
}

bool OsuMultiplayer::onServerReceive(uint32_t id, void *data, uint32_t size)
{
	const int wrapperSize = sizeof(PACKET_TYPE);
	if (size <= wrapperSize) return false;

	// unwrap the packet
	char *unwrappedPacket = (char*)data;
	unwrappedPacket += wrapperSize;
	const uint32_t unwrappedSize = (size - wrapperSize);

	const PACKET_TYPE type = (*(PACKET_TYPE*)data);
	switch (type)
	{
	case STATE_TYPE:
		if (unwrappedSize >= sizeof(GAME_STATE_PACKET))
		{
			// execute
			GAME_STATE_PACKET *pp = (struct GAME_STATE_PACKET*)unwrappedPacket;
			if (pp->state == SELECT && !m_osu->isInPlayMode() && osu_mp_allow_client_beatmap_select.getBool())
			{
				m_iLastClientBeatmapSelectID = id;
				return onClientReceiveInt(id, data, size, true);
			}
		}
		return false;

	case PLAYER_STATE_TYPE:
		return onClientReceiveInt(id, data, size, true);

	case PLAYER_CMD_TYPE:
		// TODO: only the server is interested in this for now, also unfinished
		if (unwrappedSize >= sizeof(PLAYER_INPUT_PACKET))
		{
			if (unwrappedSize % sizeof(PLAYER_INPUT_PACKET) == 0)
			{
				const uint32_t numPlayerInputs = (unwrappedSize / sizeof(PLAYER_INPUT_PACKET));
				const PLAYER_INPUT_PACKET *inputs = (PLAYER_INPUT_PACKET*)unwrappedPacket;
				for (int i=0; i<m_serverPlayers.size(); i++)
				{
					if (m_serverPlayers[i].id == id)
					{
						for (int c=0; c<numPlayerInputs; c++)
						{
							PLAYER_INPUT input;
							input.time = inputs[c].time;
							input.cursorPos.x = inputs[c].cursorPosX;
							input.cursorPos.y = inputs[c].cursorPosY;
							input.mouse1down = inputs[c].keys; // TODO
							input.mouse2down = inputs[c].keys; // TODO
							input.key1down = inputs[c].keys; // TODO
							input.key2down = inputs[c].keys; // TODO
							m_serverPlayers[i].inputBuffer.push_back(input);
						}
						break;
					}
				}
			}
		}
		return true;

	case BEATMAP_DOWNLOAD_REQUEST_TYPE:
		// we have received a download request from a client. start sending ack and then chunks once per frame etc. (client downloads from server)
		return onClientReceiveInt(id, data, size);

	case BEATMAP_DOWNLOAD_ACK_TYPE:
		// we have received acknowledgement for the download request (server downloads from client)
		return onClientReceiveInt(id, data, size);

	case BEATMAP_DOWNLOAD_CHUNK_TYPE:
		// we have received a new chunk (server downloads from client)
		return onClientReceiveInt(id, data, size);

	default:
		return true; // HACKHACK: TODO: dangerous, forwarding all broadcasts!
	}
}

void OsuMultiplayer::onClientConnectedToServer()
{
	m_osu->getNotificationOverlay()->addNotification("Connected.", 0xff00ff00);

	// force send current score state to server (e.g. if the server died, or client connection died, this can recover a match result)
	onClientScoreChange(m_osu->getScore()->getComboMax(), m_osu->getScore()->getAccuracy(), m_osu->getScore()->getScore(), m_osu->getScore()->isDead(), true);
}

void OsuMultiplayer::onClientDisconnectedFromServer()
{
	m_osu->getNotificationOverlay()->addNotification("Disconnected.", 0xffff0000);

	m_clientPlayers.clear();

	// kill all downloads and uploads
	m_downloads.clear();
	for (size_t i=0; i<m_uploads.size(); i++)
	{
		delete m_uploads[i].osuFile;
		delete m_uploads[i].musicFile;
		delete m_uploads[i].backgroundFile;
	}
	m_uploads.clear();
}

void OsuMultiplayer::onServerClientChange(uint32_t id, UString name, bool connected)
{
	debugLog("OsuMultiplayer::onServerClientChange(%i, %s, %i)\n", id, name.toUtf8(), (int)connected);

	PLAYER_CHANGE_PACKET pp;
	pp.id = id;
	pp.connected = connected;
	pp.size = clamp<int>(name.length(), 0, 254);
	for (int i=0; i<pp.size; i++)
	{
		pp.name[i] = name[i];
	}
	pp.name[254] = '\0';
	size_t size = sizeof(PLAYER_CHANGE_PACKET);

	PACKET_TYPE wrap = PLAYER_CHANGE_TYPE;
	const int wrapperSize = sizeof(PACKET_TYPE);
	char wrappedPacket[(sizeof(PACKET_TYPE) + size)];
	memcpy(&wrappedPacket, &wrap, wrapperSize);
	memcpy((void*)(((char*)&wrappedPacket) + wrapperSize), &pp, size);

	bool exists = false;
	for (int i=0; i<m_serverPlayers.size(); i++)
	{
		if (m_serverPlayers[i].id == id)
		{
			exists = true;
			if (!connected)
			{
				m_serverPlayers.erase(m_serverPlayers.begin() + i);

				engine->getNetworkHandler()->broadcast(wrappedPacket, size + wrapperSize, true);
			}
			break;
		}
	}

	if (!exists)
	{
		engine->getNetworkHandler()->broadcast(wrappedPacket, size + wrapperSize, true);

		// send playerlist to new client (before adding to m_serverPlayers)
		if (m_serverPlayers.size() > 0)
		{
			for (int i=0; i<m_serverPlayers.size(); i++)
			{
				pp.id = m_serverPlayers[i].id;
				pp.connected = true;
				pp.size = clamp<int>(m_serverPlayers[i].name.length(), 0, 254);
				for (int n=0; n<pp.size; n++)
				{
					pp.name[n] = m_serverPlayers[i].name[n];
				}
				memcpy((void*)(((char*)&wrappedPacket) + wrapperSize), &pp, size);

				engine->getNetworkHandler()->clientcast(wrappedPacket, size + wrapperSize, id, true);
			}
		}

		// send gamestate to new client
		// TODO: optimization, only send stuff like this to the current client (and not everyone)
		onServerModUpdate();

		PLAYER ply;
		ply.id = id;
		ply.name = name;

		ply.missingBeatmap = false;
		ply.downloadingBeatmap = false;
		ply.waiting = true;

		ply.combo = 0;
		ply.accuracy = 0.0f;
		ply.score = 0;
		ply.dead = false;

		ply.input.mouse1down = false;
		ply.input.mouse2down = false;
		ply.input.key1down = false;
		ply.input.key2down = false;

		ply.sortHack = sortHackCounter++;

		m_serverPlayers.push_back(ply);
	}
}

void OsuMultiplayer::onLocalServerStarted()
{
	debugLog("OsuMultiplayer::onLocalServerStarted()\n");
	m_osu->getNotificationOverlay()->addNotification("Server started.", 0xff00ff00);
}

void OsuMultiplayer::onLocalServerStopped()
{
	debugLog("OsuMultiplayer::onLocalServerStopped()\n");
	m_osu->getNotificationOverlay()->addNotification("Server stopped.", 0xffcccccc);

	m_serverPlayers.clear();
}

void OsuMultiplayer::onClientCmd()
{
	if (!engine->getNetworkHandler()->isClient() || m_playerInputBuffer.size() < 1) return;

	size_t size = sizeof(PLAYER_INPUT_PACKET) * m_playerInputBuffer.size();

	PACKET_TYPE wrap = PLAYER_CMD_TYPE;
	const int wrapperSize = sizeof(PACKET_TYPE);
	char wrappedPacket[(sizeof(PACKET_TYPE) + size)];
	memcpy(&wrappedPacket, &wrap, wrapperSize);
	memcpy((void*)(((char*)&wrappedPacket) + wrapperSize), &m_playerInputBuffer[0], size);

	engine->getNetworkHandler()->servercast(wrappedPacket, size + wrapperSize, false);
}

void OsuMultiplayer::onClientStatusUpdate(bool missingBeatmap, bool waiting, bool downloadingBeatmap)
{
	if (!engine->getNetworkHandler()->isClient()) return;

	PLAYER_STATE_PACKET pp;
	pp.id = engine->getNetworkHandler()->getLocalClientID();
	pp.missingBeatmap = missingBeatmap;
	pp.downloadingBeatmap = downloadingBeatmap;
	pp.waiting = (waiting && !missingBeatmap); // only wait if we actually have the beatmap
	size_t size = sizeof(PLAYER_STATE_PACKET);

	PACKET_TYPE wrap = PLAYER_STATE_TYPE;
	const int wrapperSize = sizeof(PACKET_TYPE);
	char wrappedPacket[(sizeof(PACKET_TYPE) + size)];
	memcpy(&wrappedPacket, &wrap, wrapperSize);
	memcpy((void*)(((char*)&wrappedPacket) + wrapperSize), &pp, size);

	if (!isServer())
		onClientReceive(pp.id, wrappedPacket, size + wrapperSize);

	engine->getNetworkHandler()->broadcast(wrappedPacket, size + wrapperSize, true);
}

void OsuMultiplayer::onClientScoreChange(int combo, float accuracy, unsigned long long score, bool dead, bool reliable)
{
	if (!engine->getNetworkHandler()->isClient()) return;

	SCORE_PACKET pp;
	pp.id = engine->getNetworkHandler()->getLocalClientID();
	pp.combo = combo;
	pp.accuracy = accuracy;
	pp.score = score;
	pp.dead = dead;
	size_t size = sizeof(SCORE_PACKET);

	PACKET_TYPE wrap = SCORE_TYPE;
	const int wrapperSize = sizeof(PACKET_TYPE);
	char wrappedPacket[(sizeof(PACKET_TYPE) + size)];
	memcpy(&wrappedPacket, &wrap, wrapperSize);
	memcpy((void*)(((char*)&wrappedPacket) + wrapperSize), &pp, size);

	if (!isServer())
		onClientReceive(pp.id, wrappedPacket, size + wrapperSize);

	engine->getNetworkHandler()->broadcast(wrappedPacket, size + wrapperSize, reliable);
}

bool OsuMultiplayer::onClientPlayStateChangeRequestBeatmap(OsuDatabaseBeatmap *beatmap)
{
	if (!engine->getNetworkHandler()->isClient() || isServer()) return false;

	const bool isBeatmapAndDiffValid = (beatmap != NULL);

	if (!isBeatmapAndDiffValid) return false;

	GAME_STATE_PACKET pp;
	pp.state = SELECT;
	pp.seekMS = 0;
	pp.quickRestart = false;
	for (int i=0; i<32; i++)
	{
		if (i < beatmap->getMD5Hash().length())
			pp.beatmapMD5Hash[i] = beatmap->getMD5Hash()[i];
		else
			pp.beatmapMD5Hash[i] = 0;
	}
	pp.beatmapId = beatmap->getID();
	size_t size = sizeof(GAME_STATE_PACKET);

	PACKET_TYPE wrap = STATE_TYPE;
	const int wrapperSize = sizeof(PACKET_TYPE);
	char wrappedPacket[(sizeof(PACKET_TYPE) + size)];
	memcpy(&wrappedPacket, &wrap, wrapperSize);
	memcpy((void*)(((char*)&wrappedPacket) + wrapperSize), &pp, size);

	engine->getNetworkHandler()->broadcast(wrappedPacket, size + wrapperSize, true);

	onClientStatusUpdate(false);

	m_osu->getNotificationOverlay()->addNotification("Request sent.", 0xff00ff00, false, 0.25f);

	return true;
}

void OsuMultiplayer::onClientBeatmapDownloadRequest()
{
	if (!engine->getNetworkHandler()->isClient()) return;

	BEATMAP_DOWNLOAD_REQUEST_PACKET pp;
	pp.dummy = 0;
	size_t size = sizeof(BEATMAP_DOWNLOAD_REQUEST_PACKET);

	PACKET_TYPE wrap = BEATMAP_DOWNLOAD_REQUEST_TYPE;
	const int wrapperSize = sizeof(PACKET_TYPE);
	char wrappedPacket[(wrapperSize + size)];
	memcpy(&wrappedPacket, &wrap, wrapperSize);
	memcpy((void*)(((char*)&wrappedPacket) + wrapperSize), &pp, size);

	if (!isServer())
	{
		// client asks server for download
		engine->getNetworkHandler()->servercast(wrappedPacket, size + wrapperSize, true);
	}
	else
	{
		// server asks client for download (client which requested the beatmap select we don't have)
		engine->getNetworkHandler()->clientcast(wrappedPacket, size + wrapperSize, m_iLastClientBeatmapSelectID, true);
	}
}

void OsuMultiplayer::onServerModUpdate()
{
	if (!isServer()) return;

	// server

	// put all mod related convars in here
	UString string = "";
	if (!osu_mp_freemod.getBool())
	{
		string = "osu_mods ";
		{
			UString mods = convar->getConVarByName("osu_mods")->getString();
			if (mods.length() < 1)
				string.append(" ;");
			else
				string.append(mods);
		}
		string.append(";");
	}

	std::vector<UString> simpleModConVars;

	// mp
	simpleModConVars.push_back("osu_mp_win_condition_accuracy");

	// mods
	if (!osu_mp_freemod.getBool() || !osu_mp_freemod_all.getBool())
	{
		// overrides
		simpleModConVars.push_back("osu_cs_override");
		simpleModConVars.push_back("osu_ar_override_lock");
		simpleModConVars.push_back("osu_ar_override");
		simpleModConVars.push_back("osu_od_override_lock");
		simpleModConVars.push_back("osu_od_override");
		simpleModConVars.push_back("osu_speed_override");

		// experimental mods
		std::vector<ConVar*> experimentalMods = m_osu->getExperimentalMods();
		for (int i=0; i<experimentalMods.size(); i++)
		{
			simpleModConVars.push_back(experimentalMods[i]->getName());
		}

		// drain
		simpleModConVars.push_back("osu_drain_type");
		simpleModConVars.push_back("osu_drain_kill");

		simpleModConVars.push_back("osu_drain_vr_duration");
		simpleModConVars.push_back("osu_drain_vr_multiplier");
		simpleModConVars.push_back("osu_drain_vr_300");
		simpleModConVars.push_back("osu_drain_vr_100");
		simpleModConVars.push_back("osu_drain_vr_50");
		simpleModConVars.push_back("osu_drain_vr_miss");
		simpleModConVars.push_back("osu_drain_vr_sliderbreak");

		simpleModConVars.push_back("osu_drain_stable_hpbar_maximum");

		simpleModConVars.push_back("osu_drain_lazer_multiplier");
		simpleModConVars.push_back("osu_drain_lazer_300");
		simpleModConVars.push_back("osu_drain_lazer_100");
		simpleModConVars.push_back("osu_drain_lazer_50");
		simpleModConVars.push_back("osu_drain_lazer_miss");
		simpleModConVars.push_back("osu_drain_lazer_health_min");
		simpleModConVars.push_back("osu_drain_lazer_health_mid");
		simpleModConVars.push_back("osu_drain_lazer_health_max");
	}

	// build final string
	for (int i=0; i<simpleModConVars.size(); i++)
	{
		string.append(simpleModConVars[i]);
		string.append(" ");
		string.append(convar->getConVarByName(simpleModConVars[i])->getString());
		string.append(";");
	}

	onClientCommandInt(string, false);
}

void OsuMultiplayer::onServerPlayStateChange(OsuMultiplayer::STATE state, unsigned long seekMS, bool quickRestart, OsuDatabaseBeatmap *beatmap)
{
	if (!isServer()) return;

	const bool isBeatmapAndDiffValid = (beatmap != NULL);

	// server
	debugLog("OsuMultiplayer::onServerPlayStateChange(%i)\n", (int)state);

	GAME_STATE_PACKET pp;
	pp.state = state;
	pp.seekMS = seekMS;
	pp.quickRestart = quickRestart;
	for (int i=0; i<32; i++)
	{
		if (isBeatmapAndDiffValid && i < beatmap->getMD5Hash().length())
			pp.beatmapMD5Hash[i] = beatmap->getMD5Hash()[i];
		else
			pp.beatmapMD5Hash[i] = 0;
	}
	pp.beatmapId = (isBeatmapAndDiffValid ? beatmap->getID() : 0);
	size_t size = sizeof(GAME_STATE_PACKET);

	PACKET_TYPE wrap = STATE_TYPE;
	const int wrapperSize = sizeof(PACKET_TYPE);
	char wrappedPacket[(sizeof(PACKET_TYPE) + size)];
	memcpy(&wrappedPacket, &wrap, wrapperSize);
	memcpy((void*)(((char*)&wrappedPacket) + wrapperSize), &pp, size);

	engine->getNetworkHandler()->broadcast(wrappedPacket, size + wrapperSize, true);

	if (isBeatmapAndDiffValid)
		onClientStatusUpdate(false);
}

void OsuMultiplayer::setBeatmap(OsuDatabaseBeatmap *beatmap)
{
	if (beatmap == NULL) return;
	setBeatmap(beatmap->getMD5Hash());
}

void OsuMultiplayer::setBeatmap(std::string md5hash)
{
	if (md5hash.length() < 32) return;

	GAME_STATE_PACKET pp;
	pp.state = STATE::SELECT;
	pp.seekMS = 0;
	pp.quickRestart = false;
	for (int i=0; i<32 && i<md5hash.length(); i++)
	{
		pp.beatmapMD5Hash[i] = md5hash[i];
	}
	pp.beatmapId = /*beatmap->getSelectedDifficulty()->beatmapId*/0;
	size_t size = sizeof(GAME_STATE_PACKET);

	PACKET_TYPE wrap = STATE_TYPE;
	const int wrapperSize = sizeof(PACKET_TYPE);
	char wrappedPacket[(sizeof(PACKET_TYPE) + size)];
	memcpy(&wrappedPacket, &wrap, wrapperSize);
	memcpy((void*)(((char*)&wrappedPacket) + wrapperSize), &pp, size);

	onClientReceiveInt(0, wrappedPacket, size + wrapperSize);
}

bool OsuMultiplayer::isServer()
{
	return engine->getNetworkHandler()->isServer();
}

bool OsuMultiplayer::isInMultiplayer()
{
	return engine->getNetworkHandler()->isClient() || engine->getNetworkHandler()->isServer();
}

bool OsuMultiplayer::isMissingBeatmap()
{
	for (size_t i=0; i<m_clientPlayers.size(); i++)
	{
		if (m_clientPlayers[i].id == engine->getNetworkHandler()->getLocalClientID())
			return m_clientPlayers[i].missingBeatmap;
	}
	return false;
}

bool OsuMultiplayer::isWaitingForPlayers()
{
	for (int i=0; i<m_clientPlayers.size(); i++)
	{
		if (m_clientPlayers[i].waiting || m_clientPlayers[i].downloadingBeatmap)
			return true;
	}
	return false;
}

bool OsuMultiplayer::isWaitingForClient()
{
	for (int i=0; i<m_clientPlayers.size(); i++)
	{
		if (m_clientPlayers[i].id == engine->getNetworkHandler()->getLocalClientID())
			return (m_clientPlayers[i].waiting || m_clientPlayers[i].downloadingBeatmap);
	}
	return false;
}

float OsuMultiplayer::getDownloadBeatmapPercentage() const
{
	if (!isDownloadingBeatmap() || m_downloads.size() < 1) return 0.0f;

	const size_t totalDownloadedBytes = m_downloads[0].numDownloadedOsuFileBytes + m_downloads[0].numDownloadedMusicFileBytes + m_downloads[0].numDownloadedBackgroundFileBytes;

	return ((float)totalDownloadedBytes / (float)m_downloads[0].totalDownloadBytes);
}

void OsuMultiplayer::onBroadcastCommand(UString command)
{
	onClientCommandInt(command, false);
}

void OsuMultiplayer::onClientcastCommand(UString command)
{
	onClientCommandInt(command, true);
}

void OsuMultiplayer::onClientCommandInt(UString string, bool executeLocallyToo)
{
	if (!isServer() || string.length() < 1) return;

	// execute locally on server
	if (executeLocallyToo)
		Console::processCommand(string);

	// WARNING: hardcoded max length (2048)
	//debugLog("length = %i", string.length());

	CONVAR_PACKET pp;
	pp.len = clamp<int>(string.length(), 0, 2047);
	for (int i=0; i<pp.len; i++)
	{
		pp.str[i] = string[i];
	}
	pp.str[2047] = '\0';
	size_t size = sizeof(CONVAR_PACKET);

	PACKET_TYPE wrap = CONVAR_TYPE;
	const int wrapperSize = sizeof(PACKET_TYPE);
	char wrappedPacket[(sizeof(PACKET_TYPE) + size)];
	memcpy(&wrappedPacket, &wrap, wrapperSize);
	memcpy((void*)(((char*)&wrappedPacket) + wrapperSize), &pp, size);

	engine->getNetworkHandler()->broadcast(wrappedPacket, size + wrapperSize, true);
}

void OsuMultiplayer::onMPForceClientBeatmapDownload()
{
	if (!isServer()) return;

	onClientCommandInt(osu_mp_request_beatmap_download.getName(), false);
}

void OsuMultiplayer::onMPSelectBeatmap(UString md5hash)
{
	setBeatmap(std::string(md5hash.toUtf8()));
}

void OsuMultiplayer::onMPRequestBeatmapDownload()
{
	if (isServer()) return;

	onClientBeatmapDownloadRequest();
}

void OsuMultiplayer::onBeatmapDownloadFinished(const BeatmapDownloadState &dl)
{
	// update client state to no-longer-downloading
	onClientStatusUpdate(true, true, false);

	// TODO: validate inputs

	// create folder structure
	UString beatmapFolderPath = "tmp/";
	{
		if (!env->directoryExists(beatmapFolderPath))
			env->createDirectory(beatmapFolderPath);

		beatmapFolderPath.append(dl.osuFileMD5Hash);
		beatmapFolderPath.append("/");
		if (!env->directoryExists(beatmapFolderPath))
			env->createDirectory(beatmapFolderPath);
	}

	// write downloaded files to disk
	{
		UString osuFilePath = beatmapFolderPath;
		osuFilePath.append(dl.osuFileMD5Hash);
		osuFilePath.append(".osu");

		if (!env->fileExists(osuFilePath))
		{
			File osuFile(osuFilePath, File::TYPE::WRITE);
			osuFile.write((const char*)&dl.osuFileBytes[0], dl.osuFileBytes.size());
		}
	}
	{
		UString musicFilePath = beatmapFolderPath;
		musicFilePath.append(dl.musicFileName);

		if (!env->fileExists(musicFilePath))
		{
			File musicFile(musicFilePath, File::TYPE::WRITE);
			musicFile.write((const char*)&dl.musicFileBytes[0], dl.musicFileBytes.size());
		}
	}
	if (dl.backgroundFileName.length() > 1)
	{
		UString backgroundFilePath = beatmapFolderPath;
		backgroundFilePath.append(dl.backgroundFileName);

		if (!env->fileExists(backgroundFilePath))
		{
			File backgroundFile(backgroundFilePath, File::TYPE::WRITE);
			backgroundFile.write((const char*)&dl.backgroundFileBytes[0], dl.backgroundFileBytes.size());
		}
	}

	// and load the beatmap
	OsuDatabaseBeatmap *beatmap = m_osu->getSongBrowser()->getDatabase()->addBeatmap(beatmapFolderPath);
	if (beatmap != NULL)
	{
		m_osu->getSongBrowser()->addBeatmap(beatmap);
		m_osu->getSongBrowser()->updateSongButtonSorting();
		if (beatmap->getDifficulties().size() > 0) // NOTE: always assume only 1 diff exists for now
		{
			m_osu->getSongBrowser()->selectBeatmapMP(beatmap->getDifficulties()[0]);

			// and update our state if we have the correct beatmap now
			if (beatmap->getDifficulties()[0]->getMD5Hash() == m_sMPSelectBeatmapScheduledMD5Hash)
				onClientStatusUpdate(false, true, false);
		}
	}
}
