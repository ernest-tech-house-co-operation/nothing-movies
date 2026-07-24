#pragma once
#include <QObject>
#include <QVariantList>
#include <memory>
#include <string>
#include "search_aggregator/search_aggregator.h"
#include "tmdb_client/TmdbClient.h"

namespace ui {

// Bridges search_aggregator's torrent-provider results through TitleCleaner
// and TmdbClient to get clean titles + posters, then hands QML a flat
// QVariantList. Runs off the UI thread, same pattern as TmdbBridge.
class SearchBridge : public QObject {
    Q_OBJECT
public:
    // tmdbApiKey: SearchBridge owns its own TmdbClient instance (cheap --
    // just holds the key string) rather than reaching into TmdbBridge's
    // private client_, so the two bridges stay decoupled.
    explicit SearchBridge(const std::string& tmdbApiKey,
                           std::shared_ptr<search_aggregator::SearchAggregatorModule> aggregator,
                           QObject* parent = nullptr);

    Q_INVOKABLE void search(const QString& query);

    // Resolves a tapped result's id+sourceName back to a playable stream
    // URL/magnet via the aggregator. title is carried through unchanged so
    // the caller doesn't need to re-match it to anything.
    Q_INVOKABLE void getStreamUrl(const QString& id, const QString& sourceName, const QString& title);

signals:
    // Each entry: { title, posterUrl, year, sourceName, id, matched, rawTitle }
    void resultsReady(const QVariantList& results);
    void searchError(const QString& message);

    void streamUrlReady(const QString& title, const QString& url);
    void streamUrlError(const QString& message);

private:
    std::shared_ptr<search_aggregator::SearchAggregatorModule> aggregator_;
    std::shared_ptr<tmdb_client::TmdbClient> tmdbClient_;
};

}  // namespace ui