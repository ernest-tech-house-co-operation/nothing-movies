# Security Policy

Nothing Movies auto-downloads and executes third-party binaries
(`vendor_updater`), scrapes external websites (`scraper_core`), and handles
torrent/network traffic (`torrent_service`). That surface area means
security reports matter a lot here — please report privately, not as a
public issue.

---

## Reporting a vulnerability

**Do not open a public GitHub issue for security vulnerabilities.**

Email: **ernesttechhouse@gmail.com**

Include as much of the following as you can:

- A clear description of the vulnerability and its impact
- Steps to reproduce, or a proof-of-concept if you have one
- Which module is affected (`vendor_updater`, `scraper_core`,
  `torrent_service`, a specific `movie_sourceN`, `ui`, etc.)
- Your OS and app version
- Whether you believe this is being actively exploited

You should get an acknowledgment within **72 hours.** If you don't hear
back in that window, it's fine to follow up on the same email thread.

## What happens after a report

1. We confirm receipt and begin investigating.
2. We work to identify the root cause and a fix.
3. We'll keep you updated on progress if you want updates.
4. Once a fix is ready, we release it and — unless you ask us not to —
   credit you in the release notes / `CHANGELOG.md` for the find.
5. Public disclosure happens **after** a fix is shipped, not before,
   unless the issue is already public elsewhere.

We ask that you give us reasonable time to fix an issue before disclosing
it publicly. In turn, we won't sit on a real report indefinitely.

---

## Areas that deserve extra scrutiny

Given the architecture, these are the highest-value places to look, and
the ones we take most seriously:

### `vendor_updater` (auto-fetches and runs external binaries)
- Anything that could let a malicious actor serve a fake "latest release"
  and get it executed on a user's machine (e.g. DNS/TLS issues, insufficient
  verification of what's downloaded, path traversal during archive
  extraction)
- The staging → backup → swap process is designed so a corrupted or
  malicious download shouldn't silently replace a working vendor binary —
  if you find a way around that, we want to know

### `scraper_core` (drives the vendored browser binary)
- Anything that could let a scraped page escalate beyond rendering/network
  capture into executing code on the host, exfiltrating local files, or
  accessing things outside its intended sandboxed scope

### `torrent_service` (libtorrent-rasterbar wrapper)
- Anything exposing the user's IP/traffic beyond expected torrent behavior,
  or allowing a malicious peer/tracker to affect the host process itself
  (not just normal torrent-protocol behavior, which is out of scope — see
  below)

### `movie_sourceN` modules
- Any accepted source that turns out to serve malware-laced content,
  deceptive redirects, or credential-harvesting pages. See
  `MOVIE_SOURCE.md` — sources can and will be pulled immediately if this
  is found, no 32-day grace period like the license violation clause;
  user safety comes first.

### Downloaded/streamed content itself
- Malformed media files or streams designed to exploit the player
  (`libmpv`/ffmpeg parsing bugs) are in scope — this is a well-known class
  of real-world media player vulnerabilities and we take it seriously.

---

## Out of scope

- Normal, expected behavior of the BitTorrent protocol (e.g. peers seeing
  your IP while torrenting — that's inherent to the protocol, not a bug
  in this app)
