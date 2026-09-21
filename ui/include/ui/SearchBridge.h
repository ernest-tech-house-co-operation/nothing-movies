#pragma once
#include <QObject>
#include <QVariantList>
#include <memory>
#include <string>
#include "search_aggregator/search_aggregator.h"



class SearchBridge : public QObject {
    Q_OBJECT
public:
    explicit SearchBridge(const std::string& tmdbApiKey,
                          std::shared_ptr<search_aggregator::SearchAggregatorModule> aggregator,
                          QObject* parent = nullptr);

    Q_INVOKABLE void search(const QString& query);
    Q_INVOKABLE void searchTV(const QString& query);
    Q_INVOKABLE void getDownloadOptions(const QString& title, int year, bool isTV);
    Q_INVOKABLE void getStreamUrl(const QString& id, const QString& sourceName, const QString& title);

signals:
    void resultsReady(const QVariantList& results);
    void searchError(const QString& message);
    void streamUrlReady(const QString& title, const QString& url);
    void streamUrlError(const QString& message);
    void downloadOptionsReady(const QVariantList& options);

private:
    std::shared_ptr<search_aggregator::SearchAggregatorModule> aggregator_;
};

