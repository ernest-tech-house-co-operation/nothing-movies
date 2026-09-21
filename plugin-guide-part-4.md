# Nothing Movies — Plugin Authoring Guide
## Part 4: Cross-Platform Builds and Submitting Your PR

> This part assumes you've built a working source per Parts 1–3. It covers
> what's left before a source is actually shippable: making sure it builds
> cleanly on both supported platforms and that the submission itself is ready.

---

## 0. The rules — read this section first

These are the non-negotiable constraints. Everything else in this doc is
just detail and explanation; this section is the part that gets submissions
rejected if it is skipped.

1. **Nothing Browser is a research tool, not a runtime dependency.**
   It is used to discover the actual API behind a site, then the app calls the
   API directly in C++.
2. **No runtime browser scraping.** No DOM scraping, no selector extraction,
   no browser automation, no page-driven runtime layer, no user-ID driven flows.
3. **A source must be a compiled module with a stable direct API or URL flow.**
   If it only works by opening a browser and scraping a page in the app, it is
   not valid for this project.
4. **Everything must build on both Windows and Linux.**
   If a source only works on one platform, it will be treated as a bad fit unless
   the site itself makes that unavoidable and you document it clearly.
5. **Pure C++ only for the source implementation.**
   No JS, no Python, no shell-based scraping workflow embedded in the app.
6. **CMake must match the actual project layout.**
   File names, casing, and link targets must reflect what is actually on disk.
7. **No browser-based source layer should be reintroduced.**
   The app should not turn into a live scraper or a browser-first runtime.

---

## 1. Cross-platform build checklist

Nothing Movies ships on **Windows** and **Linux**. Every source needs to build and
run correctly on both before it is merged.

### General

- [ ] Builds cleanly on both supported platforms
- [ ] No hardcoded path separators (`/` or `\`) — use `std::filesystem::path`
      for anything touching disk
- [ ] No hardcoded `HOME` / `LOCALAPPDATA` assumptions unless they are truly
      required and the same logic is used consistently across platforms
- [ ] No shelling out to platform-specific commands unless the logic is guarded
      and equivalent on both sides
- [ ] No runtime browser or scraper logic in the production app path

### If your source uses a real API

- [ ] The API call is direct and stable
- [ ] Request/response handling is clean and explicit
- [ ] Failures are handled gracefully instead of crashing or hanging the app
- [ ] Response parsing is robust against missing fields or rate limits

### If your source depends on a helper tool

- [ ] The dependency is minimal and stable
- [ ] It is not a browser runtime scraper layer
- [ ] It is documented clearly in the PR
- [ ] It does not undermine the direct API-only policy of the project

### Testing matrix

At minimum, before opening a PR, run the source through:

| Platform | Fresh install | Existing install | Repeat run |
|---|---|---|---|
| Linux | ✅ | ✅ | ✅ |
| Windows | ✅ | ✅ | ✅ |

Fresh install catches missing dependency/setup issues. Repeat run catches stale state
or data-cache problems.

---

## 2. CMake integration checklist

This is the part that breaks builds when people skip the obvious details.

1. **Make sure the file names in `CMakeLists.txt` match what is actually on disk.**
   CMake source lists are exact string matches; case mismatches can break the build.
2. **If a class uses `Q_OBJECT`, make sure the header is included in the target source list**
   when needed. Missing moc generation leads to confusing linker failures.
3. **Only link what the module actually needs.**
   Keep target dependencies minimal and consistent with the project layout.
4. **Match the existing project conventions.**
   Do not invent a second source layout or a second build pattern just because it looks nicer.

### Minimal rules

- [ ] `.cpp` names match actual files
- [ ] `.h` files are included when required by the build
- [ ] target dependencies are explicit and small
- [ ] CMake matches the folder structure already used by the project

---

## 3. Submitting a PR

### Before you open the PR

- [ ] Source works on both Windows and Linux
- [ ] API flow is direct and stable
- [ ] No browser scraping or selector extraction is used at runtime
- [ ] Source is a normal compiled C++ module and not a browser-driven runtime script
- [ ] No hidden user-ID or automation-based behavior is included
- [ ] The PR description explains what the source does, which APIs are used, and any current limitations

### What to include in the PR description

1. **Which site/service this source covers**
2. **Which API or direct URL flow it uses**
3. **What is implemented** — search, info, stream, download, subtitles, etc.
4. **Known limitations** — region restrictions, incomplete metadata, partial support, etc.
5. **Why it fits the project** — direct API access, stable behavior, no runtime browser scraping

### Review process

A reviewer will verify the project rules before merge. This is not optional. If a
source is browser-based, selector-driven, or scraped at runtime, it will be rejected.
The project wants direct integrations only.

---

## 4. Final note

The browser is a discovery tool, not the app.

Use it to find the API. Build the actual source against that API. Then ship the direct
integration. That is the correct model for Nothing Movies.
