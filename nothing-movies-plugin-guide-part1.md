# Nothing Movies — Plugin Authoring Guide
## Part 1: `getHomepage` and `getInfo`

> This guide explains how to build a movie source plugin for Nothing Movies.
> A plugin is a C++ module that implements `core::ISourceProvider` and ships
> an `info.json` manifest. The manifest tells the app what your source can do —
> the app reads it at startup and adjusts the UI accordingly. You never touch
> UI code. You just implement the interface and fill the JSON.

---

## The two files every plugin ships

```
movie_sourceN/
├── CMakeLists.txt
├── include/movie_sourceN/movie_sourceN.h
├── src/movie_sourceN.cpp
└── data/
    ├── info.json          ← the manifest (this guide)
    └── mock_homepage.json ← optional fake data for development
```

---

## `info.json` — the manifest

This is the single source of truth for what your plugin can do.
The app reads it once at startup. Every flag here controls a UI branch —
if you set something to `false`, the corresponding button, section, or
layout simply does not appear. No dead buttons, no broken states.

### Full manifest reference

```json
{
  "name": "MySource",

  "hasHomepage":  true,
  "hasPoster":    true,
  "hasRating":    true,
  "hasSubtitles": false,
  "hasDownload":  true,
  "hasStream":    true,
  "streamType":   "http",
  "downloadType": "torrent",
  "hasInfo":      true,

  "info": {
    "hasMovie":    true,
    "hasSeries":   true,
    "hasYear":     true,
    "hasRating":   true,
    "hasCast":     true,
    "hasSynopsis": true,
    "hasTrailer":  true,
    "hasQuality":  true,
    "hasLength":   true,
    "hasSubtitles":true
  }
}
```

### Top-level flags

| Flag | Type | What it does |
|------|------|-------------|
| `name` | string | Display name shown in the UI next to your source's content |
| `hasHomepage` | bool | Your source provides a curated homepage list. If `false`, the homepage falls back to TMDB trending and shows a notice to the user |
| `hasPoster` | bool | Your homepage/search items include poster image URLs. If `false`, the UI shows a 🎬 placeholder instead |
| `hasRating` | bool | Your items include a numeric rating. If `false`, the star rating element is hidden |
| `hasSubtitles` | bool | Your source can provide subtitle URLs. If `false`, the subtitle button is hidden everywhere |
| `hasDownload` | bool | Your source supports downloading. If `false`, the Download button never appears |
| `hasStream` | bool | Your source supports streaming. If `false`, the Stream button never appears |
| `streamType` | string | `"http"` routes to the HTTP player. `"torrent"` routes through the torrent manager |
| `downloadType` | string | `"http"` downloads directly. `"torrent"` queues a torrent download |
| `hasInfo` | bool | Your source can return a full info page for a title. If `false`, hovering a card shows no "View Info" button |

### The `info` block

This block only matters when `hasInfo` is `true`. It describes what fields
your `getMediaInfo()` implementation actually fills in. Fields you leave
`false` are hidden in `InfoScreen.qml` — the UI never shows an empty cast
list or a blank synopsis.

| Flag | What it controls in the UI |
|------|---------------------------|
| `hasMovie` | Your source returns movies. Enables single stream/download button layout in InfoScreen |
| `hasSeries` | Your source returns series. Enables season selector + episode list in InfoScreen |
| `hasYear` | Shows the release year next to the title |
| `hasRating` | Shows the star rating on the info page |
| `hasCast` | Shows the cast list |
| `hasSynopsis` | Shows the synopsis/description block |
| `hasTrailer` | Shows a "Watch Trailer" button |
| `hasQuality` | Shows the quality badge (e.g. "1080p", "4K") |
| `hasLength` | Shows the runtime in minutes |
| `hasSubtitles` | Shows subtitle track selector on the info page |

> A source can have both `hasMovie: true` and `hasSeries: true`. The app
> determines which layout to use per-title from the `type` field your
> `getMediaInfo()` returns — `"movie"` or `"series"`.

---

## The C++ interface

