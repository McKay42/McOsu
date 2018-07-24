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
#include "NetworkHandler.h"
#include "ResourceManager.h"
#include "Environment.h"

#include "Osu.h"
#include "OsuDatabase.h"
#include "OsuBeatmap.h"
#include "OsuBeatmapDifficulty.h"
#include "OsuSongBrowser2.h"
#include "OsuNotificationOverlay.h"

#include "OsuUISongBrowserInfoLabel.h"

ConVar osu_mp_freemod("osu_mp_freemod", false);
ConVar osu_mp_freemod_all("osu_mp_freemod_all", true, "allow everything, or only standard osu mods");
ConVar osu_mp_win_condition_accuracy("osu_mp_win_condition_accuracy", false);
ConVar osu_mp_allow_client_beatmap_select("osu_mp_sv_allow_client_beatmap_select", true);
ConVar osu_mp_broadcastcommand("osu_mp_broadcastcommand");
ConVar osu_mp_clientcastcommand("osu_mp_clientcastcommand");

unsigned long long OsuMultiplayer::sortHackCounter = 0;

OsuMultiplayer::OsuMultiplayer(Osu *osu)
{
	m_osu = osu;

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

bool OsuMultiplayer::onClientReceive(unsigned int id, void *data, size_t size)
{
	return onClientReceiveInt(id, data, size);
}

bool OsuMultiplayer::onClientReceiveInt(unsigned int id, void *data, size_t size, bool forceAcceptOnServer)
{
	PACKET_TYPE type = (*(PACKET_TYPE*)data);
	switch (type)
	{
	case PLAYER_CHANGE_TYPE:
		{
			// unwrap the packet
			char *unwrappedPacket = (char*)data;
			const int wrapperSize = sizeof(PACKET_TYPE);
			unwrappedPacket += wrapperSize;

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
				ply.waiting = true;
				ply.combo = 0;
				ply.accuracy = 0.0f;
				ply.score = 0;
				ply.dead = false;
				ply.sortHack = sortHackCounter++;

				m_clientPlayers.push_back(ply);

				debugLog("Player %i joined the game.\n", pp->id);
			}
		}
		return true;

	case PLAYER_STATE_TYPE:
		{
			// unwrap the packet
			char *unwrappedPacket = (char*)data;
			const int wrapperSize = sizeof(PACKET_TYPE);
			unwrappedPacket += wrapperSize;

			// execute
			PLAYER_STATE_PACKET *pp = (struct PLAYER_STATE_PACKET*)unwrappedPacket;
			for (int i=0; i<m_clientPlayers.size(); i++)
			{
				if (m_clientPlayers[i].id == pp->id)
				{
					// player state update
					m_clientPlayers[i].missingBeatmap = pp->missingBeatmap;
					m_clientPlayers[i].waiting = pp->waiting;
					break;
				}
			}
		}
		return true;

	case CONVAR_TYPE:
		if (!engine->getNetworkHandler()->isServer() || forceAcceptOnServer)
		{
			// unwrap the packet
			char *unwrappedPacket = (char*)data;
			const int wrapperSize = sizeof(PACKET_TYPE);
			unwrappedPacket += wrapperSize;

			// execute
			CONVAR_PACKET *pp = (struct CONVAR_PACKET*)unwrappedPacket;
			Console::processCommand(UString(pp->str).substr(0, pp->len));
		}
		return true;

	case STATE_TYPE:
		if (!engine->getNetworkHandler()->isServer() || forceAcceptOnServer)
		{
			// unwrap the packet
			char *unwrappedPacket = (char*)data;
			const int wrapperSize = sizeof(PACKET_TYPE);
			unwrappedPacket += wrapperSize;

			// execute
			GAME_STATE_PACKET *pp = (struct GAME_STATE_PACKET*)unwrappedPacket;
			if (pp->state == SELECT)
			{
				bool found = false;
				std::vector<OsuBeatmap*> beatmaps = m_osu->getSongBrowser()->getDatabase()->getBeatmaps();
				for (int i=0; i<beatmaps.size(); i++)
				{
					OsuBeatmap *beatmap = beatmaps[i];
					for (int d=0; d<beatmap->getDifficultiesPointer()->size(); d++)
					{
						OsuBeatmapDifficulty *diff = (*beatmap->getDifficultiesPointer())[d];
						bool uuidMatches = (diff->md5hash.length() > 0);
						for (int u=0; u<32 && u<diff->md5hash.length(); u++)
						{
							if (diff->md5hash[u] != pp->beatmapMD5Hash[u])
							{
								uuidMatches = false;
								break;
							}
						}
						if (uuidMatches)
						{
							found = true;
							m_osu->getSongBrowser()->selectBeatmapMP(beatmap, diff);
							break;
						}
					}
					if (found)
						break;
				}

				if (!found)
				{
					m_osu->getNotificationOverlay()->addNotification((pp->beatmapId > 0 ? "Missing beatmap! -> ^^^ Click top left corner ^^^" : "Missing Beatmap!"), 0xffff0000);
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
					OsuBeatmapDifficulty *diff = m_osu->getSelectedBeatmap()->getSelectedDifficulty();
					if (diff != NULL)
					{
						bool uuidMatches = (diff->md5hash.length() > 0);
						for (int u=0; u<32 && u<diff->md5hash.length(); u++)
						{
							if (diff->md5hash[u] != pp->beatmapMD5Hash[u])
							{
								uuidMatches = false;
								break;
							}
						}

						if (uuidMatches)
							m_osu->getSongBrowser()->onDifficultySelectedMP(m_osu->getSelectedBeatmap(), m_osu->getSelectedBeatmap()->getSelectedDifficulty(), true);
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
		return true;

	case SCORE_TYPE:
		{
			// unwrap the packet
			char *unwrappedPacket = (char*)data;
			const int wrapperSize = sizeof(PACKET_TYPE);
			unwrappedPacket += wrapperSize;

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

	default:
		return false;
	}
}

bool OsuMultiplayer::onServerReceive(unsigned int id, void *data, size_t size)
{
	PACKET_TYPE type = (*(PACKET_TYPE*)data);
	switch (type)
	{
	case STATE_TYPE:
		{
			// unwrap the packet
			char *unwrappedPacket = (char*)data;
			const int wrapperSize = sizeof(PACKET_TYPE);
			unwrappedPacket += wrapperSize;

			// execute
			GAME_STATE_PACKET *pp = (struct GAME_STATE_PACKET*)unwrappedPacket;
			if (pp->state == SELECT && !m_osu->isInPlayMode() && osu_mp_allow_client_beatmap_select.getBool())
				return onClientReceiveInt(id, data, size, true);
		}
		return false;

	case PLAYER_STATE_TYPE:
		return onClientReceiveInt(id, data, size, true);

	default:
		return true;// HACKHACK: TODO: dangerous, forwarding all broadcasts!
	}
}

void OsuMultiplayer::onClientConnectedToServer()
{
	m_osu->getNotificationOverlay()->addNotification("Connected.", 0xff00ff00);
}

void OsuMultiplayer::onClientDisconnectedFromServer()
{
	m_clientPlayers.clear();
	m_osu->getNotificationOverlay()->addNotification("Disconnected.", 0xffff0000);
}

void OsuMultiplayer::onServerClientChange(unsigned int id, UString name, bool connected)
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

		PLAYER ply;
		ply.id = id;
		ply.name = name;
		ply.missingBeatmap = false;
		ply.waiting = true;
		ply.combo = 0;
		ply.accuracy = 0.0f;
		ply.score = 0;
		ply.dead = false;

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
	m_serverPlayers.clear();
	debugLog("OsuMultiplayer::onLocalServerStopped()\n");
	m_osu->getNotificationOverlay()->addNotification("Server stopped.", 0xffcccccc);
}

void OsuMultiplayer::onClientStatusUpdate(bool missingBeatmap, bool waiting)
{
	if (!engine->getNetworkHandler()->isClient()) return;

	PLAYER_STATE_PACKET pp;
	pp.id = engine->getNetworkHandler()->getLocalClientID();
	pp.missingBeatmap = missingBeatmap;
	pp.waiting = (waiting && !missingBeatmap); // only wait if we actually have the beatmap
	size_t size = sizeof(PLAYER_STATE_PACKET);

	PACKET_TYPE wrap = PLAYER_STATE_TYPE;
	const int wrapperSize = sizeof(PACKET_TYPE);
	char wrappedPacket[(sizeof(PACKET_TYPE) + size)];
	memcpy(&wrappedPacket, &wrap, wrapperSize);
	memcpy((void*)(((char*)&wrappedPacket) + wrapperSize), &pp, size);

	if (!engine->getNetworkHandler()->isServer())
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

	if (!engine->getNetworkHandler()->isServer())
		onClientReceive(pp.id, wrappedPacket, size + wrapperSize);

	engine->getNetworkHandler()->broadcast(wrappedPacket, size + wrapperSize, reliable);
}

bool OsuMultiplayer::onClientPlayStateChangeRequestBeatmap(OsuBeatmap *beatmap)
{
	if (!engine->getNetworkHandler()->isClient() || engine->getNetworkHandler()->isServer()) return false;

	const bool isBeatmapAndDiffValid = (beatmap != NULL && beatmap->getSelectedDifficulty() != NULL);

	if (!isBeatmapAndDiffValid) return false;

	GAME_STATE_PACKET pp;
	pp.state = SELECT;
	pp.seekMS = 0;
	pp.quickRestart = false;
	for (int i=0; i<32; i++)
	{
		if (i < beatmap->getSelectedDifficulty()->md5hash.length())
			pp.beatmapMD5Hash[i] = beatmap->getSelectedDifficulty()->md5hash[i];
		else
			pp.beatmapMD5Hash[i] = 0;
	}
	pp.beatmapId = beatmap->getSelectedDifficulty()->beatmapId;
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

void OsuMultiplayer::onServerModUpdate()
{
	if (!engine->getNetworkHandler()->isServer()) return;

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
		simpleModConVars.push_back("osu_ar_override");
		simpleModConVars.push_back("osu_od_override");
		simpleModConVars.push_back("osu_speed_override");

		// experimental mods
		std::vector<ConVar*> experimentalMods = m_osu->getExperimentalMods();
		for (int i=0; i<experimentalMods.size(); i++)
		{
			simpleModConVars.push_back(experimentalMods[i]->getName());
		}

		// drain
		simpleModConVars.push_back("osu_drain_enabled");
		simpleModConVars.push_back("osu_draw_hpbar");
		simpleModConVars.push_back("osu_drain_duration");
		simpleModConVars.push_back("osu_drain_multiplier");
		simpleModConVars.push_back("osu_drain_300");
		simpleModConVars.push_back("osu_drain_100");
		simpleModConVars.push_back("osu_drain_50");
		simpleModConVars.push_back("osu_drain_miss");
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

void OsuMultiplayer::onServerPlayStateChange(OsuMultiplayer::STATE state, unsigned long seekMS, bool quickRestart, OsuBeatmap *beatmap)
{
	if (!engine->getNetworkHandler()->isServer()) return;

	const bool isBeatmapAndDiffValid = (beatmap != NULL && beatmap->getSelectedDifficulty() != NULL);

	// server
	debugLog("OsuMultiplayer::onServerPlayStateChange(%i)\n", (int)state);

	GAME_STATE_PACKET pp;
	pp.state = state;
	pp.seekMS = seekMS;
	pp.quickRestart = quickRestart;
	for (int i=0; i<32; i++)
	{
		if (isBeatmapAndDiffValid && i < beatmap->getSelectedDifficulty()->md5hash.length())
			pp.beatmapMD5Hash[i] = beatmap->getSelectedDifficulty()->md5hash[i];
		else
			pp.beatmapMD5Hash[i] = 0;
	}
	pp.beatmapId = (isBeatmapAndDiffValid ? beatmap->getSelectedDifficulty()->beatmapId : 0);
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

bool OsuMultiplayer::isServer()
{
	return engine->getNetworkHandler()->isServer();
}

bool OsuMultiplayer::isInMultiplayer()
{
	return engine->getNetworkHandler()->isClient() || engine->getNetworkHandler()->isServer();
}

bool OsuMultiplayer::isWaitingForPlayers()
{
	for (int i=0; i<m_clientPlayers.size(); i++)
	{
		if (m_clientPlayers[i].waiting)
			return true;
	}
	return false;
}

bool OsuMultiplayer::isWaitingForClient()
{
	for (int i=0; i<m_clientPlayers.size(); i++)
	{
		if (m_clientPlayers[i].id == engine->getNetworkHandler()->getLocalClientID())
			return m_clientPlayers[i].waiting;
	}
	return false;
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
	if (!engine->getNetworkHandler()->isServer() || string.length() < 1) return;

	// execute locally on server
	if (executeLocallyToo)
		Console::processCommand(string);

	// WARNING: hardcoded max length (1024)
	//debugLog("length = %i", string.length());

	CONVAR_PACKET pp;
	pp.len = clamp<int>(string.length(), 0, 1023);
	for (int i=0; i<pp.len; i++)
	{
		pp.str[i] = string[i];
	}
	pp.str[1023] = '\0';
	size_t size = sizeof(CONVAR_PACKET);

	PACKET_TYPE wrap = CONVAR_TYPE;
	const int wrapperSize = sizeof(PACKET_TYPE);
	char wrappedPacket[(sizeof(PACKET_TYPE) + size)];
	memcpy(&wrappedPacket, &wrap, wrapperSize);
	memcpy((void*)(((char*)&wrappedPacket) + wrapperSize), &pp, size);

	engine->getNetworkHandler()->broadcast(wrappedPacket, size + wrapperSize, true);
}