- Vulnerabilities in the vendored Nothing Browser binary itself — please
  report those directly to the [Nothing Browser project](https://github.com/BunElysiaReact/nothing-browser/blob/main/SECURITY.md)
  instead, since we don't control that codebase
- Vulnerabilities in third-party movie source *websites* themselves (report
  those to the site operators) — our concern is how our source module
  handles them, not the site's own security
- Issues requiring physical access to an already-compromised machine

---

# Security Policy

Nothing Movies auto-downloads and executes third-party binaries
(`vendor_updater`), scrapes external websites (`scraper_core`), and handles
torrent/network traffic (`torrent_service`). That surface area means
security reports matter a lot here — please report privately, not as a
public issue.

---

## Reporting a vulnerability

**Do not open a public GitHub issue for security vulnerabilities.**

Email: **ernesttechhouse@gmail.com**

Include as much of the following as you can:

- A clear description of the vulnerability and its impact
- Steps to reproduce, or a proof-of-concept if you have one
- Which module is affected (`vendor_updater`, `scraper_core`,
  `torrent_service`, a specific `movie_sourceN`, `ui`, etc.)
- Your OS and app version
- Whether you believe this is being actively exploited

You should get an acknowledgment within **72 hours.** If you don't hear
back in that window, it's fine to follow up on the same email thread.

## What happens after a report

1. We confirm receipt and begin investigating.
2. We work to identify the root cause and a fix.
3. We'll keep you updated on progress if you want updates.
4. Once a fix is ready, we release it and — unless you ask us not to —
   credit you in the release notes / `CHANGELOG.md` for the find.
5. Public disclosure happens **after** a fix is shipped, not before,
   unless the issue is already public elsewhere.

We ask that you give us reasonable time to fix an issue before disclosing
it publicly. In turn, we won't sit on a real report indefinitely.

---

## Areas that deserve extra scrutiny

Given the architecture, these are the highest-value places to look, and
the ones we take most seriously:

### `vendor_updater` (auto-fetches and runs external binaries)
- Anything that could let a malicious actor serve a fake "latest release"
  and get it executed on a user's machine (e.g. DNS/TLS issues, insufficient
  verification of what's downloaded, path traversal during archive
  extraction)
- The staging → backup → swap process is designed so a corrupted or
  malicious download shouldn't silently replace a working vendor binary —
  if you find a way around that, we want to know

### `scraper_core` (drives the vendored browser binary)
- Anything that could let a scraped page escalate beyond rendering/network
  capture into executing code on the host, exfiltrating local files, or
  accessing things outside its intended sandboxed scope

### `torrent_service` (libtorrent-rasterbar wrapper)
- Anything exposing the user's IP/traffic beyond expected torrent behavior,
  or allowing a malicious peer/tracker to affect the host process itself
  (not just normal torrent-protocol behavior, which is out of scope — see
  below)

### `movie_sourceN` modules
- Any accepted source that turns out to serve malware-laced content,
  deceptive redirects, or credential-harvesting pages. See
  `MOVIE_SOURCE.md` — sources can and will be pulled immediately if this
  is found, no 32-day grace period like the license violation clause;
  user safety comes first.

### Downloaded/streamed content itself
- Malformed media files or streams designed to exploit the player
  (`libmpv`/ffmpeg parsing bugs) are in scope — this is a well-known class
  of real-world media player vulnerabilities and we take it seriously.

---

## Out of scope

- Normal, expected behavior of the BitTorrent protocol (e.g. peers seeing
  your IP while torrenting — that's inherent to the protocol, not a bug
  in this app)
- Vulnerabilities in the vendored Nothing Browser binary itself — please
  report those directly to the [Nothing Browser project](https://github.com/BunElysiaReact/nothing-browser/blob/main/SECURITY.md)
  instead, since we don't control that codebase
- Vulnerabilities in third-party movie source *websites* themselves (report
  those to the site operators) — our concern is how our source module
  handles them, not the site's own security
- Issues requiring physical access to an already-compromised machine

---

## Supported versions

Only the **latest released version** of Nothing Movies is supported with
security fixes. Given how fast `vendor_updater` and the app itself iterate,
we don't backport fixes to older releases — please stay updated.

---

## A note on trust boundaries

This app runs a third-party binary (Nothing Browser) that we don't control
the source of, per our earlier design decision — see `README.md`. We treat
it as an external dependency with its own release cadence and its own
security process. If you find something that's genuinely in Nothing
Browser rather than in how we invoke/wrap it, please also flag it to them
directly, per their own security policy.

---

*Nothing Movies — We have what? Everything.*
*Security contact: ernesttechhouse@gmail.com*