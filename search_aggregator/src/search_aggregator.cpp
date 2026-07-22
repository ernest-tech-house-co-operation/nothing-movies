#include "search_aggregator/search_aggregator.h"
#include <iostream>

namespace search_aggregator {

void SearchAggregatorModule::registerSource(const std::string& name,
                                             std::shared_ptr<core::ISourceProvider> provider) {
    sources_.push_back(SourceEntry{name, std::move(provider), true});
}

void SearchAggregatorModule::setSourceEnabled(const std::string& name, bool enabled) {
    for (auto& entry : sources_) {
        if (entry.name == name) {
            entry.enabled = enabled;
            return;
        }
    }
}

bool SearchAggregatorModule::isSourceEnabled(const std::string& name) const {
    for (const auto& entry : sources_) {
        if (entry.name == name) return entry.enabled;
    }
    return false;
}

std::vector<std::string> SearchAggregatorModule::listSourceNames() const {
    std::vector<std::string> names;
    names.reserve(sources_.size());
    for (const auto& entry : sources_) names.push_back(entry.name);
    return names;
}

std::vector<core::MediaResult> SearchAggregatorModule::searchAll(const std::string& query) const {
    std::vector<core::MediaResult> merged;

    for (const auto& entry : sources_) {
        if (!entry.enabled || !entry.provider) continue;

        try {
            std::vector<core::MediaResult> results = entry.provider->search(query);
            merged.insert(merged.end(), results.begin(), results.end());
        } catch (const std::exception& e) {
            std::cerr << "[search_aggregator] source '" << entry.name
                      << "' failed: " << e.what() << std::endl;
            // one dead/slow/misbehaving source shouldn't sink the whole search
        }
    }

    return merged;
}

std::string SearchAggregatorModule::getStreamUrl(const core::MediaResult& result) const {
    for (const auto& entry : sources_) {
        if (entry.name == result.sourceName && entry.provider) {
            return entry.provider->getStreamUrl(result.id);
        }
    }
    return "";
}

} // namespace search_aggregator