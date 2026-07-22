# Contributing to Nothing Movies

Thanks for wanting to help build this. Before anything else, read this
document fully — it tells you where to actually start depending on what
you want to work on.

---

## 1. Read the ground rules first

Before writing any code, read these in order:

1. [`WARNING.md`](./WARNING.md) — what this project is and isn't for
2. [`LICENSE.md`](./LICENSE.md) — PolyForm Noncommercial 1.0.0
3. [`ADDITIONAL_TERMS.md`](./ADDITIONAL_TERMS.md) — our extra conditions
4. If you're submitting a movie source specifically:
   [`MOVIE_SOURCE.md`](./MOVIE_SOURCE.md) and
   [`MOVIE_SOURCE_LICENSE.md`](./MOVIE_SOURCE_LICENSE.md)

If you don't agree with these, please don't open a PR — it'll just get
closed, and that wastes both our time.

---

## 2. What kind of contribution are you making?

### A) Core modules (player, torrent_service, ui, downloader, etc.)
Bug fixes, performance improvements, UI polish, feature work on existing
modules. This is normal open-source-style contribution:

- Fork the repo
- Branch off `main`: `git checkout -b fix/short-description`
- Make your change
- Open a PR against `main` with a clear description of what changed and why
- One logical change per PR — don't bundle unrelated fixes together

### B) A new movie source
Do not start coding yet. Go read `MOVIE_SOURCE.md` in full first — there's
a hard 8-slot limit, 2 slots are already reserved, and not every working
source gets accepted even if the code is clean. Sites are the usual reason
for rejection, not code quality.

### C) Reporting a bug
Open an issue. Include:
- OS (Windows or Linux) and version
- Steps to reproduce
- What you expected vs what happened
- Your exact version string from in-app Settings (e.g. `v0.0.1-beta` or
  `v0.0.1`) — beta and official builds are triaged differently
- Logs/output if you have them, especially anything from `vendor_updater`
  or `scraper_core` if it's scraping-related

### D) A feature idea
Open a discussion or issue before writing code — for anything nontrivial,
it's worth confirming the direction fits before you sink time into it.

---

## 3. Code standards

- **Pure C++** across the whole project — no exceptions, no bolted-on
  scripting languages, no shortcuts in JS/Python. This applies to core
  modules and movie sources alike.
- **Module isolation** — don't reach into another module's `src/`. Only
  talk through the `include/` interface headers. If you need something
  from another module that isn't exposed, that's a sign the interface
  needs extending, not a reason to bypass it.
- **CMake** — every module has its own `CMakeLists.txt`. New modules follow
  the same structure as existing ones (see any `movie_source1`-style folder
  for the pattern).
- **Cross-platform by default** — Windows and Linux both, always. If you
  can only test one, say so clearly in the PR and flag it for someone on
  the other platform to verify before merge.
- **No secrets, no personal API keys, no hardcoded machine-specific paths.**

---

## 4. Commit and PR etiquette

- Write commit messages that explain *why*, not just *what*
  (`fix: correct sequential piece priority for streaming` beats `fix bug`)
- Keep PRs focused — reviewers can actually review a 100-line change, not
  a 2000-line one touching six unrelated modules
- Be ready for changes to be requested — this is normal, not personal
- Be patient. This is maintained by real people with limited time, not a
  company with a dedicated review team

---

## 5. On ownership, once merged

For core modules: standard open-source norms apply — you keep authorship
credit, the code lives under the project's license going forward.

For movie sources specifically: acceptance means different, stricter terms
around shared ownership and modification rights. Read
`MOVIE_SOURCE_LICENSE.md` — don't skip it, don't assume it works like a
normal PR.

---

## 6. Questions

Open a discussion thread rather than emailing directly for general
questions — keeps answers visible for the next person with the same
question. For security issues, see `SECURITY.md` instead of a public issue.

---

*Nothing Movies — We have what? Everything.*