#pragma once
#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <memory>

namespace search_aggregator {
class SearchAggregatorModule;
}



class HomepageBridge : public QObject {
    Q_OBJECT
public:
    explicit HomepageBridge(std::shared_ptr<search_aggregator::SearchAggregatorModule> aggregator,
                            std::nullptr_t = nullptr,
                            QObject* parent = nullptr);

    Q_INVOKABLE void loadHomepage();
    Q_INVOKABLE void loadInfo(const QString& id, const QString& sourceName,
                              const QString& type = "movie");
    Q_INVOKABLE QVariantList getSources();
    Q_INVOKABLE QVariantList getSubtitleUrls(const QString& id, const QString& sourceName);

signals:
    void homepageReady(QVariantList items, QString sourceName, bool isFallback);
    void homepageError(QString message);
    void infoReady(QVariantMap info);
    void infoError(QString message);

private:
    std::shared_ptr<search_aggregator::SearchAggregatorModule> aggregator_;
};

