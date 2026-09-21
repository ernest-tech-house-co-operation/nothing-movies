#pragma once
#include "core/ISourceProvider.h"
#include <vector>
#include <string>

// ── Plugin guide reference ────────────────────────────────────────────────
// This source is a hasInfo: FALSE torrent indexer (Apibay).
// See: nothing-movies-plugin-guide-part1.md — getHomepage and getMediaInfo
// See: nothing-movies-plugin-guide-part2.md — search, getStreamUrl, routing
//
// Because hasInfo is false:
//   - search() returns raw torrent names — SearchBridge cleans them via
//     TitleCleaner and enriches them with TMDB posters automatically.
//   - getMediaInfo() is never called — stubbed to return {}.
//   - getSubtitleUrls() is never called — stubbed to return {}.
//   - getHomepage() is never called — stubbed to return {}.
//   - getStreamUrl() returns a magnet URI — QueueBridge routes it to the
//     torrent manager (streamType: "torrent" in capabilities).
// ─────────────────────────────────────────────────────────────────────────

namespace movie_source1 {

class ApibayProvider : public core::ISourceProvider {
public:
    ApibayProvider() = default;

    // ── Required by ISourceProvider ──────────────────────────────────────

    // Returns the source's capability flags — read by HomepageBridge,
    // SearchBridge, and QueueBridge to decide what to show and how to route.
    // Plugin guide part 1: "The manifest is the contract between plugin and UI."
    core::SourceCapabilities getCapabilities() const override;

    // Hits apibay.org/q.php, parses JSON, returns raw torrent results.
    // SearchBridge will clean titles and fetch TMDB posters automatically
    // because hasInfo is false. Plugin guide part 2: "search pipeline."
    std::vector<core::MediaResult> search(const std::string& query) override;

    // Given an info_hash (the id set during search()), re-fetches the torrent
    // name from apibay.org/t.php and builds a full magnet URI with trackers.
    // QueueBridge routes this to torrent_service because streamType = "torrent".
    // Plugin guide part 2: "getStreamUrl — returning a playable URL."
    std::string getStreamUrl(const std::string& id) override;

    // ── Not implemented for this source — hasInfo: false ─────────────────

    // Never called by the app — hasInfo: false means no info pages.
    // Plugin guide part 1: "stub it to return {} for anything you don't support."
    std::vector<core::HomepageItem> getHomepage() override { return {}; }
    core::MediaInfo getMediaInfo(const std::string&) override { return {}; }

    // Never called — hasSubtitles: false. Torrents that ship a matching
    // .srt next to the video are auto-loaded by mpv's sub-auto behavior.
    // Plugin guide part 2: "getSubtitleUrls — optional subtitle support."
    std::vector<std::string> getSubtitleUrls(const std::string&) override { return {}; }

    // Never called — hasHomepage: false. Apibay has no curated homepage.
    // Plugin guide part 1: "if hasHomepage is false, TMDB fills the home screen."

private:
    std::string detectQuality(const std::string& name) const;
    std::string buildMagnet(const std::string& infoHash, const std::string& displayName) const;
    long long parseSize(const std::string& sizeStr) const;

    static constexpr long long kMaxSizeBytes = 8LL * 1024 * 1024 * 1024; // 8GB cap

    // Trackers appended to every magnet URI — improves peer discovery.
    const std::vector<std::string> trackers_ = {
        "udp://tracker.opentrackr.org:1337/announce",
        "udp://tracker.torrent.eu.org:451/announce",
        "udp://exodus.desync.com:6969/announce",
        "udp://tracker.coppersurfer.tk:6969/announce",
    };
};

} // namespace movie_source1