#pragma once

#include <QObject>
#include <QHash>
#include <QPointer>
#include <QNetworkAccessManager>

class QLabel;
class QNetworkReply;

// Shared async image loader. Feed it a URL and a QLabel — it downloads
// on the shared QNetworkAccessManager and drops the pixmap into the label.
class ImageLoader : public QObject {
    Q_OBJECT
public:
    static ImageLoader* instance();

    void load(const QString& url, QLabel* target);

private:
    explicit ImageLoader(QObject* parent = nullptr);

    QNetworkAccessManager nam_;
    QHash<QNetworkReply*, QPointer<QLabel>> pending_;
};