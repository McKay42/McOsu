//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		checks if an update is available from github
//
// $NoKeywords: $osuupdchk
//===============================================================================//

#include "OsuUpdateChecker.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "NetworkHandler.h"
#include "ConVar.h"

#include "JSON.h"

#include "Osu.h"

const char *OsuUpdateChecker::GITHUB_API_RELEASE_URL = "https://api.github.com/repos/McKay42/McOsu/releases";
const char *OsuUpdateChecker::GITHUB_RELEASE_DOWNLOAD_URL = "https://github.com/McKay42/McOsu/releases";

OsuUpdateChecker::OsuUpdateChecker() : Resource()
{
}

OsuUpdateChecker::~OsuUpdateChecker()
{
}

void OsuUpdateChecker::destroy()
{
	m_releases.clear();
	m_asyncReleases.clear();
}

void OsuUpdateChecker::checkForUpdates()
{
	if (Osu::debug->getBool()) return;

	destroy();
	engine->getResourceManager()->requestNextLoadAsync();
	engine->getResourceManager()->loadResource(this);
}

bool OsuUpdateChecker::isUpdateAvailable()
{
	for (int i=0; i<m_releases.size(); i++)
	{
		if (m_releases[i].version > Osu::version->getInt())
			return true;
	}
	return false;
}

void OsuUpdateChecker::init()
{
	m_releases = m_asyncReleases;

	debugLog("OsuUpdateChecker: Found %i releases.\n", m_releases.size());
	for (int i=0; i<m_releases.size(); i++)
	{
		debugLog("OsuUpdateChecker: Release #%i: version = %i, downloadURL = %s\n", i, m_releases[i].version, m_releases[i].downloadURL.toUtf8());
	}

	m_bReady = true;
}
void OsuUpdateChecker::initAsync()
{
	UString gitReleases = engine->getNetworkHandler()->httpGet(GITHUB_API_RELEASE_URL);
	//printf("OsuUpdateChecker: result = %s\n", gitReleases.toUtf8());

	//	[array
	//		{object
	//			"tag_name" : "22",
	//			"target_commitish": "master"
	//			"assets" : [array
	//							{object
	//								"browser_download_url": "https://github.com/McKay42/McOsu/releases/download/22/McOsu.Alpha.22.zip"
	//							}
	//			]
	//		},
	//		{
	//		},
	//		{
	//		}
	//	]

	JSONValue *value = JSON::Parse(gitReleases.toUtf8());
	if (value != NULL)
	{
		JSONArray root;
		if (value->IsArray() == false)
			printf("OsuUpdateChecker: Invalid JSON array.\n");
		else
		{
			root = value->AsArray();
			for (int i=0; i<root.size(); i++)
			{
				if (root[i]->IsObject())
				{
					JSONObject release = root[i]->AsObject();

					if (release.find(L"tag_name") != release.end() && release[L"tag_name"]->IsString() && release.find(L"target_commitish") != release.end() && release[L"target_commitish"]->IsString())
					{
						// get release version and branch
						int version = UString(release[L"tag_name"]->AsString().c_str()).toInt();
						UString branch = UString(release[L"target_commitish"]->AsString().c_str());

						// we are only interested in the master branch
						if (branch == "master")
						{
							if (release.find(L"assets") != release.end() && release[L"assets"]->IsArray())
							{
								JSONArray assets = release[L"assets"]->AsArray();
								for (int a=0; a<assets.size(); a++)
								{
									if (assets[a]->IsObject())
									{
										JSONObject asset = assets[a]->AsObject();

										if (asset.find(L"browser_download_url") != asset.end() && asset[L"browser_download_url"]->IsString())
										{
											// get download URL
											UString downloadURL = UString(asset[L"browser_download_url"]->AsString().c_str());

											// we now have everything
											GITHUB_RELEASE_BUILD b;
											b.version = version;
											b.downloadURL = downloadURL;
											m_asyncReleases.push_back(b);
										}
									}
									else
										printf("OsuUpdateChecker: Invalid JSON asset object.\n");
								}
							}
						}
					}
				}
				else
					printf("OsuUpdateChecker: Invalid JSON object.\n");
			}
		}
		delete value;
	}

	m_bAsyncReady = true;
}
