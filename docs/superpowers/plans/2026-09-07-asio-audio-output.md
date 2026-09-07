# ASIO Audio Output Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let McOsu (Windows, 32-bit) output audio through an ASIO driver via BASSASIO, selectable at runtime next to the existing WASAPI exclusive devices, so audio latency drops to driver level.

**Architecture:** The engine's WASAPI build already routes every sound as a BASS decode stream into one bassmix mixer that a device callback drains. ASIO reuses that mixer: `SoundEngine` learns a per-device `DRIVER` (BASS / WASAPI / ASIO), enumerates ASIO devices into the same output-device list as `"[ASIO] name"`, and initialises them with `BASS_ASIO_Init → ChannelEnable(callback) → Start`. McOsu adds an ASIO options subsection and a banner line. `Sound.cpp` is untouched.

**Tech Stack:** C++11, BASS 2.4.15 + bass_fx + bassmix 2.4.8 + basswasapi 2.4.2 + **BASSASIO 1.4.3** (32-bit), McEngine ConVar system, McOsu CBaseUI widgets. Verification uses a self-contained llvm-mingw cross toolchain living in the session scratchpad.

**Spec:** `docs/superpowers/specs/2026-09-07-asio-audio-output-design.md` (in McOsu)

## Global Constraints

- Two repos: engine changes in `~/McEngine` (git root; project dir `~/McEngine/McEngine`), branch `asio`, remote `origin` = `git@github.com:surreal420/McEngine.git`, `upstream` = McKay42. App changes in `~/McOsu`, branch `asio`.
- `MCENGINE_FEATURE_BASS_ASIO` requires `MCENGINE_FEATURE_BASS_WASAPI` (`#error` otherwise). ASIO-only builds are out of scope.
- Windows binaries are 32-bit: use `bassasio14.zip` root `bassasio.dll` and `c/bassasio.lib` (both i386). SDK is extracted at `$SP/bassasio` (see below).
- Minimal diff to McEngine: do not rename existing members/globals (`g_wasapiOutputMixer` is reused for ASIO); all ASIO code sits in `#ifdef MCENGINE_FEATURE_BASS_ASIO` blocks nested inside existing WASAPI sections.
- ASIO failures log `BASS_ASIO_ErrorGetCode()`, never `BASS_ErrorGetCode()`.
- Nothing is installed system-wide. Scratchpad: `SP=/tmp/claude-1000/-home-dog-McOsu/536c46db-cc8b-4956-acb7-9ac719de5056/scratchpad`. Cross-compile check tool: `$SP/ccheck.sh <file.cpp> [-DMCENGINE_FEATURE_BASS_WASAPI] [-DMCENGINE_FEATURE_BASS_ASIO]` (exit 0 = compiles). It already carries all include paths for both repos, the BASSASIO SDK and the toolchain case-insensitivity shims.
- Every code task is verified in three configurations: **default** (no extra defines), **wasapi** (`-DMCENGINE_FEATURE_BASS_WASAPI`), **asio** (`-DMCENGINE_FEATURE_BASS_WASAPI -DMCENGINE_FEATURE_BASS_ASIO`).
- Commit messages end with:
  ```
  Co-Authored-By: Claude Fable 5.1 <noreply@anthropic.com>
  Claude-Session: https://claude.ai/code/session_01CYMGWbFGtxqvLVAkvpNt1R
  ```
- There is no unit-test framework in either repo and ASIO needs real Windows hardware; the test cycle for each task is "cross-compile the touched files in all three configurations" plus reading the diff against the spec.

---

### Task 1: McEngine — BASSASIO library, feature flag and build wiring

**Files:**
- Modify: `~/McEngine/McEngine/src/Engine/Main/EngineFeatures.h:46-49`
- Create: `~/McEngine/McEngine/libraries/bassasio/include/bassasio.h`, `libraries/bassasio/lib/windows/bassasio.lib`, `libraries/bassasio/bin/bassasio.dll`, `build/bassasio.dll`
- Modify: `~/McEngine/McEngine/.cproject` (lines 48, 103, 131, 195 area), `build.bat` (LDLIBS), `build_unity.bat` (LDLIBS)

**Interfaces:**
- Produces: macro `MCENGINE_FEATURE_BASS_ASIO` (commented out by default, like WASAPI); `<bassasio.h>` resolvable from `libraries/*/include`.

- [ ] **Step 1: Add the feature flag with the dependency guard**

In `EngineFeatures.h`, directly after the WASAPI block (`//#define MCENGINE_FEATURE_BASS_WASAPI`), insert:

```c
/*
 * BASS ASIO sound (Windows only, requires MCENGINE_FEATURE_BASS_WASAPI)
 */
//#define MCENGINE_FEATURE_BASS_ASIO

#if defined(MCENGINE_FEATURE_BASS_ASIO) && !defined(MCENGINE_FEATURE_BASS_WASAPI)
#error "MCENGINE_FEATURE_BASS_ASIO requires MCENGINE_FEATURE_BASS_WASAPI"
#endif
```

- [ ] **Step 2: Copy the SDK files into the McEngine library layout**

```bash
SP=/tmp/claude-1000/-home-dog-McOsu/536c46db-cc8b-4956-acb7-9ac719de5056/scratchpad
ME=~/McEngine/McEngine
mkdir -p $ME/libraries/bassasio/include $ME/libraries/bassasio/lib/windows $ME/libraries/bassasio/bin
cp $SP/bassasio/c/bassasio.h   $ME/libraries/bassasio/include/
cp $SP/bassasio/c/bassasio.lib $ME/libraries/bassasio/lib/windows/
cp $SP/bassasio/bassasio.dll   $ME/libraries/bassasio/bin/
cp $SP/bassasio/bassasio.dll   $ME/build/
file $ME/build/bassasio.dll    # must say: PE32 ... Intel 80386
```