Your plugin class inherits from `core::ISourceProvider` and implements
these methods. You only need to implement what your manifest claims —
but all methods must exist (return empty/stub for anything you don't support).

```cpp
class MyProvider : public core::ISourceProvider {
public:
    // Called once at startup. Return your parsed SourceCapabilities.
    core::SourceCapabilities getCapabilities() const override;

    // Called on the home screen if hasHomepage is true.
    // Return a list of items to display in the source's grid section.
    std::vector<core::HomepageItem> getHomepage() override;

    // Called when the user taps "View Info" on one of your items.
    // id is the same id string you set in HomepageItem or MediaResult.
    // Return a fully populated MediaInfo. Set type to "movie" or "series".
    core::MediaInfo getMediaInfo(const std::string& id) override;

    // Called when the user taps Stream or Download.
    // Return a playable URL (HTTP) or magnet URI (torrent).
    std::string getStreamUrl(const std::string& id) override;

    // Called when the user searches. Return matching results.
    // Return empty vector if your source does not support search.
    std::vector<core::MediaResult> search(const std::string& query) override;
};
```

---

## `getHomepage()` — populating the home screen grid

When `hasHomepage: true` the app calls `getHomepage()` at startup and
displays the results in a 5-column grid above the TMDB trending section.

### What to return

```cpp
std::vector<core::HomepageItem> MyProvider::getHomepage() {
    return {
        {
            .id        = "unique-id-123",   // you'll get this back in getMediaInfo/getStreamUrl
            .title     = "Some Movie",
            .imageUrl  = "https://...",     // poster URL, leave empty if hasPoster is false
            .category  = "Trending",        // optional label
            .trailerLink = "",              // optional
            .genre     = "Action",          // optional
            .rating    = 7.5               // 0.0 if hasRating is false
        }
    };
}
```

### UI result

- Items with a non-empty `imageUrl` → poster shown in the card
- Items with empty `imageUrl` → 🎬 emoji placeholder (the UI handles this automatically)
- Rating shown as ★ 7.5 if `hasRating: true`, hidden otherwise
- If your `getHomepage()` returns an empty list, the fallback banner appears

---

## `getMediaInfo()` — the info page

When `hasInfo: true` and the user hovers a card, a "View Info" button
appears. Tapping it calls `getMediaInfo(id)` with the `id` from your
`HomepageItem`. Return a `core::MediaInfo`:

### Movie example

```cpp
core::MediaInfo MyProvider::getMediaInfo(const std::string& id) {
    core::MediaInfo info;
    info.id          = id;
    info.type        = "movie";           // triggers single-item layout in InfoScreen
    info.title       = "Some Movie";
    info.posterUrl   = "https://...";
    info.backdropUrl = "https://...";
    info.synopsis    = "A film about things.";
    info.year        = 2023;
    info.lengthMins  = 118;
    info.rating      = 7.5;
    info.quality     = "1080p";
    info.cast        = { "Actor A", "Actor B" };
    info.trailerUrl  = "https://...";
    // info.episodes is empty for movies
    return info;
}
```

### Series example

```cpp
core::MediaInfo MyProvider::getMediaInfo(const std::string& id) {
    core::MediaInfo info;
    info.id    = id;
    info.type  = "series";               // triggers season/episode layout in InfoScreen
    info.title = "Some Show";
    // ... same fields as movie ...

    // Episodes — the UI groups these by season automatically
    info.episodes = {
        { .season = 1, .episode = 1, .title = "Pilot",   .streamUrl = "magnet:?..." },
        { .season = 1, .episode = 2, .title = "Episode 2", .streamUrl = "magnet:?..." },
        { .season = 2, .episode = 1, .title = "S2 Opener", .streamUrl = "magnet:?..." },
    };
    return info;
}
```

### What the UI does with `type`

| `type` value | InfoScreen layout |
|---|---|
| `"movie"` | Poster + metadata + single Stream/Download button row |
| `"series"` | Poster + metadata + season selector + scrollable episode list, each episode has its own Stream button |

---

## Stream and download routing

The app reads `streamType` and `downloadType` from your manifest and
routes accordingly — you don't need to do anything special in code:

| Config | What happens when user taps the button |
|--------|---------------------------------------|
| `streamType: "http"` | URL from `getStreamUrl()` is handed directly to the mpv player |
| `streamType: "torrent"` | URL (magnet URI) is handed to the torrent manager, which streams while downloading |
| `downloadType: "http"` | URL is downloaded directly via the downloader module |
| `downloadType: "torrent"` | Magnet URI is queued in the torrent download manager |

Your `getStreamUrl()` just returns the right string for your source —
the app figures out what to do with it based on the manifest.

---

## Registering your plugin

In `app/main.cpp`, inside `buildSourceAggregator()`, add one line:

```cpp
#include "movie_sourceN/movie_sourceN.h"

// in buildSourceAggregator():
aggregator->registerSource("My Source Name", std::make_shared<myns::MyProvider>());
```

And in `app/CMakeLists.txt` add `movie_sourceN` to `target_link_libraries`.
And in the root `CMakeLists.txt` add `add_subdirectory(movie_sourceN)`.

That's it. The aggregator, homepage bridge, and search bridge handle
everything else automatically.

---

## Part 2 preview

Part 2 will cover:
- `search()` — returning results from a query
- `getStreamUrl()` — returning HTTP URLs vs magnet URIs
- Subtitle URL wiring
- Series episode stream routing
- Testing your plugin in isolation before registering it
