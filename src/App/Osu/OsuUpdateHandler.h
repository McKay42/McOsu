//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		checks if an update is available from github
//
// $NoKeywords: $osuupdchk
//===============================================================================//

#ifndef OSUUPDATECHECKER_H
#define OSUUPDATECHECKER_H

#include "cbase.h"
#include <pthread.h>

class OsuUpdateHandler
{
public:
	static const char *GITHUB_API_RELEASE_URL;
	static const char *GITHUB_RELEASE_DOWNLOAD_URL;

	enum class STATUS
	{
		STATUS_UP_TO_DATE,
		STATUS_CHECKING_FOR_UPDATE,
		STATUS_DOWNLOADING_UPDATE,
		STATUS_INSTALLING_UPDATE,
		STATUS_SUCCESS_INSTALLATION,
		STATUS_ERROR
	};

public:
	static void *run(void *data);

public:
	OsuUpdateHandler();
	virtual ~OsuUpdateHandler();
	void stop(); // tells the update thread to stop at the next cancellation point
	void wait(); // blocks until the update thread is finished

	void checkForUpdates();

	inline STATUS getStatus() const {return m_status;}
	bool isUpdateAvailable();

private:
	static const char *TEMP_UPDATE_DOWNLOAD_FILEPATH;
	static ConVar *m_osu_release_stream_ref;

	// async
	void _requestUpdate();
	bool _downloadUpdate(UString url);
	void _installUpdate(UString zipFilePath);

	pthread_t m_updateThread;
	bool _m_bKYS;

	// releases
	enum class STREAM
	{
		STREAM_NULL,
		STREAM_DESKTOP,
		STREAM_VR
	};

	STREAM stringToStream(UString streamString);
	Environment::OS stringToOS(UString osString);
	STREAM getReleaseStream();

	struct GITHUB_RELEASE_BUILD
	{
		Environment::OS os;
		STREAM stream;
		float version;
		UString downloadURL;
	};
	std::vector<GITHUB_RELEASE_BUILD> m_releases;

	// status
	STATUS m_status;
	int m_iNumRetries;
};

#endif