- [ ] **Step 3: Wire the Eclipse project and the batch builds**

`.cproject`: after every line containing `libraries/basswasapi/include` add the same line with `basswasapi` → `bassasio`; after the line `value="basswasapi"/>` add `value="bassasio"/>`; after the line containing `libraries/basswasapi/lib/windows` add the same with `bassasio`.

```bash
cd ~/McEngine/McEngine
sed -i -e '/libraries\/basswasapi\/include/{p;s/basswasapi/bassasio/}' \
       -e '/value="basswasapi"\/>/{p;s/basswasapi/bassasio/}' \
       -e '/libraries\/basswasapi\/lib\/windows/{p;s/basswasapi/bassasio/}' .cproject
grep -c bassasio .cproject     # expected: 4
```

`build.bat` and `build_unity.bat`: in the `set LDLIBS=` line replace `-lbass -lbass_fx` with `-lbass -lbass_fx -lbasswasapi -lbassmix -lbassasio` (the batch builds never linked the WASAPI add-ons either; adding import libs of unreferenced DLLs adds no runtime dependency).

```bash
sed -i 's/-lbass -lbass_fx /-lbass -lbass_fx -lbasswasapi -lbassmix -lbassasio /' build.bat build_unity.bat
grep -c bassasio build.bat build_unity.bat   # expected: 1 each
```

- [ ] **Step 4: Verify the guard and the header resolve**

```bash
SP=/tmp/claude-1000/-home-dog-McOsu/536c46db-cc8b-4956-acb7-9ac719de5056/scratchpad
printf '#include "cbase.h"\n#include <bassasio.h>\nint x = BASSASIOVERSION;\n' > $SP/ccheck-out/t1.cpp
$SP/ccheck.sh $SP/ccheck-out/t1.cpp -DMCENGINE_FEATURE_BASS_WASAPI -DMCENGINE_FEATURE_BASS_ASIO && echo OK
$SP/ccheck.sh $SP/ccheck-out/t1.cpp -DMCENGINE_FEATURE_BASS_ASIO; echo "exit=$? (expected non-zero: #error)"
```

- [ ] **Step 5: Commit (McEngine)**

```bash
cd ~/McEngine && git add -A McEngine/libraries/bassasio McEngine/build/bassasio.dll McEngine/src/Engine/Main/EngineFeatures.h McEngine/.cproject McEngine/build.bat McEngine/build_unity.bat
git commit -m "Add BASSASIO library and MCENGINE_FEATURE_BASS_ASIO feature flag"
```

---

### Task 2: McEngine — driver-aware output device list (no ASIO behaviour yet)

**Files:**
- Modify: `~/McEngine/McEngine/src/Engine/SoundEngine.h` (struct `OUTPUT_DEVICE`, private members, `initializeOutputDevice` signature)
- Modify: `~/McEngine/McEngine/src/Engine/SoundEngine.cpp` (constructor, `updateOutputDevices`, `initializeOutputDevice`, `setOutputDevice`, `setOutputDeviceForce`)

**Interfaces:**
- Produces: `OUTPUT_DEVICE::DRIVER { BASS, WASAPI, ASIO }`, `OUTPUT_DEVICE::driver`, `SoundEngine::m_currentOutputDriver`, `bool initializeOutputDevice(int id, OUTPUT_DEVICE::DRIVER driver)`, `static OUTPUT_DEVICE::DRIVER getDefaultDriver()`.
- Consumed by Task 3.

- [ ] **Step 1: Extend the header**

Replace the `OUTPUT_DEVICE` struct and the `initializeOutputDevice` declaration in `SoundEngine.h`:

```cpp
	struct OUTPUT_DEVICE
	{
		enum class DRIVER
		{
			BASS,	// BASS plays directly (default builds, incl. dsound fallback)
			WASAPI,	// MCENGINE_FEATURE_BASS_WASAPI: mixer -> basswasapi
			ASIO	// MCENGINE_FEATURE_BASS_ASIO: mixer -> bassasio
		};

		int id;
		bool enabled;
		bool isDefault;
		UString name;
		DRIVER driver;
	};

	static OUTPUT_DEVICE::DRIVER getDefaultDriver();

	void updateOutputDevices(bool handleOutputDeviceChanges, bool printInfo);
	bool initializeOutputDevice(int id, OUTPUT_DEVICE::DRIVER driver);
```

and add next to `int m_iCurrentOutputDevice;`:

```cpp
	OUTPUT_DEVICE::DRIVER m_currentOutputDriver;
```

- [ ] **Step 2: Implement `getDefaultDriver()` and set the driver on every entry**

In `SoundEngine.cpp`, above the constructor:

```cpp
SoundEngine::OUTPUT_DEVICE::DRIVER SoundEngine::getDefaultDriver()
{
#ifdef MCENGINE_FEATURE_BASS_WASAPI
	return OUTPUT_DEVICE::DRIVER::WASAPI;
#else
	return OUTPUT_DEVICE::DRIVER::BASS;
#endif
}
```

Constructor (both the BASS and the SDL branches): after `m_iCurrentOutputDevice = -1;` add `m_currentOutputDriver = getDefaultDriver();`, and when building `defaultOutputDevice` add `defaultOutputDevice.driver = getDefaultDriver();`. Replace `initializeOutputDevice(defaultOutputDevice.id);` with `initializeOutputDevice(defaultOutputDevice.id, defaultOutputDevice.driver);` (both branches).

`updateOutputDevices`: where `soundDevice` is filled (`soundDevice.isDefault = isDefault;`) add `soundDevice.driver = getDefaultDriver();`.

`initializeOutputDevice(int id, OUTPUT_DEVICE::DRIVER driver)`: after `m_iCurrentOutputDevice = id;` add `m_currentOutputDriver = driver;`. Inside the dsound-fallback retry, replace `initializeOutputDevice(id)` with `initializeOutputDevice(id, driver)`. At the end where `m_sCurrentOutputDevice` is looked up, match `m_outputDevices[i].id == id && m_outputDevices[i].driver == driver`.

