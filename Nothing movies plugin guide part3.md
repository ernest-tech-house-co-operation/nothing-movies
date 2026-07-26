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
anti-bot behavior, the fix goes into Nothing Browser once and every source
benefits automatically. If everyone used a different tool, every source would
break independently and need separate fixes.

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

Nothing Browser movie sources are written in **pure C++**. Do not use the
Piggy JS library (`nothing-browser` npm package) or any JS/TypeScript layer.
`scraper_core` provides the C++ interface to Nothing Browser. That is what
you use.

```cpp
#include "scraper_core/scraper_core.h"

// Navigate to a page and get its HTML
std::string html = scraper_core::getPageHtml("https://example.com/player/123");

// Extract a value using a CSS selector
std::string token = scraper_core::querySelector(html, "form#wrapper input[name=csrftkn]", "value");

// Get a cookie from a response
std::string session = scraper_core::getCookie("https://example.com/proxy/player/abc?csrftkn=" + token, "session");
```

Check `scraper_core/include/scraper_core/scraper_core.h` for the full API.
If a method you need doesn't exist yet, add it to `scraper_core` — don't
work around it by shelling out or using a different tool.

---

## The Cloudstream reference pattern

When implementing a new source, find the Cloudstream plugin for the same
site if one exists. Cloudstream plugins are written in Kotlin but the logic
translates directly to C++:

| Cloudstream | C++ equivalent |
|---|---|
| `app.post(url, requestBody = json)` | `httpPost(url, json)` via libcurl |
| `app.get(url).document.select("selector")` | Nothing Browser + CSS selector |
| `app.get(url).cookies["session"]` | Nothing Browser cookie extraction |
| `parsedSafe<DataClass>()` | `nlohmann::json::parse()` |
| `data.animeSeasons.forEach { }` | range-based for loop over parsed JSON |

The Cloudstream plugin tells you the exact API endpoints, request format,
response structure, and what selectors to use. Translate it to C++ — don't
copy the Kotlin.

---

## External dependency rules

Nothing Browser binary is **never bundled in the Nothing Movies app binary**.

- **Windows:** the Nothing Movies installer handles downloading and installing
  Nothing Browser. The installer checks for it and fetches it if missing.
- **Linux:** installation docs include a one-liner:
  ```bash
  curl -L https://github.com/BunElysiaReact/nothing-browser/releases/latest/download/nothing-browser-linux-x86_64.tar.gz | tar -xz -C ~/.local/bin
  ```
- **Auto-update:** `vendor_updater` keeps the binary current. Wire your
  source's vendor target the same way `scraper_core` does — point it at
  the Nothing Browser GitHub releases and let the updater handle the rest.

Your source's `init()` should verify Nothing Browser exists and is the
expected version before attempting to use it, and fail gracefully with a
clear error if it doesn't:

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

**`CMakeLists.txt`** — link `scraper_core` alongside `curl` and `nlohmann_json`:

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

# Auto-copy data folder to build dir
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

This is what the implementation of `movie_source2` looks like conceptually.
The actual code lives in `movie_source2/src/movie_source2.cpp`.

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
  → id is the episode link URL from GetEpisode response
  → scraper_core: GET /proxy/player/{id}
      extract csrftkn from: form#wrapper input[name=csrftkn]
  → scraper_core: GET /proxy/player/adehu1awmdxx?csrftkn={token}
      extract session cookie
  → return "{mainUrl}/proxy/nocache/{id}/" + headers{"Cookie": "session={session}"}
```

---

## What goes in `info.json` for a Nothing Browser source

Same format as any other source. The fact that it uses Nothing Browser
internally is an implementation detail — the manifest just describes what
the source can do from the app's perspective:

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

- [ ] Uses `scraper_core` — no other scraping tool
- [ ] Written in pure C++ — no JS/TS/Python anywhere in the module
- [ ] `init()` checks for Nothing Browser availability and fails gracefully
- [ ] External binary wired to `vendor_updater` — not bundled
- [ ] Tested with Nothing Browser installed (not just with the JSON API parts)
- [ ] Stream URL extraction tested and confirmed working
- [ ] Nothing Browser version requirement documented in the PR

---

## Part 4 preview

Part 4 will cover:
- Windows + Linux cross-platform build checklist for Nothing Browser sources
- How to document selector changes when a site updates
- The `vendor_updater` integration in detail
- Submitting a PR with a Nothing Browser source