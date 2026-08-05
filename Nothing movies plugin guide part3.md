# Nothing Movies — Plugin Authoring Guide
## Part 3: Nothing Browser Sources

> This part covers how to build a movie source that uses Nothing Browser
> as its scraping engine. Read Part 1 and Part 2 first.

---

## The rule: Nothing Browser is the only scraping engine

All movie sources that need to scrape a website **must use Nothing Browser**
and no other tool. No Playwright, no Puppeteer, no Python Selenium, no
headless Chrome wrappers of any kind.

**Why:** one engine means one thing to maintain. When something about the
underlying binary or protocol changes, the fix goes into `scraper_core`
once and every source benefits automatically. If everyone used a different
tool, every source would break independently and need separate fixes.

**Do not write or ship any JS/TS/Python in your source module.** Everything
your source needs to do should be reachable through the `scraper_core` API
below. If it isn't, that's a gap in `scraper_core` — raise it, don't work
around it with a second tool.

---

## When do you actually need Nothing Browser?

Not every source needs it. Ask yourself:

- Does the site have a clean JSON/REST API I can hit with libcurl? → **no browser needed**
- Does getting what you need require executing JavaScript in a real page
  context, or interacting with the DOM? → **Nothing Browser needed for that step**

A source can use libcurl for most of its data and Nothing Browser only for
the specific steps that need a real browser. Use the simplest tool that
works for each step — don't route everything through Nothing Browser by
default just because it's available.

---

## The C++ interface — `scraper_core`

`scraper_core` owns one thing: getting you a live, supervised connection to
the Nothing Browser daemon, and letting you send it commands. It does not
know what a "site" is, does not track tabs for you, and does not decide
what commands to send or in what order — that's entirely your source
module's job.

```cpp
#include "scraper_core/ScraperEngine.h"
```

The engine object itself is owned elsewhere in the app and started once,
early — your source module doesn't construct or `start()` it, it's handed
a reference to use.

### `scraper_core::isAvailable()`

```cpp
bool isAvailable();
```

Returns `true` if the Nothing Browser binary is installed. Check this in
your source's `init()` and fail gracefully if it's `false`:

```cpp
bool MySource::init() {
    if (!scraper_core::isAvailable()) {
        std::cerr << "[my_source] Nothing Browser not found — unavailable\n";
        return false;
    }
    return true;
}
```

### `NothingBrowser::sendRaw(cmd, payload, timeoutMs)`

```cpp
QJsonObject sendRaw(const QString& cmd,
                     const QJsonObject& payload = {},
                     int timeoutMs = 15000);
```

This is the only function that actually does anything, and the only one
your source calls directly. It sends `cmd` with `payload` to the daemon and
blocks until the reply comes back (or times out), returning:

```json
{ "id": "...", "ok": true, "data": <command-specific result> }
```

or on failure:

```json
{ "id": "...", "ok": false, "data": "<error description>" }
```

Always check `.value("ok").toBool()` before trusting `.value("data")`.

There's no translation layer — whatever commands the Nothing Browser daemon
documents are exactly what you pass as `cmd`, with whatever payload fields
it expects. `scraper_core` doesn't validate or reshape them.

Nothing Browser's wire protocol (informally, "the Piggy Protocol") — every
command name, its payload shape, and what it returns — is documented here:

> https://github.com/ernest-tech-house-co-operation/nothing-browser/blob/main/PROTOCOL.md

That doc is the authoritative command reference. If you're not sure what a
command expects or returns, that's where to look — not this guide.

### Tabs are yours to manage

There's no `registerSite` or site registry. You open your own tab and keep
the id yourself:

```cpp
QJsonObject tabReply = engine->sendRaw("tab.new");
QString tabId = tabReply.value("data").toString();

// tabId is now yours — pass it in the payload of whatever commands you send
engine->sendRaw("navigate", {{"tabId", tabId}, {"url", someUrl}});

// close it when you're done
engine->sendRaw("tab.close", {{"tabId", tabId}});
```

