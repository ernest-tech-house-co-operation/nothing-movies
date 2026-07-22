# ⚠️ WARNING — READ BEFORE CONTRIBUTING OR USING

## This project is not a license to break the law

Nothing Movies is open source because there is nothing novel or hidden in it.
Every technique used here — torrent streaming, HTTP downloading, video
playback, browser automation for scraping — is public, well-documented, and
used by thousands of other legitimate open source projects. Being open source
is a transparency decision, not a permission slip.

**This project is not meant to be exploited.** That includes, but is not
limited to:

- Using this codebase to build a tool whose primary purpose is mass piracy
  distribution
- Submitting `movie_sourceN` modules that scrape or proxy sites you know to be
  illegal, malicious, or operating in bad faith
- Using the scraper/automation layer to bypass paywalls, DRM, or access
  controls on services you have not been authorized to access
- Repackaging this project, or forks of it, as a "clean" front for piracy
  while hiding the actual source list from users

## Movie source modules are not automatically accepted

Whether a given `movie_sourceN` pull request gets merged is **actively under
debate on a case-by-case basis.** A source compiling and returning results is
not the bar. Maintainers reserve the right to reject, remove, or quarantine
any source module for any reason, including but not limited to:

- Legal risk to users or maintainers
- The source itself operating maliciously (malware-laced streams, deceptive
  ads, credential harvesting)
- Lack of transparency about where content is actually coming from
- Community or legal pressure after merge

**Merged today does not mean permanent.** A source can be pulled at any time
if concerns are raised later.

## Your responsibility as a user

- You are responsible for how you use this software in your jurisdiction.
- Nothing Movies does not host, seed, or store any copyrighted content itself.
  It is a client — what you point it at is on you.
- If you fork this project and add sources or behavior that violate this
  warning, that fork is **yours**, not this project's, and this project's
  maintainers bear no responsibility for it.

## Why this is open source anyway

Because hiding the code doesn't make the intent good, and showing the code
doesn't make the intent bad. Nothing here is a secret technique or a novel
exploit — libtorrent, ffmpeg, Qt, and browser automation are all standard,
publicly documented tools. Being upfront about exactly how this works is the
whole point. Use that transparency responsibly.

---

**If you don't agree with the above, do not use, build, or contribute to this
project.**