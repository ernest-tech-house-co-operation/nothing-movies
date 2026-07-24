#pragma once
#include <QObject>
#include <QVariantList>
#include <QPointer>
#include <memory>
#include "tmdb_client/TmdbClient.h"

namespace ui {

class TmdbBridge : public QObject {
    Q_OBJECT
public:
    explicit TmdbBridge(const std::string& apiKey, QObject* parent = nullptr);

    Q_INVOKABLE void loadTrending();
    Q_INVOKABLE void loadUpcoming();

signals:
    void trendingReady(const QVariantList& movies);
    void upcomingReady(const QVariantList& movies);
    void loadError(const QString& message);

private:
    std::shared_ptr<tmdb_client::TmdbClient> client_;

    static QVariantList toVariantList(const std::vector<tmdb_client::TmdbListItem>& items);
};

}