//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		network play
//
// $NoKeywords: $osump
//===============================================================================//

#ifndef OSUMULTIPLAYER_H
#define OSUMULTIPLAYER_H

#include "cbase.h"

class Osu;
class OsuBeatmap;

class OsuMultiplayer
{
public:
	enum STATE
	{
		SELECT,
		START,
		SEEK,
		SKIP,
		PAUSE,
		UNPAUSE,
		RESTART,
		STOP
	};

	struct PLAYER_INPUT
	{
		long time;
		Vector2 cursorPos;
		bool mouse1down;
		bool mouse2down;
		bool key1down;
		bool key2down;
	};

	struct PLAYER
	{
		unsigned int id;
		UString name;

		// state
		bool missingBeatmap;
		bool waiting;

		// score
		int combo;
		float accuracy;
		unsigned long long score;
		bool dead;

		// input
		PLAYER_INPUT input;
		std::vector<PLAYER_INPUT> inputBuffer;

		unsigned long long sortHack;
	};

public:
	OsuMultiplayer(Osu *osu);
	~OsuMultiplayer();

	void update();

	// receive
	bool onClientReceive(uint32_t id, void *data, uint32_t size);
	bool onClientReceiveInt(uint32_t id, void *data, uint32_t size, bool forceAcceptOnServer = false);
	bool onServerReceive(uint32_t id, void *data, uint32_t size);

	// client events
	void onClientConnectedToServer();
	void onClientDisconnectedFromServer();

	// server events
	void onServerClientChange(uint32_t id, UString name, bool connected);
	void onLocalServerStarted();
	void onLocalServerStopped();

	// clientside game events
	void onClientCmd();
	void onClientStatusUpdate(bool missingBeatmap, bool waiting = true);
	void onClientScoreChange(int combo, float accuracy, unsigned long long score, bool dead, bool reliable = false);
	bool onClientPlayStateChangeRequestBeatmap(OsuBeatmap *beatmap);

	// serverside game events
	void onServerModUpdate();
	void onServerPlayStateChange(STATE state, unsigned long seekMS = 0, bool quickRestart = false, OsuBeatmap *beatmap = NULL);

	// tourney events
	void setBeatmap(OsuBeatmap *beatmap);
	void setBeatmap(std::string md5hash);

	bool isServer();
	bool isInMultiplayer();

	bool isWaitingForPlayers();	// are we waiting for any player
	bool isWaitingForClient();	// is the waiting state set for the local player

	inline std::vector<PLAYER> *getPlayers() {return &m_clientPlayers;}
	inline std::vector<PLAYER> *getServerPlayers() {return &m_serverPlayers;}

private:
	static unsigned long long sortHackCounter;
	static ConVar *m_cl_cmdrate;

	void onClientcastCommand(UString command);
	void onBroadcastCommand(UString command);
	void onClientCommandInt(UString string, bool executeLocallyToo);

	void onMPSelectBeatmap(UString md5hash);

	enum PACKET_TYPE
	{
		PLAYER_CHANGE_TYPE,
		PLAYER_STATE_TYPE,
		PLAYER_CMD_TYPE,
		CONVAR_TYPE,
		STATE_TYPE,
		SCORE_TYPE
	};

#pragma pack(1)

	struct PLAYER_CHANGE_PACKET
	{
		uint32_t id;
		bool connected;
		size_t size;
		wchar_t name[255];
	};

	struct PLAYER_STATE_PACKET
	{
		uint32_t id;
		bool missingBeatmap;	// this is only used visually
		bool waiting;			// this will block all players until everyone is ready
	};

	struct PLAYER_INPUT_PACKET
	{
		int32_t time;			// 32 bits
		int16_t cursorPosX;		// 16 bits
		int16_t cursorPosY;		// 16 bits
		unsigned char keys;		//  8 bits
	};

	struct CONVAR_PACKET
	{
		wchar_t str[1024];
		size_t len;
	};

	struct GAME_STATE_PACKET
	{
		STATE state;
		unsigned long seekMS;
		bool quickRestart;
		char beatmapMD5Hash[32];
		long beatmapId;
	};

	struct SCORE_PACKET
	{
		uint32_t id;
		int32_t combo;
		float accuracy;
		uint64_t score;
		bool dead;
	};

#pragma pack()

	Osu *m_osu;
	std::vector<PLAYER> m_serverPlayers;
	std::vector<PLAYER> m_clientPlayers;

	float m_fNextPlayerCmd;
	std::vector<PLAYER_INPUT_PACKET> m_playerInputBuffer;

	bool m_bMPSelectBeatmapScheduled;
	std::string m_sMPSelectBeatmapScheduledMD5Hash;
};

#endif
