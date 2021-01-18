//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		network play
//
// $NoKeywords: $osump
//===============================================================================//

#ifndef OSUMULTIPLAYER_H
#define OSUMULTIPLAYER_H

#include "cbase.h"

class File;

class Osu;
class OsuDatabaseBeatmap;

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
		bool downloadingBeatmap;
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
	void onClientStatusUpdate(bool missingBeatmap, bool waiting = true, bool downloadingBeatmap = false);
	void onClientScoreChange(int combo, float accuracy, unsigned long long score, bool dead, bool reliable = false);
	bool onClientPlayStateChangeRequestBeatmap(OsuDatabaseBeatmap *beatmap);
	void onClientBeatmapDownloadRequest();

	// serverside game events
	void onServerModUpdate();
	void onServerPlayStateChange(STATE state, unsigned long seekMS = 0, bool quickRestart = false, OsuDatabaseBeatmap *beatmap = NULL);

	// tourney events
	void setBeatmap(OsuDatabaseBeatmap *beatmap);
	void setBeatmap(std::string md5hash);

	bool isServer();
	bool isInMultiplayer();

	bool isMissingBeatmap(); // are we missing the serverside beatmap
	bool isWaitingForPlayers();	// are we waiting for any player
	bool isWaitingForClient();	// is the waiting state set for the local player

	inline bool isDownloadingBeatmap() const {return m_downloads.size() > 0;}

	inline std::vector<PLAYER> *getPlayers() {return &m_clientPlayers;}
	//inline std::vector<PLAYER> *getServerPlayers() {return &m_serverPlayers;}

	float getDownloadBeatmapPercentage() const;

private:
	struct BeatmapUploadState
	{
		uint32_t id;
		uint32_t serial;

		size_t numUploadedOsuFileBytes;
		size_t numUploadedMusicFileBytes;
		size_t numUploadedBackgroundFileBytes;

		File *osuFile;
		File *musicFile;
		File *backgroundFile;
	};

	struct BeatmapDownloadState
	{
		uint32_t serial;

		size_t totalDownloadBytes;

		size_t numDownloadedOsuFileBytes;
		size_t numDownloadedMusicFileBytes;
		size_t numDownloadedBackgroundFileBytes;

		UString osuFileMD5Hash;
		UString musicFileName;
		UString backgroundFileName;

		std::vector<char> osuFileBytes;
		std::vector<char> musicFileBytes;
		std::vector<char> backgroundFileBytes;
	};

private:
	static unsigned long long sortHackCounter;
	static ConVar *m_cl_cmdrate;

	void onClientcastCommand(UString command);
	void onBroadcastCommand(UString command);
	void onClientCommandInt(UString string, bool executeLocallyToo);

	void onMPForceClientBeatmapDownload();
	void onMPSelectBeatmap(UString md5hash);
	void onMPRequestBeatmapDownload();

	void onBeatmapDownloadFinished(const BeatmapDownloadState &dl);

private:
	enum PACKET_TYPE
	{
		PLAYER_CHANGE_TYPE,
		PLAYER_STATE_TYPE,
		PLAYER_CMD_TYPE,
		CONVAR_TYPE,
		STATE_TYPE,
		SCORE_TYPE,
		BEATMAP_DOWNLOAD_REQUEST_TYPE,
		BEATMAP_DOWNLOAD_ACK_TYPE,
		BEATMAP_DOWNLOAD_CHUNK_TYPE
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
		bool downloadingBeatmap;// this is also only used visually
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
		wchar_t str[2048];
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

	struct BEATMAP_DOWNLOAD_REQUEST_PACKET
	{
		uint32_t dummy;

		// TODO: add support for requesting a specific beatmap download, and not just whatever the peer currently has selected
	};

	struct BEATMAP_DOWNLOAD_ACK_PACKET
	{
		uint32_t serial;

		uint32_t osuFileSizeBytes;
		uint32_t musicFileSizeBytes;
		uint32_t backgroundFileSizeBytes;

		char osuFileMD5Hash[32];
		wchar_t musicFileName[1024];
		wchar_t backgroundFileName[1024];
	};

	struct BEATMAP_DOWNLOAD_CHUNK_PACKET
	{
		uint32_t serial;

		uint8_t fileType; // 1 = .osu file, 2 = .mp3/.ogg file, 3 = .png/.jpeg file
		uint32_t numDataBytes;
		// ... (data bytes)
	};

#pragma pack()

private:
	Osu *m_osu;
	std::vector<PLAYER> m_serverPlayers;
	std::vector<PLAYER> m_clientPlayers;

	float m_fNextPlayerCmd;
	std::vector<PLAYER_INPUT_PACKET> m_playerInputBuffer;

	bool m_bMPSelectBeatmapScheduled;
	std::string m_sMPSelectBeatmapScheduledMD5Hash;

	std::vector<BeatmapUploadState> m_uploads;
	std::vector<BeatmapDownloadState> m_downloads;

	unsigned int m_iLastClientBeatmapSelectID;
};

#endif
