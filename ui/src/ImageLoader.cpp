#include "ui/ImageLoader.h"

#include <QLabel>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>

ImageLoader* ImageLoader::instance() {
    static ImageLoader* s = new ImageLoader();
    return s;
}

ImageLoader::ImageLoader(QObject* parent) : QObject(parent) {}

void ImageLoader::load(const QString& url, QLabel* target) {
    if (!target) return;
    if (url.isEmpty()) {
        target->clear();
        return;
    }

    QNetworkRequest req{QUrl(url)};
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = nam_.get(req);
    pending_.insert(reply, QPointer<QLabel>(target));

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QPointer<QLabel> lbl = pending_.take(reply);
        reply->deleteLater();
        if (!lbl) return;
        if (reply->error() != QNetworkReply::NoError) return;

        QPixmap pm;
        if (!pm.loadFromData(reply->readAll())) return;

        // Fit every image to the target geometry, expanding and center-cropping
        // to emulate QML's PreserveAspectCrop without changing the label size.
        const QSize target = lbl->size();
        if (target.isValid() && !target.isEmpty()) {
            pm = pm.scaled(target, Qt::KeepAspectRatioByExpanding,
                           Qt::SmoothTransformation);
        }
        lbl->setScaledContents(false);
        lbl->setPixmap(pm);
    });
}