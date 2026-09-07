# ASIO audio output for McOsu (Windows) — design

Date: 2026-09-07
Repos touched: `McEngine` (engine, branch `asio`) and `McOsu` (this repo, branch `asio`).

## Goal

Let McOsu play audio through an ASIO driver (via un4seen's BASSASIO add-on) so that
music and hitsounds have driver-level latency (typically 3–10 ms) instead of the
~40 ms of the default BASS/WASAPI-shared path or the even higher DirectSound path.

ASIO is added as a **runtime-selectable output driver inside the existing WASAPI
exclusive build**. One binary offers both WASAPI devices and ASIO devices in the
output device list; the user picks one.

## Background: how audio currently works

- McOsu contains only the app layer. `SoundEngine` / `Sound` (the BASS wrappers)
  live in McEngine (`McEngine/src/Engine/SoundEngine.*`, `Sound.*`).
- Default Windows build: `BASS_Init(device, ...)` with BASS's own output
  (WASAPI shared internally, or DirectSound when `win_snd_fallback_dsound` is set).
  Hitsounds are `BASS_SampleLoad` samples, music is a BASS_FX tempo stream that BASS
  plays directly.
- WASAPI build (`MCENGINE_FEATURE_BASS_WASAPI` in `EngineFeatures.h`):
  `BASS_Init(0 /* no sound */)`, then `BASS_WASAPI_Init(device, exclusive, buffer,
  period, OutputWasapiProc)`. All sounds are created as *decode* streams
  (`BASS_STREAM_DECODE | BASS_SAMPLE_FLOAT`), pushed into one bassmix mixer
  (`g_wasapiOutputMixer`, `BASS_MIXER_NONSTOP`), and `OutputWasapiProc` pulls mixed
  float data from that mixer. Hitsounds are loaded from an in-memory file buffer
  into fresh decode streams per play (`m_danglingWasapiStreams`).

BASSASIO uses exactly the same shape: `BASS_ASIO_Init(device)`, enable output channel
0 with an `ASIOPROC` callback, join channel 1 to it for stereo, set float format,
`BASS_ASIO_Start(bufferLength)`. The callback pulls from the same mixer. Therefore
`Sound.cpp` needs **no changes**; only `SoundEngine` grows an ASIO branch.

## Constraints and decisions

- `MCENGINE_FEATURE_BASS_ASIO` **requires** `MCENGINE_FEATURE_BASS_WASAPI`
  (enforced with `#error`). An ASIO-only build is out of scope: the mixer-based
  `Sound.cpp` code path is keyed on the WASAPI define, and WASAPI is the natural
  fallback when an ASIO driver fails.
- Windows binaries are 32-bit (all shipped BASS DLLs are i386). Use the 32-bit
  `bassasio.dll` / `c/bassasio.lib` from `bassasio14.zip` (BASSASIO 1.4.3).
- Keep the diff to McEngine minimal so the fork can keep merging upstream: no
  renames of existing members/globals (`g_wasapiOutputMixer` is reused for ASIO
  with a comment), ASIO code is added under `#ifdef MCENGINE_FEATURE_BASS_ASIO`
  blocks nested in the existing WASAPI sections.
- Hardcoded offset: McOsu sets `osu_universal_offset_hardcoded = -25` for WASAPI
  builds (mixer pipeline compensation). ASIO uses the same pipeline and keeps the
  same value. The driver-reported ASIO output latency is shown in the main menu
  banner so users can fine-tune with the normal Universal Offset slider. Deriving
  the offset from the reported latency is a possible follow-up, not part of this
  change.

## McEngine changes

### `src/Engine/Main/EngineFeatures.h`

```c
/*
 * BASS ASIO sound (Windows only, requires MCENGINE_FEATURE_BASS_WASAPI)
 */
//#define MCENGINE_FEATURE_BASS_ASIO

#if defined(MCENGINE_FEATURE_BASS_ASIO) && !defined(MCENGINE_FEATURE_BASS_WASAPI)
#error "MCENGINE_FEATURE_BASS_ASIO requires MCENGINE_FEATURE_BASS_WASAPI"
#endif
```

### Library files

- `libraries/bassasio/include/bassasio.h`
- `libraries/bassasio/lib/windows/bassasio.lib` (32-bit MinGW-linkable import lib)
- `libraries/bassasio/bin/bassasio.dll` and `build/bassasio.dll` (runtime)
- `.cproject`: add the bassasio include path, `-L` path and `bassasio` library next
  to the existing basswasapi entries in the Windows Debug and Windows Release configs.
- `build.bat` / `build_unity.bat`: add `-lbassasio` to `LDLIBS`.

### `src/Engine/SoundEngine.h`

```cpp
struct OUTPUT_DEVICE
{
    enum class DRIVER { BASS, WASAPI, ASIO };
    int id;
    bool enabled;
    bool isDefault;
    UString name;
    DRIVER driver;
};

// public, always declared (no-ops / false / 0 when ASIO is not compiled in):
bool isASIO() const;                    // current output device is an ASIO device
float getASIOOutputLatency() const;     // driver-reported output latency, seconds
int getASIOBufferLength() const;        // effective ASIO buffer length, samples
void openASIOControlPanel();            // BASS_ASIO_ControlPanel()

// private:
bool initializeOutputDevice(int id, OUTPUT_DEVICE::DRIVER driver);
#ifdef MCENGINE_FEATURE_BASS_ASIO
bool initializeASIOOutputDevice(int id);   // helper called from initializeOutputDevice
#endif
OUTPUT_DEVICE::DRIVER m_currentOutputDriver;
float m_fASIOOutputLatency;
int m_iASIOBufferLength;
```

The "Default" entry (id -1) has driver `WASAPI` in WASAPI builds and `BASS`
otherwise. All ASIO entries are explicit devices (BASSASIO has no default device).

### `src/Engine/SoundEngine.cpp`

ConVars (ASIO only):

- `win_snd_asio_buffer_size` (int, **samples**, default `0` = driver preferred
  length). Change callback forces a device restart, identical to the WASAPI
  buffer cvar. Clamped/snapped to the driver's `bufmin`/`bufmax`/`bufgran`
  (`-1` = powers of two, `0` = only `bufpref`). ASIO users think in samples
  (64/128/256), so the cvar and the UI use samples; milliseconds are shown as a
  derived value at the active device rate (`getASIOSampleRate()`).
- `win_snd_asio_control_panel` (concommand) → `openASIOControlPanel()`.

Callbacks:

```cpp
DWORD CALLBACK OutputAsioProc(BOOL input, DWORD channel, void *buffer, DWORD length, void *user)
{
    // same as OutputWasapiProc: BASS_ChannelGetData(g_wasapiOutputMixer, buffer, length)
}

void CALLBACK AsioNotifyProc(DWORD notify, void *user)
{
    if (notify == BASS_ASIO_NOTIFY_RESET) g_asioResetRequested = true;   // std::atomic<bool>
}
```

`SoundEngine::update()`: if `g_asioResetRequested` is set (driver asked for a
reset, e.g. buffer size changed in its control panel), clear it and call
`restart()` on the main thread.

Device enumeration (`updateOutputDevices`): after the WASAPI loop, enumerate
`BASS_ASIO_GetDeviceInfo(d, &info)` and append entries named `"[ASIO] " + info.name`
(driver `ASIO`, enabled, not default), reusing the duplicate-name suffix logic and
skipping entries already present (same driver + id).

`initializeOutputDevice(id, driver)`:

1. Record `m_iCurrentOutputDevice` / `m_currentOutputDriver`, apply the
   `snd_*` config overrides (unchanged).
2. Free the previous device: `BASS_Free()`, `BASS_WASAPI_Free()`, and
   `if (BASS_ASIO_IsStarted()) BASS_ASIO_Stop(); BASS_ASIO_Free();`.
   Reset `m_fASIOOutputLatency`, `m_iASIOBufferLength`.
3. `BASS_Init(0, freq, BASS_DEVICE_NOSPEAKER, hwnd)` (no-sound device, unchanged).
4. If `driver == ASIO` → `return initializeASIOOutputDevice(id);`
   else the existing WASAPI code runs unchanged.
5. On any failure return `false`; callers already fall back to the previous device.

`initializeASIOOutputDevice(id)`:

1. `BASS_ASIO_Init(id, BASS_ASIO_THREAD)` (dedicated driver thread: lets the
   device be freed from any thread and keeps driver windows working).
2. `BASS_ASIO_SetNotify(AsioNotifyProc, NULL)`.
3. `BASS_ASIO_SetRate(snd_freq)`; if the driver refuses, log and keep its rate.
   `rate = BASS_ASIO_GetRate()`.
4. `BASS_ASIO_GetInfo(&info)`; `chans = min(2, info.outputs)`; fail if 0.
5. `g_wasapiOutputMixer = BASS_Mixer_StreamCreate(rate, chans, BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | BASS_MIXER_NONSTOP)`.
6. `BASS_ASIO_ChannelEnable(FALSE, 0, OutputAsioProc, NULL)`;
   `if (chans == 2) BASS_ASIO_ChannelJoin(FALSE, 1, 0)`;
   `BASS_ASIO_ChannelSetFormat(FALSE, 0, BASS_ASIO_FORMAT_FLOAT)`.
7. `buflen = computeASIOBufferLength(info, rate)` from `win_snd_asio_buffer_size`.
8. `BASS_ASIO_Start(buflen, 0)`.
9. Store `m_iASIOBufferLength` (requested/snapped value) and
   `m_fASIOOutputLatency = BASS_ASIO_GetLatency(FALSE) / rate`; log both.
10. `BASS_ASIO_ChannelSetVolume(FALSE, -1, m_fVolume)`; set `m_bReady = true`
    and `m_sCurrentOutputDevice` (same tail as the WASAPI path).

Every failing BASSASIO call logs `BASS_ASIO_ErrorGetCode()` (not
`BASS_ErrorGetCode()`) and shows the existing "Sound Error" message box.

`setVolume()`: when the current driver is ASIO call
`BASS_ASIO_ChannelSetVolume(FALSE, -1, volume)` instead of `BASS_WASAPI_SetVolume`.

`~SoundEngine()`: `BASS_ASIO_Stop(); BASS_ASIO_Free();` when an ASIO device is
active (ASIO drivers are COM objects and should be released before DLL unload).

### Threading notes

`OutputAsioProc` runs on the ASIO driver thread, exactly like `OutputWasapiProc`
runs on the WASAPI thread; both only call `BASS_ChannelGetData` on the mixer, which
BASS documents as safe. `AsioNotifyProc` only sets an atomic flag.

## McOsu changes

### `OsuOptionsMenu.h/.cpp`

Inside `#ifdef MCENGINE_FEATURE_BASS_ASIO`, after the WASAPI subsection:

- `addSubSection("ASIO")`
- label: `"Pick an [ASIO] device in \"Select Output Device\" above."`
- `m_asioBufferSizeSlider = addSlider("Buffer Size:", 0.0f, 9.0f, NULL)`: a
  log-scale slider whose index maps to samples (0 = driver default, 1..9 =
  8, 16, ..., 2048). It is deliberately not bound to the cvar (the generic
  binding is linear), so `updateLayout()` refreshes it from the cvar by hand and
  there is no reset button. `onASIOBufferChange` shows `"%i (%.1f ms)"` at the
  active ASIO rate, or `"driver default"`. Applied deferred in `update()` when
  the slider is released, like the WASAPI sliders (`m_bASIOBufferChangeScheduled`).
- labels: `"0 = driver default. Most drivers set the buffer in their own panel:"`
- `addButton("Open ASIO Control Panel")` → `onASIOControlPanelClicked` →
  `engine->getSound()->openASIOControlPanel()`.
- `addButton("Restart SoundEngine")` → existing `onOutputDeviceRestart`.

New members: `m_asioBufferSizeSlider`, `m_asioBufferSizeResetButton`,
`m_bASIOBufferChangeScheduled`, `m_win_snd_asio_buffer_size_ref`.

### `OsuMainMenu.cpp` (banner, computed every frame in `draw()`)

Inside the WASAPI banner block, before the WASAPI text:

```cpp
#ifdef MCENGINE_FEATURE_BASS_ASIO
if (engine->getSound()->isASIO())
    bannerText = UString::format("-- ASIO Mode! buffer = %i samples (%.1f ms @ %i Hz), driver output latency = %.1f ms --",
        engine->getSound()->getASIOBufferLength(),
        engine->getSound()->getASIOOutputLatency()*1000.0f);
else
#endif
    bannerText = ... existing WASAPI text ...
```

### `Osu.cpp`

No change: the `-25` hardcoded offset already applies to every WASAPI build,
and ASIO builds are WASAPI builds.

## Error handling summary

| Situation | Behaviour |
|---|---|
| No ASIO drivers installed | No `[ASIO]` entries in the list; nothing else changes. |
| `BASS_ASIO_Init` / `Start` fails | Error box with BASSASIO error code; `setOutputDevice` re-initialises the previous device (WASAPI). |
| Driver refuses `snd_freq` rate | Mixer is created at the driver's rate; BASS resamples sources. |
| Driver only has 1 output | Mono mixer, single channel enabled. |
| Buffer size outside driver range | Clamped to `bufmin..bufmax` and snapped to granularity before `BASS_ASIO_Start`. |
| Driver requests reset (control panel change) | `update()` restarts the engine on the main thread; McOsu's existing device-change callback reloads music and skin sounds. |

## Verification

There is no unit-test infrastructure in either repo, and ASIO needs Windows plus a
real driver, so verification is:

1. **Cross-compile** every touched engine and app file with a self-contained
   llvm-mingw toolchain (i686 target) kept entirely in the session scratchpad:
   - default features (neither WASAPI nor ASIO): must still compile,
   - WASAPI only: must still compile,
   - WASAPI + ASIO: must compile.
2. **Full link** of a 32-bit `McOsu.exe` (WASAPI + ASIO) from McEngine +
   McOsu sources with the same toolchain, using the import libraries shipped in
   `McEngine/libraries`. The Engine.cpp app-selection edit (`new Osu()`) is done
   on a scratchpad copy, not in the repo. The resulting exe + DLL set is delivered
   as a zip for the user to test on Windows.
3. **Manual test checklist for the user (Windows, ASIO driver installed):**
   - `[ASIO] <driver>` entries appear in Options → Audio → Select Output Device.
   - Selecting one plays music and hitsounds; banner shows buffer/latency.
   - Volume sliders work; pause/resume/scrub work in song select and gameplay.
   - Changing buffer size in the driver panel restarts the engine without a crash.
   - Selecting a WASAPI device again works (fallback path).
   - Universal Offset calibration with the reported latency.

Nothing is installed system-wide; the toolchain, BASSASIO SDK and build
directories all live under the session scratchpad.
