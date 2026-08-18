# Nothing Movies — Plugin Authoring Guide
## Part 4: Cross-Platform Builds, VendorManager, and Submitting Your PR

> This part assumes you've built a working source per Parts 1–3. It
> covers what's left before it's actually shippable: making sure it
> builds cleanly on both supported platforms, wiring any external binary
> dependency into `VendorManager` correctly, and what a submission PR
> needs to include.

---

## 0. The rules — read this section first

These are the non-negotiable constraints. Everything else in this doc is
detail and explanation; this section is what actually gets your PR
rejected or your build broken if skipped.

1. **Nothing Browser is the only scraping engine** (Part 3). No second
   scraping tool, ever, for any source.
2. **Every external binary dependency — Nothing Browser or anything
   else — goes through `vendor_updater::VendorManager::registerVendor()`.**
   Never construct a `VendorUpdater` directly and never write your own
   download/extract logic. A second, independently-constructed instance
   is exactly how a vendor's files silently end up in two different
   places on disk — this already happened once in this codebase and cost
   a real debugging session to fix.
3. **`VendorSpec::sourceRepoUrl` is mandatory.** Registration is flatly
   refused without it. There is no override.
4. **Every vendor's files live under one shared root, no exceptions:**
   `<appDir>/nothing/<vendor name>/`. This is pinned once, at process
   startup, by `main.cpp` setting the `NOTHINGMOVIES_VENDOR_ROOT`
   environment variable **before** anything calls `registerVendor()` —
   see §2 for why this matters and what breaks if it's skipped or
   reordered.
5. **`VendorSpec::name` may not contain `/`, `\`, or `..`.** Registration
   is refused otherwise (this is what stops a name from escaping the
   shared root folder).
6. **CMake — link `vendor_updater`.** Any module (`ui`, a movie source,
   whatever) that includes `vendor_updater/VendorManager.h` or
   `VendorUpdater.h` must add `vendor_updater` to its own
   `target_link_libraries(...)`. Without it you get a plain "file not
   found" on the `#include`, even though the header exists on disk —
   CMake didn't give the compiler an include path to it.
7. **CMake — list `Q_OBJECT` headers explicitly if they live in a
   different folder than their `.cpp`.** This project keeps headers in
   `include/<module>/` and sources in `src/`. AUTOMOC's automatic
   header-discovery assumes the header sits next to the `.cpp`; when it
   doesn't, list the header directly in `add_library(...)`'s source list
   (every existing bridge in `ui/CMakeLists.txt` already does this — copy
   the pattern). Skipping this doesn't fail at configure or compile
   time — it fails at **link** time, with a confusing
   `undefined reference to vtable for YourClass` error, because `moc`
   silently never ran on that header at all.