`setOutputDevice` / `setOutputDeviceForce`: replace

```cpp
				int previousOutputDevice = m_iCurrentOutputDevice;

				if (!initializeOutputDevice(m_outputDevices[i].id))
					initializeOutputDevice(previousOutputDevice);
```

with

```cpp
				const int previousOutputDevice = m_iCurrentOutputDevice;
				const OUTPUT_DEVICE::DRIVER previousOutputDriver = m_currentOutputDriver;

				if (!initializeOutputDevice(m_outputDevices[i].id, m_outputDevices[i].driver))
					initializeOutputDevice(previousOutputDevice, previousOutputDriver); // if something went wrong, automatically switch back to the previous device
```

and the "already current" check in `setOutputDevice` becomes `if (m_outputDevices[i].id != m_iCurrentOutputDevice || m_outputDevices[i].driver != m_currentOutputDriver)`. The SDL branches call `initializeOutputDevice(-1, OUTPUT_DEVICE::DRIVER::BASS);`.

- [ ] **Step 3: Cross-compile in all three configurations**

```bash
SP=/tmp/claude-1000/-home-dog-McOsu/536c46db-cc8b-4956-acb7-9ac719de5056/scratchpad
F=~/McEngine/McEngine/src/Engine/SoundEngine.cpp
$SP/ccheck.sh $F && $SP/ccheck.sh $F -DMCENGINE_FEATURE_BASS_WASAPI && $SP/ccheck.sh $F -DMCENGINE_FEATURE_BASS_WASAPI -DMCENGINE_FEATURE_BASS_ASIO && echo ALL OK
```

Expected: `ALL OK`.

- [ ] **Step 4: Commit (McEngine)**

```bash
cd ~/McEngine && git add McEngine/src/Engine/SoundEngine.h McEngine/src/Engine/SoundEngine.cpp
git commit -m "SoundEngine: track output driver per device"
```

---

### Task 3: McEngine — ASIO device enumeration, initialisation, volume, reset handling

**Files:**
- Modify: `~/McEngine/McEngine/src/Engine/SoundEngine.h` (public accessors, private helper + members)
- Modify: `~/McEngine/McEngine/src/Engine/SoundEngine.cpp` (includes, cvars, callbacks, `updateOutputDevices`, `initializeOutputDevice`, new `initializeASIOOutputDevice`, `~SoundEngine`, `update`, `setVolume`, accessors)

**Interfaces:**
- Consumes: Task 2's `OUTPUT_DEVICE::DRIVER`, `m_currentOutputDriver`.
- Produces (public, always declared): `bool isASIO() const`, `float getASIOOutputLatency() const` (seconds), `int getASIOBufferLength() const` (samples), `void openASIOControlPanel()`; cvars `win_snd_asio_buffer_size` (float seconds, default 0) and `win_snd_asio_control_panel` (concommand). Used by Tasks 4 and 5.

- [ ] **Step 1: Header additions**

Public section of `SoundEngine.h` (after `getVolume()`):

```cpp
	// asio (always declared; no-ops unless MCENGINE_FEATURE_BASS_ASIO)
	bool isASIO() const;
	inline float getASIOOutputLatency() const {return m_fASIOOutputLatency;}	// seconds, driver-reported output latency
	inline int getASIOBufferLength() const {return m_iASIOBufferLength;}		// samples, effective buffer length
	void openASIOControlPanel();
```

Private section (next to `m_currentOutputDriver`):

```cpp
#ifdef MCENGINE_FEATURE_BASS_ASIO

	bool initializeASIOOutputDevice(int id);

#endif

	float m_fASIOOutputLatency;
	int m_iASIOBufferLength;
```

- [ ] **Step 2: Includes, globals, cvars and callbacks in `SoundEngine.cpp`**

Directly after the existing `#ifdef MCENGINE_FEATURE_BASS_WASAPI ... OutputWasapiProc ... #endif` block add:

```cpp
#ifdef MCENGINE_FEATURE_BASS_ASIO

#include <bassasio.h>
#include <atomic>

// NOTE: g_wasapiOutputMixer is shared: ASIO output pulls from the same bassmix mixer as WASAPI output
std::atomic<bool> g_asioResetRequested(false);

DWORD CALLBACK OutputAsioProc(BOOL input, DWORD channel, void *buffer, DWORD length, void *user)
{
	if (g_wasapiOutputMixer != 0)
	{
		const int c = BASS_ChannelGetData(g_wasapiOutputMixer, buffer, length);

		if (c < 0)
			return 0;

		return c;
	}

	return 0;
}

void CALLBACK AsioNotifyProc(DWORD notify, void *user)
{
	// called on the driver thread, e.g. after the user changed the buffer size in the driver's control panel
	if (notify == BASS_ASIO_NOTIFY_RESET)
		g_asioResetRequested = true;
}

#endif
```

After the WASAPI cvar block (`#ifdef MCENGINE_FEATURE_BASS_WASAPI ... _WIN_SND_WASAPI_EXCLUSIVE_CHANGE ... #endif`) add:

