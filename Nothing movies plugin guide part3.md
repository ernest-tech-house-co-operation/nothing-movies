# Nothing Movies — Plugin Authoring Guide
## Part 3: Nothing Browser Sources

> This part covers how to build a movie source that uses Nothing Browser
> as its scraping engine. Read Part 1 and Part 2 first.

---

## The rule: Nothing Browser is the only scraping engine

All movie sources that need to scrape a website **must use Nothing Browser**
and no other tool. No Playwright, no Puppeteer, no Python Selenium, no
headless Chrome wrappers of any kind.

**Why:** one engine means one thing to maintain. When a site changes its
anti-bot behavior, the fix goes into Nothing Browser (or `scraper_core`)
once and every source benefits automatically. If everyone used a different
tool, every source would break independently and need separate fixes.

---

## When do you actually need Nothing Browser?

Not every source needs it. Ask yourself:

- Does the site have a clean JSON/REST API I can hit with libcurl? → **no browser needed**
- Does getting the stream URL require executing JavaScript, extracting cookies,
  or submitting forms? → **Nothing Browser needed for that step only**

A source can use libcurl for data (homepage, search, info) and Nothing Browser
only for stream URL extraction. That's the recommended pattern — use the
simplest tool that works for each step.

---

## The C++ interface — `scraper_core`

Nothing Browser movie sources are written in **pure C++ against
`scraper_core`'s API**. That is the only thing a source ever touches:

```cpp
#include "scraper_core/ScraperEngine.h"

scraper_core::NothingBrowser browser;
browser.start();                                    // launches Nothing Browser
browser.registerSite("mysite", "https://example.com");

browser.call("mysite", "navigate", {"https://example.com/player/123"});

QJsonObject reply = browser.call("mysite", "provide.attr",
    {"form#wrapper input[name=csrftkn]", "value"});
QString token = reply.value("data").toString();

QJsonObject session = browser.call("mysite", "session.export");

browser.shutdown();
```

Method names passed to `call()` (`"navigate"`, `"provide.attr"`,
`"session.export"`, etc.) match the Nothing Browser site API 1:1 — see
`scraper_core/scripts/piggy_runner.js` for the authoritative list of what's
callable and how arguments map.

**Do not write or ship any JS/TS/Python in your source module.** If a method
you need isn't reachable through `browser.call(...)`, that's a `scraper_core`
gap — fix it there, don't work around it in your plugin.

---

## Implementation note: how `scraper_core` talks to Nothing Browser

This section is here so contributors understand the architecture — you
don't need any of this to *write* a source, only if you're modifying
`scraper_core` itself.

Nothing Browser's actual wire protocol (tab lifecycle, message framing,
event vs. reply vs. error shapes) is non-trivial and is already correctly
implemented and tested in the official `nothing-browser` npm client
library. Rather than re-deriving that protocol by hand in C++ — which
drifts out of sync with the real binary and is very easy to get subtly
wrong — `scraper_core::NothingBrowser` spawns a small, persistent Node
process (`scraper_core/scripts/piggy_runner.js`) that uses the real
`nothing-browser` client internally, and talks to it over a simple
JSON-lines protocol on stdin/stdout:

```
C++ (scraper_core)  <-- JSON lines over stdin/stdout -->  piggy_runner.js  <-- real protocol -->  Nothing Browser binary
```

This is an internal implementation detail of `scraper_core` only.
Source plugins never see `piggy_runner.js`, never spawn Node themselves,
and never send raw JSON commands — they call `browser.call(site, method,
args)` and get a `QJsonObject` back, same as always.

**Runtime requirement:** because of this, machines running Nothing Movies
now need Node.js available on `PATH` in addition to the Nothing Browser
binary. Document this alongside the Nothing Browser install step (see
below).

---

## External dependency rules

Nothing Browser binary is **never bundled in the Nothing Movies app binary**.

- **Windows:** the Nothing Movies installer handles downloading and installing
  Nothing Browser. The installer checks for it and fetches it if missing.
- **Linux:** installation docs include a one-liner:
  ```bash
  curl -L https://github.com/BunElysiaReact/nothing-browser/releases/latest/download/nothing-browser-linux-x86_64.tar.gz | tar -xz -C ~/.local/bin
  ```
- **Node.js + the runner's dependencies** must also be present at runtime:
  ```bash
  cd scraper_core/scripts
  npm install
  ```
  This installs the `nothing-browser` npm client that `piggy_runner.js`
  depends on. `node_modules/` here is git-ignored and must be installed
  locally / as part of your packaging step — it is never committed.
- **Auto-update:** `vendor_updater` keeps the Nothing Browser binary current.
  Wire your source's vendor target the same way `scraper_core` does — point
  it at the Nothing Browser GitHub releases and let the updater handle the
  rest. (The `piggy_runner.js` npm dependency is versioned via
  `scraper_core/scripts/package.json` instead.)

Your source's `init()` should verify Nothing Browser exists before
attempting to use it, and fail gracefully with a clear error if it doesn't:

```cpp
bool AnimeCloudProvider::init() {
    if (!scraper_core::isAvailable()) {
        std::cerr << "[movie_source2] Nothing Browser not found — stream extraction unavailable\n";
        return false;
    }
    return true;
}
```

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
complexity described above. Your source's CMakeLists just links it:

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

## Example: AnimeCloud (movie_source2) flow

### Data flow (no browser needed)

```
getHomepage()
  → POST /api.v1.anime.AnimeService/ListAnimesByViewCount {"page": 1}
  → parse JSON → vector<HomepageItem>

search(query)
  → POST /api.v1.AnimeSearchService/SearchAnimes {"q": "query"}
  → parse JSON → vector<MediaResult>

getMediaInfo(id)
  → POST /api.v1.anime.AnimeService/GetAnime {"slug": id}
  → parse JSON → MediaInfo with title, poster, synopsis, genres
  → for each season/episode:
      POST /api.v1.anime.AnimeService/GetEpisode {"slug", "season", "episode"}
      → extract episode links
  → return MediaInfo with full episode list
```

### Stream URL extraction (Nothing Browser needed)

```
getStreamUrl(id)
  → browser.start()
  → browser.registerSite(siteName, baseUrl)
  → browser.call(siteName, "navigate", {baseUrl + "/proxy/player/" + id})
  → browser.call(siteName, "provide.attr",
                  {"form#wrapper input[name=csrftkn]", "value"})
      → csrfToken = reply.data
  → browser.call(siteName, "navigate",
                  {baseUrl + "/proxy/player/" + id + "?csrftkn=" + csrfToken})
  → browser.call(siteName, "session.export")
      → sessionCookie = reply.data
  → return baseUrl + "/proxy/nocache/" + id + "/" with Cookie header
```

---

## What goes in `info.json` for a Nothing Browser source

Same format as any other source — using Nothing Browser internally is an
implementation detail, the manifest just describes capabilities:

```json
{
  "name": "AnimeCloud",
  "version": "1.0.0",
  "profilePicture": "https://fireani.me/favicon.ico",
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
- [ ] Tested with Nothing Browser **and** Node.js installed, plus
      `scraper_core/scripts/npm install` run at least once
- [ ] Stream URL extraction tested and confirmed working
- [ ] Nothing Browser version requirement documented in the PR

---

## Part 4 preview

Part 4 will cover:
- Windows + Linux cross-platform build checklist for Nothing Browser sources
  (including the Node.js runtime requirement)
- How to document selector/method changes when a site or `piggy_runner.js` updates
- The `vendor_updater` integration in detail
- Submitting a PR with a Nothing Browser source