### `daemonRecovered` signal

```cpp
void daemonRecovered();
```

`scraper_core` runs a watchdog: if the daemon connection drops unexpectedly
(crash, external kill, etc.), it auto-reconnects or respawns the daemon on
its own. When that happens, **any tabId your source was holding is now
invalid**. Connect to this signal if your source needs to be robust across
a mid-session daemon restart — it's your cue to open a fresh tab and
recover your own state, however makes sense for what you were doing.

### What your source is responsible for

Everything past "open a tab, send commands, read replies" is up to you:
which commands to send, in what sequence, what selectors or page
interactions your target needs, how to interpret the results, and how to
handle that specific site's behavior. `scraper_core` deliberately has no
opinion on any of it.

---

## External dependency rules

Nothing Browser binary is **never bundled in the Nothing Movies app binary**.

- **Windows:** the Nothing Movies installer handles downloading and installing
  Nothing Browser. The installer checks for it and fetches it if missing.
- **Linux:** installation docs include a one-liner:
  ```bash
  curl -L https://github.com/BunElysiaReact/nothing-browser/releases/latest/download/nothing-browser-linux-x86_64.tar.gz | tar -xz -C ~/.local/bin
  ```
- **Auto-update:** `vendor_updater` keeps the Nothing Browser binary current.
  Wire your source's vendor target the same way `scraper_core` does — point
  it at the Nothing Browser GitHub releases and let the updater handle the
  rest.

Your source's `init()` should verify Nothing Browser exists before
attempting to use it, and fail gracefully with a clear error if it doesn't
(see the `isAvailable()` example above).

---

## Full source structure for a Nothing Browser source

```
movie_source2/
├── CMakeLists.txt
├── include/movie_source2/movie_source2.h
├── src/movie_source2.cpp
└── data/
    └── info.json
```

Nothing changes here from before — `scraper_core` absorbs all the
transport complexity described above. Your source's CMakeLists just links it:

```cmake
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

add_library(movie_source2 STATIC src/movie_source2.cpp)
target_include_directories(movie_source2 PUBLIC include)
target_link_libraries(movie_source2 PUBLIC
    core
    scraper_core
    CURL::libcurl
    nlohmann_json::nlohmann_json
)

add_custom_command(
    TARGET movie_source2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/data
        ${CMAKE_BINARY_DIR}/movie_source2/data
    COMMENT "Copying movie_source2 data files to build directory"
)
```

---

## What goes in `info.json` for a Nothing Browser source

Same format as any other source — using Nothing Browser internally is an
implementation detail, the manifest just describes capabilities:

```json
{
  "name": "MySource",
  "version": "1.0.0",
  "profilePicture": "https://example.com/favicon.ico",
  "hasHomepage": true,
  "hasPoster": true,
  "hasRating": true,
  "hasSubtitles": true,
  "hasDownload": true,
  "hasStream": true,
  "streamType": "http",
  "downloadType": "http",
  "hasInfo": true,
  "info": {
    "hasMovie": true,
    "hasSeries": true,
    "hasYear": true,
    "hasRating": true,
    "hasCast": false,
    "hasSynopsis": true,
    "hasTrailer": false,
    "hasQuality": true,
    "hasLength": false,
    "hasSubtitles": true
  }
}
```

---

## Submission checklist for Nothing Browser sources

Everything in Part 1 and Part 2 applies, plus:

- [ ] Uses `scraper_core::NothingBrowser` — no other scraping tool, no JS/TS/Python in the module itself
- [ ] `init()` checks for Nothing Browser availability and fails gracefully
- [ ] External binary wired to `vendor_updater` — not bundled
- [ ] Handles `daemonRecovered` sensibly if the source holds long-lived tab state
- [ ] Nothing Browser version requirement documented in the PR

---

## Part 4 preview

Part 4 will cover:
- Windows + Linux cross-platform build checklist for Nothing Browser sources
- The `vendor_updater` integration in detail
- Submitting a PR with a Nothing Browser source