```cpp
#ifdef MCENGINE_FEATURE_BASS_ASIO

void _WIN_SND_ASIO_BUFFER_SIZE_CHANGE(UString oldValue, UString newValue);
void _WIN_SND_ASIO_CONTROL_PANEL(void);

ConVar win_snd_asio_buffer_size("win_snd_asio_buffer_size", 0.0f, FCVAR_NONE, "ASIO buffer length in seconds (e.g. 0.005 = 5 ms), 0 = driver default/preferred length, clamped to the driver's supported range", _WIN_SND_ASIO_BUFFER_SIZE_CHANGE);
ConVar win_snd_asio_control_panel("win_snd_asio_control_panel", FCVAR_NONE, "open the control panel of the current ASIO driver", _WIN_SND_ASIO_CONTROL_PANEL);

void _WIN_SND_ASIO_BUFFER_SIZE_CHANGE(UString oldValue, UString newValue)
{
	const int oldValueMS = std::round(oldValue.toFloat()*1000.0f);
	const int newValueMS = std::round(newValue.toFloat()*1000.0f);

	if (oldValueMS != newValueMS && engine->getSound()->isASIO())
		engine->getSound()->setOutputDeviceForce(engine->getSound()->getOutputDevice()); // force restart
}

void _WIN_SND_ASIO_CONTROL_PANEL(void)
{
	engine->getSound()->openASIOControlPanel();
}

#endif
```

- [ ] **Step 3: Constructor init and device enumeration**

Constructor: next to `m_fVolume = 1.0f;` add

```cpp
	m_fASIOOutputLatency = 0.0f;
	m_iASIOBufferLength = 0;
```

`updateOutputDevices`: right before the closing `#endif` of the function's `#ifdef MCENGINE_FEATURE_SOUND` block (after the WASAPI/BASS device loop ends) add:

```cpp
#ifdef MCENGINE_FEATURE_BASS_ASIO

	// asio devices are appended after the wasapi devices, prefixed so users can tell them apart in the same list
	BASS_ASIO_DEVICEINFO asioDeviceInfo;
	for (int d=0; (BASS_ASIO_GetDeviceInfo(d, &asioDeviceInfo) == true); d++)
	{
		if (printInfo)
			debugLog("SoundEngine: ASIO Device %i = \"%s\", driver = \"%s\"\n", d, asioDeviceInfo.name, asioDeviceInfo.driver);

		// only add new devices
		bool alreadyKnown = false;
		for (size_t i=0; i<m_outputDevices.size(); i++)
		{
			if (m_outputDevices[i].driver == OUTPUT_DEVICE::DRIVER::ASIO && m_outputDevices[i].id == d)
			{
				alreadyKnown = true;
				break;
			}
		}
		if (alreadyKnown)
			continue;

		UString originalDeviceName = "[ASIO] ";
		originalDeviceName.append(asioDeviceInfo.name);

		OUTPUT_DEVICE soundDevice;
		soundDevice.id = d;
		soundDevice.name = originalDeviceName;
		soundDevice.enabled = true;
		soundDevice.isDefault = false;
		soundDevice.driver = OUTPUT_DEVICE::DRIVER::ASIO;

		// avoid duplicate names
		int duplicateNameCounter = 2;
		while (true)
		{
			bool foundDuplicateName = false;
			for (size_t i=0; i<m_outputDevices.size(); i++)
			{
				if (m_outputDevices[i].name == soundDevice.name)
				{
					foundDuplicateName = true;

					soundDevice.name = originalDeviceName;
					soundDevice.name.append(UString::format(" (%i)", duplicateNameCounter));

					duplicateNameCounter++;

					break;
				}
			}

			if (!foundDuplicateName)
				break;
		}

		m_outputDevices.push_back(soundDevice);

		// sanity
		if (d > sanityLimit)
		{
			debugLog("WARNING: SoundEngine::updateOutputDevices() found too many ASIO devices ...\n");
			break;
		}
	}

#endif
```

- [ ] **Step 4: Free + dispatch in `initializeOutputDevice`, and the ASIO helper**

In `initializeOutputDevice`, after `BASS_WASAPI_Free();` (inside its `#ifdef MCENGINE_FEATURE_BASS_WASAPI`) add:

```cpp
#ifdef MCENGINE_FEATURE_BASS_ASIO

	if (BASS_ASIO_IsStarted())
		BASS_ASIO_Stop();

	BASS_ASIO_Free(); // harmless if nothing was initialized

	m_fASIOOutputLatency = 0.0f;
	m_iASIOBufferLength = 0;

#endif
```

Directly after the `BASS_Init(...)` failure handling (i.e. right before the line `#ifdef MCENGINE_FEATURE_BASS_WASAPI` that begins `const float bufferSize = ...`) add:

```cpp
#ifdef MCENGINE_FEATURE_BASS_ASIO

	if (driver == OUTPUT_DEVICE::DRIVER::ASIO)
		return initializeASIOOutputDevice(id);

#endif
```

Add the helper after `initializeOutputDevice`:

