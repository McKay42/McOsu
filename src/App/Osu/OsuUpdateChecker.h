//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		checks if an update is available from github
//
// $NoKeywords: $osuupdchk
//===============================================================================//

#ifndef OSUUPDATECHECKER_H
#define OSUUPDATECHECKER_H

#include "Resource.h"

class OsuUpdateChecker : public Resource
{
public:
	static const char *GITHUB_API_RELEASE_URL;
	static const char *GITHUB_RELEASE_DOWNLOAD_URL;

public:
	OsuUpdateChecker();
	virtual ~OsuUpdateChecker();

	void checkForUpdates();

	bool isUpdateAvailable();

private:
	void init();
	void initAsync();
	void destroy();

	struct GITHUB_RELEASE_BUILD
	{
		int version;
		UString downloadURL;
	};
	std::vector<GITHUB_RELEASE_BUILD> m_releases;
	std::vector<GITHUB_RELEASE_BUILD> m_asyncReleases;
};

#endif
