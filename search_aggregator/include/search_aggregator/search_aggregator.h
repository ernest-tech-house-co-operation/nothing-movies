#pragma once
#include "core/ISourceProvider.h"
#include <memory>
#include <string>
#include <vector>

namespace search_aggregator {

struct SourceEntry {
    std::string name;                                   // must match the sourceName each provider stamps on its results
    std::shared_ptr<core::ISourceProvider> provider;
    bool enabled = true;
    // When false, SearchBridge skips the TMDB poster/title lookup for this
    // source's results entirely -- results come back as a flat list (raw
    // cleaned title, no poster) almost instantly. Tapping an entry still
    // goes to Info/Stream as normal.
    bool loadImages = true;
};

// Holds every registered movie source (slot 1-8) and queries whichever
// ones are currently enabled. Doesn't know or care what apibay, tmdb, or
// any specific source is -- only talks through core::ISourceProvider.
class SearchAggregatorModule {
public:
    SearchAggregatorModule() = default;

    // Registered once at startup in app/main.cpp for each active slot,
    // e.g. registerSource("Apibay (Torrent)", std::make_shared<movie_source1::ApibayProvider>());
    // loadImages=false marks a source as list-only (see SourceEntry::loadImages).
    void registerSource(const std::string& name, std::shared_ptr<core::ISourceProvider> provider,
                         bool loadImages = true);

    // Settings-screen toggles -- on by default, opt-out per source.
    void setSourceEnabled(const std::string& name, bool enabled);
    bool isSourceEnabled(const std::string& name) const;
    std::vector<std::string> listSourceNames() const;

    // Per-source poster/TMDB-matching toggle, checked by SearchBridge.
    void setSourceImages(const std::string& name, bool loadImages);
    bool isSourceImagesEnabled(const std::string& name) const;

    // Queries every enabled source and merges results into one list.
    // A source that throws/fails is skipped, not fatal to the rest.
    std::vector<core::MediaResult> searchAll(const std::string& query) const;

    // Routes back to whichever provider produced this result (matched via
    // result.sourceName) and asks it for the stream/magnet URL.
    std::string getStreamUrl(const core::MediaResult& result) const;

private:
    std::vector<SourceEntry> sources_;
};

} // namespace search_aggregator