```cpp
#ifdef MCENGINE_FEATURE_BASS_ASIO

bool SoundEngine::initializeASIOOutputDevice(int id)
{
	// NOTE: BASS itself was already initialized with the "no sound" device by initializeOutputDevice(), exactly like the wasapi path

	// BASS_ASIO_THREAD hosts the driver in its own thread, which lets it be freed from any thread and keeps driver windows responsive
	if (!BASS_ASIO_Init(id, BASS_ASIO_THREAD))
	{
		m_bReady = false;
		engine->showMessageError("Sound Error", UString::format("BASS_ASIO_Init() failed (%i)!", (int)BASS_ASIO_ErrorGetCode()));
		return false;
	}

	BASS_ASIO_SetNotify(AsioNotifyProc, NULL);

	// try to run the device at our sample rate, but never fail because of it (the mixer simply gets created at the driver's rate)
	const double requestedRate = (double)snd_freq.getInt();
	if (!BASS_ASIO_SetRate(requestedRate))
		debugLog("SoundEngine: BASS_ASIO_SetRate(%f) failed (%i), keeping driver rate\n", requestedRate, (int)BASS_ASIO_ErrorGetCode());

	const double rate = BASS_ASIO_GetRate();

	BASS_ASIO_INFO asioInfo;
	memset(&asioInfo, 0, sizeof(asioInfo));
	if (!BASS_ASIO_GetInfo(&asioInfo) || rate <= 0.0)
	{
		m_bReady = false;
		engine->showMessageError("Sound Error", UString::format("BASS_ASIO_GetInfo() failed (%i)!", (int)BASS_ASIO_ErrorGetCode()));
		return false;
	}

	const int numOutputChannels = (asioInfo.outputs >= 2 ? 2 : (int)asioInfo.outputs);
	if (numOutputChannels < 1)
	{
		m_bReady = false;
		engine->showMessageError("Sound Error", "ASIO device has no output channels!");
		return false;
	}

	g_wasapiOutputMixer = BASS_Mixer_StreamCreate((DWORD)rate, numOutputChannels, BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | BASS_MIXER_NONSTOP);
	if (g_wasapiOutputMixer == 0)
	{
		m_bReady = false;
		engine->showMessageError("Sound Error", UString::format("BASS_Mixer_StreamCreate() failed (%i)!", BASS_ErrorGetCode()));
		return false;
	}

	// output channel 0 (+ joined channel 1 for stereo) is fed by OutputAsioProc with interleaved float samples from the mixer
	if (!BASS_ASIO_ChannelEnable(FALSE, 0, OutputAsioProc, NULL)
		|| (numOutputChannels == 2 && !BASS_ASIO_ChannelJoin(FALSE, 1, 0))
		|| !BASS_ASIO_ChannelSetFormat(FALSE, 0, BASS_ASIO_FORMAT_FLOAT))
	{
		m_bReady = false;
		engine->showMessageError("Sound Error", UString::format("BASS_ASIO_ChannelEnable/Join/SetFormat() failed (%i)!", (int)BASS_ASIO_ErrorGetCode()));
		return false;
	}

	// buffer length: 0 = driver preferred, otherwise seconds -> samples, clamped and snapped to what the driver supports
	DWORD bufferLength = 0;
	{
		const float requestedSeconds = win_snd_asio_buffer_size.getFloat();
		if (requestedSeconds > 0.0f)
		{
			DWORD requested = (DWORD)std::round((double)requestedSeconds * rate);

			if (asioInfo.bufmin > 0 && requested < asioInfo.bufmin)
				requested = asioInfo.bufmin;
			if (asioInfo.bufmax > 0 && requested > asioInfo.bufmax)
				requested = asioInfo.bufmax;

			if (asioInfo.bufgran == -1)
			{
				// powers of 2 only: round to nearest
				DWORD pow2 = 1;
				while (pow2*2 <= requested)
					pow2 *= 2;
				requested = ((requested - pow2) < (pow2*2 - requested) ? pow2 : pow2*2);
				if (asioInfo.bufmax > 0 && requested > asioInfo.bufmax)
					requested = pow2;
			}
			else if (asioInfo.bufgran == 0)
			{
				// only the preferred length is available
				requested = asioInfo.bufpref;
			}
			else if (asioInfo.bufgran > 1 && asioInfo.bufmin > 0)
			{
				// snap to multiples of the granularity, starting from bufmin
				const DWORD steps = (requested - asioInfo.bufmin + (DWORD)asioInfo.bufgran/2) / (DWORD)asioInfo.bufgran;
				requested = asioInfo.bufmin + steps*(DWORD)asioInfo.bufgran;
				if (asioInfo.bufmax > 0 && requested > asioInfo.bufmax)
					requested = asioInfo.bufmax;
			}

			bufferLength = requested;
		}
	}

	debugLog("SoundEngine: ASIO driver = \"%s\", rate = %f, outputs = %i, buffer min/max/pref/gran = %i/%i/%i/%i, requested = %i\n", asioInfo.name, rate, (int)asioInfo.outputs, (int)asioInfo.bufmin, (int)asioInfo.bufmax, (int)asioInfo.bufpref, (int)asioInfo.bufgran, (int)bufferLength);

	if (!BASS_ASIO_Start(bufferLength, 0))
	{
		m_bReady = false;
		engine->showMessageError("Sound Error", UString::format("BASS_ASIO_Start() failed (%i)!", (int)BASS_ASIO_ErrorGetCode()));
		return false;
	}

	m_iASIOBufferLength = (int)(bufferLength > 0 ? bufferLength : asioInfo.bufpref);
	m_fASIOOutputLatency = (float)((double)BASS_ASIO_GetLatency(FALSE) / rate);

	debugLog("SoundEngine: ASIO started, buffer = %i samples (%.2f ms), reported output latency = %.2f ms\n", m_iASIOBufferLength, (double)m_iASIOBufferLength / rate * 1000.0, (double)m_fASIOOutputLatency * 1000.0);

	BASS_ASIO_ChannelSetVolume(FALSE, -1, m_fVolume);

	m_bReady = true;

	for (size_t i=0; i<m_outputDevices.size(); i++)
	{
		if (m_outputDevices[i].id == id && m_outputDevices[i].driver == OUTPUT_DEVICE::DRIVER::ASIO)
		{
			m_sCurrentOutputDevice = m_outputDevices[i].name;
			break;
		}
	}
	debugLog("SoundEngine: Output Device = \"%s\"\n", m_sCurrentOutputDevice.toUtf8());

	return true;
}

#endif
```

`SoundEngine.cpp` needs `#include <cstring>` (for `memset`) and `<cmath>` (for `std::round`, already used by the WASAPI cvar callbacks) near the top of the file if not already present through `cbase.h`.

- [ ] **Step 5: Destructor, `update()`, `setVolume()`, accessors**

`~SoundEngine()`: inside `if (m_bReady) { BASS_Free(); ... }` add before `BASS_Free();`:

```cpp
#ifdef MCENGINE_FEATURE_BASS_ASIO

		// asio drivers are COM objects: release them before the dll unloads
		if (m_currentOutputDriver == OUTPUT_DEVICE::DRIVER::ASIO)
		{
			if (BASS_ASIO_IsStarted())
				BASS_ASIO_Stop();

			BASS_ASIO_Free();
		}

#endif
```

`update()` body (keep the commented block, add before it):

