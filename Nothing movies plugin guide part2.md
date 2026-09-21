# Nothing Movies — Plugin Authoring Guide
## Part 2: `search`, `getStreamUrl`, `getSubtitleUrls`, and stream/download routing

> Part 1 covered `getHomepage` and `getMediaInfo`. This part covers the search
> pipeline, stream/download URL resolution, subtitle support, and how the app
> routes everything to the right backend based on your manifest flags.

---

## The search pipeline

Search is the most important part of the whole app. Every source that ships
must implement `search()` — it is the one non-optional method.

### The rule: `hasInfo` controls TMDB enrichment

- `hasInfo: false` (e.g. a torrent indexer) — your raw results go through
  the TMDB title cleaner and poster matcher. Ugly torrent names like
  `Interstellar.2014.1080p.BluRay` get cleaned to `Interstellar (2014)` with
  a real poster pulled from TMDB. You don't do anything — the app handles it.

- `hasInfo: true` (e.g. a streaming site with its own metadata) — your
  results are used as-is. No TMDB call, no title cleaning. Your source is
  expected to return clean titles and poster URLs directly.

### What `search()` returns

```cpp
std::vector<core::MediaResult> MyProvider::search(const std::string& query) {
    std::vector<core::MediaResult> results;

    // hit your site/API, parse results, fill in:
    core::MediaResult r;
    r.id         = "unique-id-from-your-source";  // you get this back in getStreamUrl
    r.title      = "Movie Title";                  // raw is fine for hasInfo=false sources
    r.posterUrl  = "https://...";                  // leave empty for hasInfo=false (TMDB fills it)
    r.sourceName = "MySource";                     // must match your registered name exactly

    results.push_back(r);
    return results;
}
```

**For `hasInfo: false` sources** (torrent indexers):
- `title` — return the raw torrent name, the cleaner handles it
- `posterUrl` — leave empty, TMDB fills it
- `id` — return whatever your source uses to identify the result (info hash,
  internal ID, etc.) — you'll need it in `getStreamUrl`

**For `hasInfo: true` sources** (streaming sites):
- `title` — return the clean, human-readable title
- `posterUrl` — return a real poster URL if you have one
- `id` — same rule, you'll need it back in `getStreamUrl` and `getMediaInfo`

---

## `getStreamUrl()` — returning a playable URL

Called when the user taps Stream or Download. Return one string:

```cpp
std::string MyProvider::getStreamUrl(const std::string& id) {
    // look up the stream URL for this id
    // return a magnet URI for torrents:
    return "magnet:?xt=urn:btih:...&dn=...";

    // or an HTTP URL for direct streams/downloads:
    return "https://cdn.example.com/video/12345.mp4";
}
```

The app routes the returned string automatically based on your manifest:

| `streamType` in manifest | What the app does with the URL |
|---|---|
| `"torrent"` | Passes to torrent manager, starts sequential download, plays when buffered |
| `"http"` | Passes directly to mpv player, starts immediately |

| `downloadType` in manifest | What the app does |
|---|---|
| `"torrent"` | Queues as torrent download |
| `"http"` | Queues as direct HTTP download |

**You never touch the player or downloader directly.** Return the URL string
and the app's routing layer handles the rest based on what your manifest says.

---

## `getSubtitleUrls()` — optional subtitle support

Only called if `hasSubtitles: true` in your manifest. Return a list of
direct subtitle URLs — the player loads them automatically via mpv's
`sub-add` command, so any URL mpv can fetch works.

```cpp
std::vector<std::string> MyProvider::getSubtitleUrls(const std::string& id) {
    // return direct URLs to subtitle files (.srt, .vtt, .ass, etc.)
    return {
        "https://example.com/subs/12345_en.srt",
        "https://example.com/subs/12345_fr.srt"
    };

    // return empty vector if no subtitles available for this id
    return {};
}
```

If `hasSubtitles: false` in your manifest, this method is never called —
stub it to return `{}` and move on.

---

## Stream while downloading (torrents)

If your source uses `streamType: "torrent"`, the app automatically handles
stream-while-downloading:

1. User taps Stream
2. App calls `getStreamUrl()` → you return a magnet URI
3. App adds the magnet to the torrent manager with sequential piece priority
4. App polls progress every second
5. When 5% is buffered (`readyToPlay: true`), the app opens the file in mpv
6. mpv plays from the front while the torrent continues downloading in the background
7. The download stays in the queue — user can see it in Downloads

You don't implement any of this. Your job is just to return the magnet URI.

---

## Series episode streaming

For series sources (`hasSeries: true`), each episode in `getMediaInfo()`
can have its own `streamUrl` pre-filled:

```cpp
info.episodes = {
    {
        .season    = 1,
        .episode   = 1,
        .title     = "Pilot",
        .synopsis  = "...",
        .streamUrl = "magnet:?xt=urn:btih:..."  // pre-filled stream URL
    }
};
```

If `streamUrl` is set on an episode, the app uses it directly when the
user taps Play on that episode — `getStreamUrl()` is not called again.

If `streamUrl` is empty, the app falls back to calling
`getStreamUrl(result.id)` — useful if your source resolves episode URLs
lazily (e.g. needs a separate API call per episode).

---

## Full manifest for a streaming source (hasInfo: true)

```json
{
  "name": "MyStreamingSource",
  "version": "1.0.0",
  "profilePicture": "https://...",

  "hasHomepage":  true,
  "hasPoster":    true,
  "hasRating":    true,
  "hasSubtitles": true,
  "hasDownload":  true,
  "hasStream":    true,
  "streamType":   "http",
  "downloadType": "http",
  "hasInfo":      true,

  "info": {
    "hasMovie":    true,
    "hasSeries":   true,
    "hasYear":     true,
    "hasRating":   true,
    "hasCast":     true,
    "hasSynopsis": true,
    "hasTrailer":  false,
    "hasQuality":  true,
    "hasLength":   true,
    "hasSubtitles":true
  }
}
```

## Full manifest for a torrent indexer (hasInfo: false)

```json
{
  "name": "MyTorrentSource",
  "version": "1.0.0",
  "profilePicture": "https://...",

  "hasHomepage":  false,
  "hasPoster":    false,
  "hasRating":    false,
  "hasSubtitles": false,
  "hasDownload":  true,
  "hasStream":    true,
  "streamType":   "torrent",
  "downloadType": "torrent",
  "hasInfo":      false
}
```

---

## Methods summary — what you must implement

| Method | Required | Notes |
|---|---|---|
| `search()` | Always | The only truly mandatory method |
| `getStreamUrl()` | If `hasStream` or `hasDownload` | Return magnet or HTTP URL |
| `getCapabilities()` | Always | Return your parsed manifest |
| `getHomepage()` | If `hasHomepage` | Return homepage items |
| `getMediaInfo()` | If `hasInfo` | Return full metadata per title |
| `getSubtitleUrls()` | If `hasSubtitles` | Return subtitle URLs |

For any method your source doesn't support, stub it to return `{}` or `""`.
The manifest flags tell the app which methods to call — a flag set to `false`
means that method is never called, so a stub is safe.

---

## Part 3 preview

Part 3 will cover:
- Versioning your source and how `version` in the manifest is used
- The `vendor_updater` integration for sources that need external tools
- Testing your source in isolation before registering it
- Windows + Linux cross-platform build checklist
- Submitting a PR