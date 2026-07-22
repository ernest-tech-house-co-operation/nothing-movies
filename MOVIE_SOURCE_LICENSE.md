# Movie Source License

**Drafted by Ernest Tech House. Terms may change at any time, without prior
notice. Continued participation as a source provider after a change means
acceptance of the updated terms.**

This license governs any `movie_sourceN` module submitted to and accepted
into the Nothing Movies project. It exists alongside, not instead of, the
project's main `LICENSE.md` (PolyForm Noncommercial 1.0.0) and
`ADDITIONAL_TERMS.md`. By submitting a source module for review, and by
having it accepted, you agree to these terms.

---

## 1. Acceptance and credit

If your movie source is accepted into Nothing Movies:

- Your name (or the name/handle you provide) will be featured in-app under
  source providers.
- Your source becomes a bundled, built-in part of the application, subject
  to the project's overall 8-slot limit and all rules in `MOVIE_SOURCE.md`.

## 2. Shared ownership

Once accepted, your source is no longer solely yours in the codebase sense:

- **Your source is also our source.** Nothing Movies / Ernest Tech House
  gains the right to maintain, modify, adapt, and ship your source module
  as part of the app, without needing to ask permission for each change.
- **Any lead programmer on the team may modify your source directly** —
  bug fixes, adapting to site changes, performance work, integration
  changes, or anything else needed to keep it working and consistent with
  the rest of the codebase.

You are not losing credit for having built it — you are giving up exclusive
control over it once it's part of the app.

## 3. Keeping your own repo in sync

Your source must live in its own git repository (see `MOVIE_SOURCE.md`,
Section 12). If changes are made to your source within the main project —
by a lead programmer, for a fix, an update, or any other reason — **you
should employ those same changes back into your own repository.**

This keeps your standalone repo and the in-app version from diverging, and
keeps your source directly reusable/forkable off your own repo, consistent
with what actually ships.

## 4. Requirements recap (see `MOVIE_SOURCE.md` for full detail)

- Must be pure C++
- Must work on both Windows and Linux
- Must have its own git repository
- Must not use a free-text URL input — built into the binary only
- Additional tools must be wired to the `vendor_updater` plugable system
- Site must meet the acceptance criteria (torrent sites or genuinely popular
  sites — not indie/personal sites)

## 5. No guarantee of permanence

Acceptance today does not guarantee your source stays in the app forever.
Ernest Tech House and the Nothing Movies maintainers reserve the right to:

- Remove a source at any time (site becomes unreliable, unsafe, legally
  risky, or is superseded)
- Modify a source without prior consultation with the original author
- Reassign or restructure source slots as needed

## 6. This license can change

**Ernest Tech House drafted this license and may change it at any time,
without notice.** If you're an active source provider, it's on you to check
back on this document periodically. Continuing to have your source shipped
in the app after a change constitutes acceptance of the new terms.

## 7. Relationship to the main project license

This license does not replace `LICENSE.md` (PolyForm Noncommercial 1.0.0) or
`ADDITIONAL_TERMS.md` — it sits alongside them, specifically for movie
source contributions. Where something isn't addressed here, the main
project license and additional terms still apply.

---

*Nothing Movies — We have what? Everything.*
*Movie Source License drafted and maintained by Ernest Tech House.*