```cpp
#ifdef MCENGINE_FEATURE_BASS_ASIO

	// the driver asked for a reset (e.g. buffer size changed in its control panel); do it on the main thread
	if (g_asioResetRequested.exchange(false))
	{
		debugLog("SoundEngine: ASIO driver requested a reset, restarting ...\n");
		restart();
	}

#endif
```

`setVolume()`: replace the WASAPI block with

```cpp
#ifdef MCENGINE_FEATURE_BASS_WASAPI

#ifdef MCENGINE_FEATURE_BASS_ASIO

	if (m_currentOutputDriver == OUTPUT_DEVICE::DRIVER::ASIO)
		BASS_ASIO_ChannelSetVolume(FALSE, -1, m_fVolume); // all output channels
	else

#endif

	BASS_WASAPI_SetVolume(BASS_WASAPI_CURVE_WINDOWS | (!win_snd_wasapi_exclusive.getBool() && !win_snd_wasapi_shared_volume_affects_device.getBool() ? BASS_WASAPI_VOL_SESSION : 0), m_fVolume);

#endif
```

Accessors (after `getOutputDevices()`):

```cpp
bool SoundEngine::isASIO() const
{
#ifdef MCENGINE_FEATURE_BASS_ASIO

	return (m_bReady && m_currentOutputDriver == OUTPUT_DEVICE::DRIVER::ASIO);

#else

	return false;

#endif
}

void SoundEngine::openASIOControlPanel()
{
#ifdef MCENGINE_FEATURE_BASS_ASIO

	if (!isASIO())
	{
		debugLog("SoundEngine::openASIOControlPanel() no ASIO device is active\n");
		return;
	}

	if (!BASS_ASIO_ControlPanel())
		debugLog("SoundEngine::openASIOControlPanel() BASS_ASIO_ControlPanel() failed (%i)\n", (int)BASS_ASIO_ErrorGetCode());

#endif
}
```

- [ ] **Step 6: Cross-compile in all three configurations**

```bash
SP=/tmp/claude-1000/-home-dog-McOsu/536c46db-cc8b-4956-acb7-9ac719de5056/scratchpad
F=~/McEngine/McEngine/src/Engine/SoundEngine.cpp
$SP/ccheck.sh $F && $SP/ccheck.sh $F -DMCENGINE_FEATURE_BASS_WASAPI && $SP/ccheck.sh $F -DMCENGINE_FEATURE_BASS_WASAPI -DMCENGINE_FEATURE_BASS_ASIO && echo ALL OK
```

Expected: `ALL OK`. Then read `git diff` once against the spec's `initializeASIOOutputDevice` step list (10 steps) and the error-handling table.

- [ ] **Step 7: Commit (McEngine)**

```bash
cd ~/McEngine && git add McEngine/src/Engine/SoundEngine.h McEngine/src/Engine/SoundEngine.cpp
git commit -m "SoundEngine: add ASIO output via BASSASIO (runtime-selectable next to WASAPI)"
```

---

### Task 4: McOsu — ASIO options subsection

**Files:**
- Modify: `~/McOsu/src/App/Osu/OsuOptionsMenu.h:172-173` (handlers), `:253-256` (widgets), `:292-293` (cvar refs), `:307-308` (scheduled flags)
- Modify: `~/McOsu/src/App/Osu/OsuOptionsMenu.cpp:454-455` (cvar refs), `:486-489` (widget init), `:510-511` (flag init), `:779-803` (WASAPI subsection → add ASIO subsection after it), `:1576-1600` (deferred apply in `update()`), `:3494-3545` (slider handlers)

**Interfaces:**
- Consumes: `engine->getSound()->openASIOControlPanel()`, cvar `win_snd_asio_buffer_size` (Task 3).
- Produces: `onASIOBufferChange(CBaseUISlider*)`, `onASIOControlPanelClicked()`.

- [ ] **Step 1: Header**

After `void onWASAPIPeriodChange(CBaseUISlider *slider);` add:

```cpp
	void onASIOBufferChange(CBaseUISlider *slider);
	void onASIOControlPanelClicked();
```

After `OsuOptionsMenuResetButton *m_wasapiPeriodSizeResetButton;` add:

```cpp
	CBaseUISlider *m_asioBufferSizeSlider;
	OsuOptionsMenuResetButton *m_asioBufferSizeResetButton;
```

After `ConVar *m_win_snd_wasapi_period_size_ref;` add `ConVar *m_win_snd_asio_buffer_size_ref;`. After `bool m_bWASAPIPeriodChangeScheduled;` add `bool m_bASIOBufferChangeScheduled;`.

- [ ] **Step 2: Constructor initialisation**

After `m_win_snd_wasapi_period_size_ref = convar->getConVarByName("win_snd_wasapi_period_size", false);` add
`m_win_snd_asio_buffer_size_ref = convar->getConVarByName("win_snd_asio_buffer_size", false);`.
After `m_wasapiPeriodSizeResetButton = NULL;` add `m_asioBufferSizeSlider = NULL;` and `m_asioBufferSizeResetButton = NULL;`.
After `m_bWASAPIPeriodChangeScheduled = false;` add `m_bASIOBufferChangeScheduled = false;`.

- [ ] **Step 3: The subsection**

Directly after the WASAPI block's `addLabel("");` and before its `#endif`, insert:

