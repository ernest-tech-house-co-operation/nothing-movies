#pragma once
#include <QObject>
#include <QVariantList>
#include <QString>

namespace ui {

class VendorBridge : public QObject {
    Q_OBJECT
public:
    explicit VendorBridge(QObject* parent = nullptr);

    // Snapshot of all registered vendors: name, sourceRepoUrl, releaseRepo,
    // license, currentTag, vendorDir — used for the initial list render.
    Q_INVOKABLE QVariantList listVendors();

    // Triggers a check for one vendor on a background thread; result comes
    // back via updateStatus / updateFinished (never blocks the UI thread).
    Q_INVOKABLE void checkVendor(const QString& name);

    // Checks every registered vendor, one after another, on a background thread.
    Q_INVOKABLE void checkAll();

signals:
    void updateStatus(QString vendorName, QString status);   // "checking"
    void updateFinished(QString vendorName, bool updated, QString newTag, QString error);

private:
    QVariantList toVariantList() const;
};

} // namespace ui
