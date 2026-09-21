# Nothing Movies — Plugin Authoring Guide
## Part 3: Source Discovery and Direct API Integration

> This part covers how to build a movie source the correct way in Nothing Movies:
> use Nothing Browser only as a research and reverse-engineering tool to find the API,
> then write a clean direct integration in C++ that does not depend on browser scraping
> at runtime. Read Part 1 and Part 2 first.

---

## The rules — read this section first

1. **Nothing Browser is a research tool, not a runtime source engine.**
   It is used to inspect a site, capture network requests, and discover the API behind it.
   It is not a shipped app component and it is not meant to be used as a production
   scraper layer inside the app.
2. **Every source must use direct API or direct URL access.**
   No browser automation, no selector scraping, no DOM parsing, no user-ID tricks,
   no runtime browser-driven extraction. If you need to run a browser to do the work
   at runtime, the source is not valid for this project.
3. **The final app must call the API directly in C++.**
   The browser workflow is only the discovery phase: use it once to find the endpoint,
   then implement the same request cleanly in the source module.
4. **Do not build scraper-like behavior into the shipped app.**
   These sites rotate, block, and change constantly. A persistent browser-based source
   layer would become a never-ending cat-and-mouse fight with anti-bot systems,
   Cloudflare, fingerprinting, and session churn.
5. **If a source depends on a binary, it must be a minimal, stable external dependency,
   not a runtime scraping browser.** Keep it narrow and direct.
6. **The source must still be a built-in compiled module.**
   No free-text URL injection or user-managed "paste a site URL and let it scrape" workflow.

---

## What Nothing Browser actually is

Nothing Browser is a scraper-first reverse-engineering browser designed to expose the
underlying web traffic behind a site. In plain English: it helps answer the question
"what API is this website actually calling?"

It is useful for:

- inspecting API requests and responses
- capturing cookies, storage, and websocket traffic
- exporting a working request into curl, fetch, or Python for reproduction
- discovering the real direct API behind a website that looks like a normal UI

It is not useful as a production source layer because:

- sites rotate, block, and change constantly
- anti-bot layers evolve faster than the app can keep up
- browser-based scraping is slow, fragile, and expensive to maintain
- the app should call a direct API once it is discovered, not keep a browser open to scrape forever

The browser is therefore the research tool, not the runtime architecture.

---

## The correct workflow

1. Open Nothing Browser and inspect the target site.
2. Capture the actual request that returns the data you need.
3. Find the direct API endpoint, auth pattern, and payload shape.
4. Export the working call and reproduce it in C++.
5. Ship a clean direct integration in `movie_sourceN`.
6. Remove the browser from the runtime path completely.

This is the important distinction:

- **Nothing Browser is used to discover the API**
- **Nothing Movies uses the API directly**

---

## What a valid source looks like

A valid source module does this:

- implements `core::ISourceProvider`
- uses standard C++ networking and parsing
- calls a direct API or direct URL endpoint
- returns parsed data to the aggregator
- does not depend on browser automation at runtime
- does not query a DOM or parse selectors at runtime

A valid source does not do this:

- run a browser in the app
- scrape a page at runtime
- depend on selectors, DOM extraction, or user-ID flows
- rely on page automation to defeat Cloudflare or other anti-bot systems
- ship a scraper-based plugin as if it were a normal source module

---

## Source discovery pattern

When you are investigating a site, the browser can help. But the source module itself
must eventually become a clean API client. In other words:

- research: browser
- implementation: direct C++ API client
- production runtime: direct API calls only

This is the project policy and it is intentional.

---

## Direct API only: what you are allowed to build

Your source may use:

- libcurl / HTTP requests
- JSON parsing
- direct API endpoints
- direct media URL fetching
- stable public APIs with predictable contracts

Your source may not use:

- browser automation in the app
- selector scraping or DOM-based extraction
- page rendering as part of the core app runtime
- a continuing browser-driven anti-bot process in production

---

## Full source structure

```text
movie_source2/
├── CMakeLists.txt
├── include/movie_source2/movie_source2.h
├── src/movie_source2.cpp
└── data/
    └── info.json
```

Nothing changes here from other sources — the implementation detail is what matters.
Your source is still a built-in compiled module, but the data path must be direct and
stable, not browser-driven.

```cmake
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

add_library(movie_source2 STATIC src/movie_source2.cpp)
target_include_directories(movie_source2 PUBLIC include)
target_link_libraries(movie_source2 PUBLIC
    core
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

## What goes in `info.json`

Same format as any other source — the implementation detail is hidden behind the module:

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

## Submission checklist for direct API sources

Everything in Part 1 and Part 2 applies, plus:

- [ ] Uses direct API or direct URL integration
- [ ] No browser scraping at runtime
- [ ] No selector-based extraction or DOM parsing
- [ ] No user-ID flow or browser automation dependency
- [ ] Source is built in as a normal compiled C++ module
- [ ] Works on both Windows and Linux
- [ ] Has a stable API contract and does not rely on runtime browser control
- [ ] Does not reintroduce a persistent scraper-like runtime layer

---

## Final note

Nothing Browser is the origin story of the API discovery process. It is how people find
what the website is actually doing. But the app itself is not a browser, and it is not
meant to become one.

The goal is simple: use the browser to find the API, then replace it with a clean direct
C++ implementation. That is the correct architecture for Nothing Movies.

---

## Part 4 preview

Part 4 covers the actual app-level source conventions, module wiring, and the project rules
that keep source submissions disciplined and consistent.

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

If your source also registers a vendor dependency directly (rather than
relying on `scraper_core` for Nothing Browser specifically), add
`vendor_updater` to `target_link_libraries(...)` too — see Part 4 §3 for
what breaks if you skip this.

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
- [ ] Any external binary dependency registered with `VendorManager` (not bundled, not downloaded manually)
- [ ] `VendorSpec::sourceRepoUrl` set to a real, public, open-source repository
- [ ] Handles `daemonRecovered` sensibly if the source holds long-lived tab state
- [ ] Nothing Browser version requirement documented in the PR
- [ ] If you touched `CMakeLists.txt` for this: matches the checklist in Part 4 §3 (correct file casing, `Q_OBJECT` headers listed for AUTOMOC, `vendor_updater` linked if used)

---

## Part 4 preview

Part 4 covers:
- Windows + Linux cross-platform build checklist for Nothing Browser sources
- The `VendorManager` / `vendor_updater` integration in full detail, including
  exactly how the shared vendor root folder is pinned for this app
- The CMake failure modes you'll hit if you skip the checklist above, and
  how to fix each one
- Submitting a PR with a Nothing Browser source