# Adding a Movie Source — Guidelines & Rules

Nothing Movies supports pluggable movie source modules. This document is the
full guide for anyone wanting to build and submit one. Read all of it before
opening a PR — most rejected submissions fail on things covered here.

---

## 1. What a movie source is

A movie source is a self-contained module (`movie_sourceN`) that implements
the shared `core::ISourceProvider` interface — it searches a site/service and
returns results the `search_aggregator` can merge with everything else.

It is **not**:
- A generic "enter any URL" downloader — see [Section 5](#5-no-url-based-sources).
- A wrapper around a single indie/unknown site — see [Section 4](#4-what-sites-we-accept).

---

## 2. Slot limit — only 8 total

The app has a **hard limit of 8 plugable source slots.** This is intentional,
not a technical limitation:

- Fewer, vetted sources means better reliability and less legal/quality risk
  for everyone using the app.
- More sources isn't automatically better — a flaky or shady 9th source hurts
  the app more than it helps.

**2 of the 8 slots are permanently reserved:**

| Slot | Owner | Role |
|---|---|---|
| 1 | Nothing Movies core | The torrent service — the main, primary source. Not up for replacement. |
| 2 | Ernest Tech House | Reserved and maintained by Ernest Tech House, our main sponsor. They provide the infrastructure (including hosting this project under their GitHub organization) and maintain this slot directly. |

**The remaining 6 slots are open for community-submitted sources**, subject to
review and the acceptance criteria below.

---

## 3. Submission is never guaranteed

Building a working source module is not the bar for acceptance. A PR can be
rejected for any of the following, even if the code itself is clean and
functional:

- The underlying site is low quality, unreliable, or "sketchy"
- The site is a small/indie/unknown operation — see Section 4
- A slot isn't currently available (all 6 community slots are filled)
- Legal, safety, or quality concerns raised during review
- Duplicate functionality of an already-accepted source

Acceptance is a judgment call made by maintainers, not an automatic result of
passing tests.

---

## 4. What sites we accept

**Accepted:**
- **Torrent sites** — legitimate, well-known torrent indexers/trackers.
  These fit naturally since the torrent service is already the app's core.
- **Popular, widely-used movie streaming/download sites** — sites with real,
  established user bases. If a large number of people already use and trust
  a site, it's a reasonable candidate.

**Not accepted:**
- Your own personal/indie website, no matter how well it works
- Small, obscure, or unknown sites with no real user base
- Sites that are unreliable, ad-riddled to the point of being unsafe, or that
  show clear signs of malware/scam behavior
- Anything that reads as an attempt to sneak a single person's pet project
  into the app's 6 open slots

**Why this bar exists:** many people use this app. A source slot is a
responsibility, not just a feature — a bad source reflects on the whole
project and can put users at risk.

---

## 5. No URL-based sources

There is **no "download from URL" option** in this app, by design. Every
source must be a **built-in, compiled module in the binary** — not a runtime
field where a user (or a submission) points at an arbitrary link.

This is intentional, for two reasons:
1. **Control over quality** — every source that exists in the app has been
   reviewed and compiled in, not typed in by a user at runtime.
2. **Control over count** — the 8-slot limit only means something if sources
   can't be added around it via a URL field.

If your source requires configuration (API endpoints, etc.), those values are
hardcoded or config-bundled at build time — never exposed as a free-text URL
input to the end user.

---

## 6. Cross-platform requirement

Every submitted source **must work on both Windows and Linux.** No
platform-specific-only sources will be accepted unless there's a genuinely
unavoidable technical reason (rare, and must be clearly documented and
justified in the PR).

- Test on both platforms before submitting.
- If your source needs an external tool/binary, it must be available (or
  fetchable) on both platforms — see Section 7.

---

## 7. If your source needs additional tools

Some sources may need an extra binary or tool beyond core dependencies (e.g.
a headless browser, a specific parser, a CLI tool). If so:

1. **Do not bundle the binary directly in your PR.** Repos should stay light.
2. **Wire it into the existing `vendor_updater` plugable system.** Your
   source module should register a new vendor target (repo, platform tag,
   destination folder) the same way `scraper_core` does for Nothing Browser.
3. **Define the fetch link/repo, not a hardcoded copy.** The auto-updater
   checks defined release links/repos on an interval and keeps the tool
   current — same pattern already used for the Nothing Browser dependency.
4. Your module's `init()` should verify the tool exists and is the expected
   version before attempting to use it, and fail gracefully with a clear
   error if it doesn't.

Example shape (matches the existing `vendor_updater` pattern):

```cpp
vendor_updater::VendorUpdater updater(
    "yourtool/yourtool-repo",      // GitHub repo for your tool
    "vendor/yourtool",             // where it lands locally
    platformTag                    // "windows" or "linux"
);
```

If your source needs a tool that has no public releases to auto-update from,
that's a strong signal it's not ready for submission yet.

---

## 8. Testing is mandatory

Before submitting:

- [ ] Source builds cleanly on both Windows and Linux
- [ ] Search returns real, correctly-formatted results
- [ ] `getStreamUrl()` returns a working, playable stream/download link
- [ ] Any external tool dependency is wired to `vendor_updater`, not bundled
- [ ] No hardcoded personal API keys, credentials, or anything that only
      works on your machine
- [ ] You've tested with the app actually running, not just unit tests

"It compiles" is not the same as "it works." PRs without evidence of real
testing (screenshots, a short demo, or clear notes on what was tested) will
be sent back before review even starts.

---

## 9. Submission checklist

When opening a PR for a new `movie_sourceN` module, include:

1. Which site/service this source pulls from, and why it qualifies under
   [Section 4](#4-what-sites-we-accept) (torrent site or genuinely popular
   site — link to evidence of its user base if not obvious)
2. Confirmation it's tested on both Windows and Linux
3. Any additional tool dependencies and how they're wired to `vendor_updater`
4. A note on current slot availability — check open issues/discussions for
   the current count of filled community slots before assuming one's open

---

## 10. Final note

We built this because we're not hiding anything and we're not doing anything
new — every technique here is public and well understood. That doesn't mean
every source gets in. Quality and trust matter more than source count. If
your PR gets rejected, it's usually about the site, not your code.

## 11. Language requirement — pure C++, no exceptions

Every movie source module must be written **entirely in C++.** No Python, no
JS, no shell scripts doing the actual logic, no mixed-language wrappers. This
matches the rest of the project — one language, top to bottom, no exceptions
for source modules either.

If your source relies on a tool that isn't C++ (e.g. an external binary),
that's fine — see Section 7 — but the module code itself, the part that
implements `ISourceProvider`, must be pure C++.

## 12. Your source needs its own git repo

Every submitted source module must live in its own Git repository, not just
exist as a folder dropped into a PR. This is required so:

- The source has real version history independent of the main project
- Changes can be tracked and attributed properly
- The source can be referenced/pulled as its own unit if needed

Link your repo in the PR description.

## 13. Ownership and licensing — read before submitting

Accepting a movie source module into Nothing Movies is not like a normal
open source contribution. By submitting a source for acceptance, you agree
to the terms in [`MOVIE_SOURCE_LICENSE.md`](./MOVIE_SOURCE_LICENSE.md).

In short:
- Your source becomes joint property between you and Nothing Movies /
  Ernest Tech House once accepted.
- Any lead programmer on the team can modify your source directly.
- If changes are made to your source within the main project, you are
  expected to pull those changes back into your own repo — this keeps your
  repo and the in-app copy from drifting apart.
- Your name will be credited in-app under source providers.

Read the full license before submitting — accepted sources are bound by it
automatically, whether or not you've read it.