```cpp
#ifdef MCENGINE_FEATURE_BASS_ASIO

	addSubSection("ASIO");
	addLabel("Pick an \"[ASIO] ...\" device in \"Select Output Device\" above.")->setTextColor(0xff666666);
	m_asioBufferSizeSlider = addSlider("Buffer Size:", 0.000f, 0.050f, convar->getConVarByName("win_snd_asio_buffer_size"));
	m_asioBufferSizeSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onASIOBufferChange) );
	m_asioBufferSizeSlider->setKeyDelta(0.001f);
	m_asioBufferSizeSlider->setAnimated(false);
	addLabel("0 = driver default. Values outside the driver's range are clamped.")->setTextColor(0xff666666);
	addLabel("Most drivers only change the buffer in their own panel:")->setTextColor(0xff666666);
	OsuUIButton *asioControlPanel = addButton("Open ASIO Control Panel");
	asioControlPanel->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onASIOControlPanelClicked) );
	asioControlPanel->setColor(0xff00566b);
	OsuUIButton *restartSoundEngineASIO = addButton("Restart SoundEngine");
	restartSoundEngineASIO->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onOutputDeviceRestart) );
	restartSoundEngineASIO->setColor(0xff00566b);
	addLabel("");

#endif
```

- [ ] **Step 4: Deferred apply in `update()`**

After the `if (m_bWASAPIPeriodChangeScheduled) {...}` block add:

```cpp
	if (m_bASIOBufferChangeScheduled)
	{
		if (!m_asioBufferSizeSlider->isActive())
		{
			m_bASIOBufferChangeScheduled = false;

			m_win_snd_asio_buffer_size_ref->setValue(m_asioBufferSizeSlider->getFloat());

			// and update reset buttons as usual
			onResetUpdate(m_asioBufferSizeResetButton);
		}
	}
```

- [ ] **Step 5: Handlers (after `onWASAPIPeriodChange`)**

```cpp
void OsuOptionsMenu::onASIOBufferChange(CBaseUISlider *slider)
{
	m_bASIOBufferChangeScheduled = true;

	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				if (m_elements[i].elements.size() == 3)
				{
					CBaseUILabel *labelPointer = dynamic_cast<CBaseUILabel*>(m_elements[i].elements[2]);
					const int ms = (int)std::round(slider->getFloat()*1000.0f);
					labelPointer->setText(ms > 0 ? UString::format("%i ms", ms) : UString("driver default"));
				}

				m_asioBufferSizeResetButton = m_elements[i].resetButton; // HACKHACK: disgusting

				break;
			}
		}
	}
}

void OsuOptionsMenu::onASIOControlPanelClicked()
{
	if (!engine->getSound()->isASIO())
	{
		m_osu->getNotificationOverlay()->addNotification("Select an [ASIO] output device first.", 0xffffff00);
		return;
	}

	engine->getSound()->openASIOControlPanel();
}
```

(`m_osu->getNotificationOverlay()` and `OsuNotificationOverlay.h` are already used/included in this file.)

- [ ] **Step 6: Cross-compile in all three configurations**

```bash
SP=/tmp/claude-1000/-home-dog-McOsu/536c46db-cc8b-4956-acb7-9ac719de5056/scratchpad
F=~/McOsu/src/App/Osu/OsuOptionsMenu.cpp
$SP/ccheck.sh $F && $SP/ccheck.sh $F -DMCENGINE_FEATURE_BASS_WASAPI && $SP/ccheck.sh $F -DMCENGINE_FEATURE_BASS_WASAPI -DMCENGINE_FEATURE_BASS_ASIO && echo ALL OK
```

Expected: `ALL OK`.

- [ ] **Step 7: Commit (McOsu)**

```bash
cd ~/McOsu && git add src/App/Osu/OsuOptionsMenu.h src/App/Osu/OsuOptionsMenu.cpp
git commit -m "Options: add ASIO subsection (buffer size, control panel, restart)"
```

---

### Task 5: McOsu — main menu banner for ASIO

**Files:**
- Modify: `~/McOsu/src/App/Osu/OsuMainMenu.cpp:595-601`

**Interfaces:**
- Consumes: `isASIO()`, `getASIOBufferLength()`, `getASIOOutputLatency()` (Task 3).

- [ ] **Step 1: Banner text**

Replace the WASAPI banner block with:

```cpp
#ifdef MCENGINE_FEATURE_BASS_WASAPI

#ifdef MCENGINE_FEATURE_BASS_ASIO

		if (engine->getSound()->isASIO())
			bannerText = UString::format("-- ASIO Mode! buffer = %i samples, driver output latency = %.1f ms --",
					engine->getSound()->getASIOBufferLength(),
					engine->getSound()->getASIOOutputLatency()*1000.0f);
		else

#endif

		bannerText = UString::format(convar->getConVarByName("win_snd_wasapi_exclusive")->getBool() ?
				"-- WASAPI Exclusive Mode! win_snd_wasapi_buffer_size = %i ms --" :
				"-- WASAPI Shared Mode! win_snd_wasapi_buffer_size = %i ms --",
				(int)(std::round(convar->getConVarByName("win_snd_wasapi_buffer_size")->getFloat()*1000.0f)));

#endif
```

- [ ] **Step 2: Cross-compile in all three configurations**

```bash
SP=/tmp/claude-1000/-home-dog-McOsu/536c46db-cc8b-4956-acb7-9ac719de5056/scratchpad
F=~/McOsu/src/App/Osu/OsuMainMenu.cpp
$SP/ccheck.sh $F && $SP/ccheck.sh $F -DMCENGINE_FEATURE_BASS_WASAPI && $SP/ccheck.sh $F -DMCENGINE_FEATURE_BASS_WASAPI -DMCENGINE_FEATURE_BASS_ASIO && echo ALL OK
```

- [ ] **Step 3: Commit (McOsu)**

```bash
cd ~/McOsu && git add src/App/Osu/OsuMainMenu.cpp
git commit -m "Main menu: show ASIO buffer/latency in the banner"
```

---

### Task 6: Full 32-bit Windows link of McOsu.exe (WASAPI + ASIO)

**Files:**
- Create (scratchpad only, never committed): `$SP/Engine.patched.cpp` (copy of `Engine.cpp` with `#include "Osu.h"` and `m_app = new Osu();` enabled), `$SP/link.sh`
- Output: `$SP/dist/McOsu.exe` + required DLLs, zipped to `$SP/McOsu-asio-win32.zip`