8. **Match existing file-naming casing exactly, per module.** This
   codebase is not consistent project-wide: `vendor_updater` uses
   lowercase-with-underscores `.cpp` filenames (`vendor_updater.cpp`)
   while `ui` uses PascalCase (`QueueBridge.cpp`) — except `MainWindow.cpp`,
   which someone renamed to `main_window.cpp` in that same module. Before
   adding a new file to any module, check what its *siblings* are
   actually named on disk and match that, not what "seems consistent."
   CMake source lists are exact string matches — no case-insensitive
   fallback.

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
      is pinned for this app (see §2), don't invent a second convention
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
      **both** `windows` and `linux` at minimum, matching whatever
      `platformTag()`-equivalent your registration uses (see §2 — this
      codebase's `platformTag()` returns exactly `"windows"` or
      `"linux"`, not a more specific string like `"linux-x86_64"`) —
      sources that only work on one platform won't be accepted unless
      the target site itself is platform-exclusive in some way that
      makes this unavoidable (rare — flag it explicitly in your PR if so)
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
- **this actually happened in this exact codebase**: `scraper_core`
  originally constructed its own private `VendorUpdater` pointed at
  `<appDir>/nothing`, while a separate piece of setup code constructed a
  second one pointed at `vendor/nothing-browser` (relative to whatever
  the process's cwd happened to be). They silently diverged, and the
  vendored binary was never found. `VendorManager` exists specifically
  so there is exactly one owner of "where does this vendor's stuff live."

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
| `platformTag` | Yes | Substring that must appear in the release asset filename to match this platform. **In this codebase, use exactly `"windows"` or `"linux"`** (see `scraper_core`'s own `platformTag()` for the canonical example) — don't invent a more specific string like `"linux-x86_64"` unless your actual release assets are named that way. |
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

### Where files actually end up — and how that's guaranteed

`VendorManager::rootDir()` has a generic fallback baked into the class
itself (per-OS user-data directory), **but this app does not use that
fallback in practice.** `main.cpp` pins it explicitly, once, at startup,
before anything registers a vendor:

```cpp
// main.cpp — must run before browser->start() or any other
// registerVendor() call. Reordering this breaks the pin silently:
// whichever registration happens first would fall through to
// VendorManager's generic per-OS default instead.
void setVendorRootEnv() {
    const QString root = QDir(QCoreApplication::applicationDirPath()).filePath("nothing");
    qputenv("NOTHINGMOVIES_VENDOR_ROOT", root.toUtf8());
}
```

This makes `<appDir>/nothing` the real, load-bearing root for this app —
not just a suggestion or a dev convenience. Every vendor's own code (like
`scraper_core`'s `vendorDirPath()`, which computes where it expects the
Nothing Browser binary to actually be) has to independently agree with
this same path. If you're registering a new vendor from a new module,
you don't need to duplicate this env-setting logic — it's already done
once, globally, in `main.cpp` — but if your module also independently
computes a path to check `isAvailable()`-style before the binary is
downloaded, that computation must resolve to the *same* place
`VendorManager` will actually put it: `<appDir>/nothing/<your vendor
name>/`.

Concretely, for `nothing-browser`:

```
<appDir>/nothing/
├── manifest.json                     # every registered vendor's declared metadata
├── nothing-browser/                   # this vendor's live, in-use install
│   ├── .version                       # last-applied release tag
│   └── nothing-browser-headless       # the actual binary scraper_core spawns
├── nothing-browser_staging/            # transient — extraction happens here first
└── nothing-browser_prev/               # transient — old install, kept only during a swap
```

If you register a second vendor tool from a different module, it lands
as a sibling: `<appDir>/nothing/<that vendor's name>/`, same root,
different subfolder — never anywhere else.

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
    "platformTag": "linux",
    "license": "MIT"
  }
]
```

This exists specifically so the "what's actually running on my machine"
question has an answer that doesn't require opening the app — anyone can
read this file directly.

### Registering is idempotent — call it freely

`registerVendor()` is safe to call every time your module initializes,
even across multiple app runs or multiple code paths that might both try
to set up the same vendor. A duplicate `name` is simply rejected (logged,
not fatal) without touching the existing registration. You don't need to
guard calls with your own "have I already registered this" check —
`VendorManager` already does that for you.

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

## 3. CMake integration — the part that actually breaks builds

This section exists because every single build error hit while wiring
`VendorManager` into this codebase for real was a CMake/build-system
issue, not a logic bug. If your source registers a vendor dependency
(or adds any new `QObject`-derived bridge class), expect to hit these
same three failure modes if you skip this checklist:

1. **`#include "vendor_updater/VendorManager.h"` fails to find the file**,
   even though it exists on disk exactly where the compiler says it's
   looking. Fix: add `vendor_updater` to your module's
   `target_link_libraries(...)`. Header search paths in this project
   come from `target_include_directories(vendor_updater PUBLIC include)`
   inside `vendor_updater`'s own `CMakeLists.txt` — but that only
   propagates to modules that actually link against `vendor_updater`.
2. **`CMake Error ... Cannot find source file: src/whatever.cpp`** at
   the *configure* step (before any compilation happens). This is a
   filename/casing mismatch between what's on disk and what
   `add_library(...)` lists — check both against each other character
   by character. This project mixes casing conventions between modules
   (see rule 8 in §0); don't assume a class's `.cpp` matches its
   `.h`'s casing.
3. **`undefined reference to vtable for YourClass`** at the *link*
   step (the build otherwise completes — this is the sneakiest one,
   since it looks like everything worked until the very last step).
   This means `moc` never ran on a header containing `Q_OBJECT`. Fix:
   explicitly list that header in `add_library(...)`'s source list,
   immediately next to its `.cpp` — see any existing bridge in
   `ui/CMakeLists.txt` for the pattern to copy.

### Minimal checklist for adding a new `QObject`-derived bridge to `ui`

- [ ] `.cpp` added to `add_library(ui STATIC ...)`
- [ ] Matching `.h` **also** added to the same list, right next to it
- [ ] Filename casing on both matches what's actually on disk, not what
      "looks right"
- [ ] If the bridge touches `vendor_updater` types, `vendor_updater` is
      in `ui`'s `target_link_libraries(...)`
- [ ] Registered as a QML context property wherever the others are
      (`MainWindow.cpp`, via `rootContext()->setContextProperty(...)`)
- [ ] Threaded through `MainWindow`'s constructor parameter list and
      header member list consistently — both files need the new
      parameter, in the same position

---

## 4. Submitting a PR with a Nothing Browser source

### Before you open the PR

- [ ] Everything in the Part 3 submission checklist
- [ ] Everything in the §1 cross-platform checklist above
- [ ] Everything in the §3 CMake checklist above, if you touched the build
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