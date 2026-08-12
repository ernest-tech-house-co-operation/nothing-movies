# Nothing Movies — Plugin Authoring Guide
## Part 4: Cross-Platform Builds, VendorManager, and Submitting Your PR

> This part assumes you've built a working source per Parts 1–3. It
> covers what's left before it's actually shippable: making sure it
> builds cleanly on both supported platforms, wiring any external binary
> dependency into `VendorManager` correctly, and what a submission PR
> needs to include.

---

## 1. Cross-platform build checklist

Nothing Movies ships on **Windows** and **Linux**. Every source needs to
build and run correctly on both before it's merged.

### General

- [ ] Builds with no warnings-as-errors violations on both platforms'
      CI configuration
- [ ] No hardcoded path separators (`/` or `\`) — use
      `std::filesystem::path` for anything touching disk, and let it
      handle separators for you
- [ ] No hardcoded `HOME`/`LOCALAPPDATA`-style paths — if you need a
      user data directory, get it the same way `VendorManager::rootDir()`
      does (env var override → per-OS fallback), don't invent a second
      convention
- [ ] No shelling out to platform-specific commands (`ls`, `dir`, `grep`,
      etc.) unless guarded by `#ifdef _WIN32` / `#else` with a real
      equivalent on both sides. `vendor_updater`'s use of `tar` for
      `.tar.gz` extraction is a deliberate, documented exception — its
      Windows counterpart uses `.zip` assets specifically to avoid
      needing `tar` there. Follow that pattern rather than adding new
      shell-outs.

### If your source uses Nothing Browser (Part 3 sources)

- [ ] `scraper_core::isAvailable()` is checked before every use, not just
      once at startup — the binary can be mid-update or momentarily
      missing
- [ ] Your source doesn't assume a specific install location for Nothing
      Browser — that's `scraper_core`'s and `VendorManager`'s job, not
      yours
- [ ] Tested with a **fresh** (never-downloaded) Nothing Browser install
      to confirm your source's `init()` fails gracefully rather than
      crashing when it's genuinely absent

### If your source ships its own compiled dependency

- [ ] The dependency builds (or is fetched as a prebuilt binary) for
      **both** `windows-x64` and `linux-x86_64` at minimum — sources that
      only work on one platform won't be accepted unless the target site
      itself is platform-exclusive in some way that makes this
      unavoidable (rare — flag it explicitly in your PR if so)
- [ ] Any prebuilt binary dependency goes through `VendorManager` (see
      §2) — it is never checked into the repo or bundled in the app
      binary directly, regardless of platform

### Testing matrix

At minimum, before opening a PR, run your source through:

| Platform | Fresh install | Existing install, source's first run | Existing install, N-th run |
|---|---|---|---|
| Linux    | ✅ | ✅ | ✅ |
| Windows  | ✅ | ✅ | ✅ |

"Fresh install" catches missing-dependency handling. "First run" catches
any one-time setup your source does (creating folders, writing initial
config). "N-th run" catches anything that breaks on state left over from
a previous run — a very common bug class for scrapers that cache tab IDs,
tokens, or cursors across `getHomepage()`/`search()` calls.

---

## 2. `VendorManager` in full detail

Part 3 covered the basic registration call. This section is the complete
reference.

### Why this exists

Before `VendorManager`, each source (or the app itself) that needed an
external binary — like Nothing Browser — wired up its own download,
extraction, and update-check logic independently. That meant:

- external files ending up scattered across the filesystem with no
  single place a user or reviewer could go look at what's actually been
  downloaded and run on their machine
- no consistent way to confirm a given binary dependency was even
  open-source, let alone verify what it does
- every source re-solving the same problems (GitHub release parsing,
  platform-specific archive extraction, atomic swap-on-update) slightly
  differently, with slightly different bugs

`VendorManager` centralizes all of it into one registry with two rules
that are not configurable per-source: everything lives under one root
folder, and nothing gets registered without a public source repo attached.

### `VendorSpec` reference

```cpp
struct VendorSpec {
    std::string name;
    std::string sourceRepoUrl;
    std::string releaseRepo;
    std::string releasesApiUrl;
    std::string platformTag;
    std::vector<std::string> assetMustContain;
    std::vector<std::string> assetMustNotContain;
    std::string license;
};
```

| Field | Required? | Notes |
|---|---|---|
| `name` | Yes | Unique across the whole app. Becomes the vendor's folder name — no `/`, `\`, or `..`. |
| `sourceRepoUrl` | **Yes** | The tool's own public, open-source repository. Registration is refused without it — no exceptions, no override flag. |
| `releaseRepo` | Yes, unless `releasesApiUrl` is set | `"owner/repo"` — used to build `https://api.github.com/repos/<releaseRepo>/releases/latest`. |
| `releasesApiUrl` | No | Full override for tools that don't publish via the standard GitHub releases API shape. |
| `platformTag` | Yes | Substring that must appear in the release asset filename to match this platform, e.g. `"linux-x86_64"`, `"win-x64"`. |
| `assetMustContain` | No | Extra required substrings, for releases that publish multiple builds sharing a platform suffix (GUI vs. headless, etc). |
| `assetMustNotContain` | No | Substrings that disqualify a match even if everything else fits. |
| `license` | No | Informational only — shown in the Settings → External Tools UI. |

### How asset matching works

For each asset in the latest release, `VendorUpdater` checks, in order:

1. Does the filename contain `platformTag`? If not, reject.
2. Does it contain every string in `assetMustContain`? If any are
   missing, reject.
3. Does it contain any string in `assetMustNotContain`? If so, reject.
4. Does it end in `.zip` or `.tar.gz`? If not, reject.

The **first** asset that passes all four wins. If a release ever
publishes assets in an order where this could matter, make your
`assetMustContain`/`assetMustNotContain` specific enough that ordering
doesn't matter — don't rely on position.

### Where files actually end up

```
<rootDir()>/
├── manifest.json                 # every registered vendor's declared metadata
├── nothing-browser/               # spec.name — the live, in-use install
│   └── .version                   # last-applied release tag
├── nothing-browser_staging/        # transient — extraction happens here first
└── nothing-browser_prev/           # transient — old install, kept only during a swap
```

`rootDir()` itself:

- Linux: `$HOME/.local/share/nothingmovies/vendor`
- Windows: `%LOCALAPPDATA%\NothingMovies\vendor`
- Either platform: overridable via the `NOTHINGMOVIES_VENDOR_ROOT`
  environment variable (intended for tests and local dev, not for
  shipping a source that assumes a custom root — don't rely on this
  being set in production)

The `_staging` and `_prev` suffixes are transient and only exist mid-update:
a new version is fully downloaded and extracted into `_staging` first;
only once that's verified non-empty does the manager rename the current
install to `_prev`, promote `_staging` into place, and then delete `_prev`.
If your source somehow needs to reason about update state, treat the
existence of `_staging` or `_prev` as "an update is/was in progress," never
as a stable location to read from.

### `manifest.json`

Every successful `registerVendor()` call rewrites this file — a flat JSON
array of every currently-registered vendor's declared metadata:

```json
[
  {
    "name": "nothing-browser",
    "sourceRepoUrl": "https://github.com/ernest-tech-house-co-operation/nothing-browser",
    "releaseRepo": "BunElysiaReact/nothing-browser",
    "releasesApiUrl": "",
    "platformTag": "linux-x86_64",
    "license": "MIT"
  }
]
```

This exists specifically so the "what's actually running on my machine"
question has an answer that doesn't require opening the app — anyone can
read this file directly.

### Manual update checks and the UI

`Settings → External Tools` (`VendorsScreen.qml`, backed by `VendorBridge`)
lists every registered vendor with its current version, license, a link to
`sourceRepoUrl` (opens in the system browser via `Qt.openUrlExternally`),
and a **Check for updates** button per vendor plus a **Check All**.

If your source needs to trigger a check programmatically (e.g. right
after first registering a brand-new dependency, so it's usable
immediately rather than waiting for the next background cycle):

```cpp
auto result = vendor_updater::VendorManager::instance().checkAndUpdate(spec.name);
if (!result.error.empty()) {
    std::cerr << "[my_source] Failed to fetch dependency: " << result.error << "\n";
}
```

This call blocks and does real network I/O — don't call it from a UI
thread or anywhere latency-sensitive. `VendorBridge::checkVendor()` /
`checkAll()` already do this correctly (background thread, result
marshaled back via `QMetaObject::invokeMethod`); if you're calling
`VendorManager` directly from C++ rather than through the bridge, you're
responsible for doing the same.

### Background watching

Something in app startup (not your source) is responsible for calling:

```cpp
vendor_updater::VendorManager::instance().startAllBackgroundWatches(
    3600, // interval in seconds
    [](vendor_updater::UpdateResult result) {
        // log, notify, etc.
    }
);
```

Your source doesn't need to do this itself — registering is enough.
Just don't assume your dependency is guaranteed to be on the *very*
latest version at all times; background watches run on an interval, not
instantly on every release.

---

## 3. Submitting a PR with a Nothing Browser source

### Before you open the PR

- [ ] Everything in the Part 3 submission checklist
- [ ] Everything in the §1 cross-platform checklist above
- [ ] If you registered a new external dependency: confirm
      `manifest.json` picks it up correctly and `sourceRepoUrl` actually
      resolves to a real, public repository (a reviewer will check this)
- [ ] Run your source against a handful of real titles end to end —
      `getHomepage()`, `search()`, `getMediaInfo()`, `getStreamUrl()` (and
      `getSubtitleUrls()` if `hasSubtitles` is true) — and confirm the
      images actually load in the app (not just that a URL string comes
      back non-empty; see the note below)

**Common failure mode worth specifically checking:** if your scraper
pulls image URLs out of `img` tags via `getAttribute()`, make sure you're
resolving them against the page's own origin before returning them (e.g.
`new URL(raw, location.href).href` inside your injected script). A
relative path that works fine when the site renders it in a browser will
silently resolve to the wrong place once it's round-tripped through your
source and displayed in a compiled QML app — it won't error, it'll just
never load.

### What to include in the PR description

1. **Site(s) covered** and what capabilities are implemented (mirror your
   `info.json` — don't make reviewers cross-reference)
2. **Nothing Browser version** you tested against, and whether you're
   depending on any command/behavior not yet in `PROTOCOL.md`
3. **Any registered vendor dependency** — name it explicitly, link its
   `sourceRepoUrl`, and explain briefly why it's needed if it's not
   self-evident
4. **Known limitations** — partial subtitle support, certain content
   types unsupported, regions/languages not covered, etc. Silence on
   limitations is treated as a claim of full coverage.

### Review process

A reviewer will pull your branch and run through the testing matrix in
§1 independently before merging — this isn't optional even for small
sources, since the most common regressions (stale tab state, relative
URLs, platform-specific extraction paths) only show up under exactly
those conditions.