**Interfaces:**
- Consumes: everything above; `$SP/fullcompile.sh` (compiles all 208 sources into `$SP/build-obj`, honours `$SP/Engine.patched.cpp` when present).

- [ ] **Step 1: Patch Engine.cpp copy for the Osu app**

```bash
SP=/tmp/claude-1000/-home-dog-McOsu/536c46db-cc8b-4956-acb7-9ac719de5056/scratchpad
sed -e 's|^//#include "Osu.h"|#include "Osu.h"|' -e 's|^\t\t//m_app = new Osu();|\t\tm_app = new Osu();|' -e 's|^\t\tm_app = new FrameworkTest();|\t\t//m_app = new FrameworkTest();|' \
    ~/McEngine/McEngine/src/Engine/Engine.cpp > $SP/Engine.patched.cpp
grep -n 'new Osu()\|#include "Osu.h"\|new FrameworkTest' $SP/Engine.patched.cpp
```

Expected: `#include "Osu.h"` and `m_app = new Osu();` uncommented, FrameworkTest commented.

- [ ] **Step 2: Compile everything**

```bash
$SP/fullcompile.sh
```

Expected last line: `=== done. objects: 208, failures: 0`. For any failure read `$SP/build-obj/<file>.log`; toolchain-only problems (clang vs GCC, header casing) are fixed with scratchpad shims in `$SP/shim`, never in the repos.

- [ ] **Step 3: Link**

> **Pitfall found during execution:** the un4seen `bass*.lib` files contain MSVC-style import-descriptor objects (`__IMPORT_DESCRIPTOR_BASS_FX`, `BASS_FX_NULL_THUNK_DATA`). lld links them in and then emits an *empty* import table for that DLL, so the first call (`BASS_FX_TempoCreate` on song select) jumps to an unpatched IAT slot and crashes. Generate clean import libraries first and put their directory first in the `-L` order:
>
> ```bash
> mkdir -p $SP/implibs2 && cd $SP/implibs2
> gen() { { echo "LIBRARY $(basename $2)"; echo EXPORTS; $TC/llvm-nm "$3" | grep -o "__imp__[A-Za-z0-9_]*@[0-9]*" | sed 's/^__imp__//' | sort -u; } > $1.def; $TC/llvm-dlltool -m i386 -k -d $1.def -l lib$1.a -D "$(basename $2)"; }
> gen bass $ME/build/bass.dll $ME/libraries/bass/lib/windows/bass.lib   # same for bass_fx, bassmix, basswasapi, bassasio
> ```
> Verify after linking: `llvm-readobj --coff-imports McOsu.exe` lists `Symbol:` lines under every DLL, and `llvm-nm McOsu.exe | grep NULL_THUNK` prints nothing.


```bash
cat > $SP/link.sh <<'EOF'
#!/bin/bash
set -u
SP=/tmp/claude-1000/-home-dog-McOsu/536c46db-cc8b-4956-acb7-9ac719de5056/scratchpad
TC=$SP/toolchain/llvm-mingw-20260826-msvcrt-ubuntu-22.04-x86_64/bin
ME=$HOME/McEngine/McEngine
mkdir -p $SP/dist
L=()
for d in $ME/libraries/*/lib/windows; do L+=("-L$d"); done
"$TC/i686-w64-mingw32-g++" -static -mwindows "${L[@]}" -o $SP/dist/McOsu.exe $SP/build-obj/*.o \
  -lbass -lbass_fx -lbassmix -lbasswasapi -lbassasio -lfreetype -lglew32 -lopengl32 -lglu32 -lgdi32 -lcomctl32 -ldwmapi -lcomdlg32 -lpsapi -lws2_32 -lwinmm -llibjpeg -lshell32 -lole32 -luuid
EOF
chmod +x $SP/link.sh && $SP/link.sh && file $SP/dist/McOsu.exe
```

Expected: `PE32 executable (GUI) Intel 80386`. Unresolved symbols name the missing `-l<lib>` (see the `.cproject` "Libraries (-l)" list for candidates); iterate on the link line only.

- [ ] **Step 4: Assemble the runtime set and zip it**

```bash
cd $SP/dist && cp ~/McEngine/McEngine/build/{bass.dll,bass_fx.dll,bassmix.dll,basswasapi.dll,bassasio.dll,bassflac.dll,glew32.dll} . 
cp -r ~/McEngine/McEngine/build/{fonts,shaders,sounds,materials,models} . 2>/dev/null; cp -r ~/McOsu/build/{models,shaders} . 2>/dev/null
ls -la $SP/dist && cd $SP && zip -qr McOsu-asio-win32.zip dist && ls -la McOsu-asio-win32.zip
```

Check what `~/McEngine/McEngine/build` actually contains (`ls`) and copy every non-.o asset directory the game needs (fonts, shaders, sounds, materials, models, plus McOsu's `build/models` and `build/shaders`); the exe expects them next to it.

- [ ] **Step 5: Record the result**

No commit (scratchpad artefacts). Note in the final report: object count, link result, zip path/size, and the manual test checklist from the spec.

---

### Task 7: Push and hand-off

- [ ] **Step 1: Push the McEngine branch to the user's fork**

```bash
cd ~/McEngine && git push -u origin asio
```

- [ ] **Step 2: McOsu branch stays local unless the user asks** (the McOsu remote is the user's fork already; report the branch name).

- [ ] **Step 3: Final report** with: what changed in each repo (files), how to build on Windows (uncomment `MCENGINE_FEATURE_BASS_WASAPI` and `MCENGINE_FEATURE_BASS_ASIO` in `EngineFeatures.h`, copy `src/App/Osu` into `McEngine/src/App/`, enable `new Osu()` in `Engine.cpp`, ship `bassasio.dll`), the zip location, and the untested-on-hardware caveat with the manual checklist.
