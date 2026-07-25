#pragma once
#include <QObject>
#include <QVariantList>
#include <memory>
#include <string>

namespace search_aggregator {
class SearchAggregatorModule;
}

namespace ui {

class TmdbBridge;

class HomepageBridge : public QObject {
    Q_OBJECT

public:
    explicit HomepageBridge(std::shared_ptr<search_aggregator::SearchAggregatorModule> aggregator,
                            TmdbBridge* tmdbBridge,
                            QObject* parent = nullptr);

    Q_INVOKABLE void loadHomepage();

signals:
    void homepageReady(QVariantList items, QString sourceName, bool isFallback);
    void homepageError(QString message);

private:
    std::shared_ptr<search_aggregator::SearchAggregatorModule> aggregator_;
    TmdbBridge* tmdbBridge_ = nullptr;
};

} // namespace ui