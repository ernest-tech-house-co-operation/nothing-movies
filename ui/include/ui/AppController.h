#pragma once
#include <QObject>
#include <QString>

namespace ui {

// Thin QML-facing bridge: QML calls startStream(), MainWindow listens for
// streamRequested() and does the actual work (switching the central
// QStackedWidget to the native mpv page and feeding it the url). Kept
// separate from SearchBridge so QML's "resolve this result to a url" step
// and "now go play that url" step stay decoupled -- InfoScreen could get
// a url from anywhere and still call startStream with it.
class AppController : public QObject {
    Q_OBJECT
public:
    explicit AppController(QObject* parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE void startStream(const QString& title, const QString& url) {
        emit streamRequested(title, url);
    }

    Q_INVOKABLE void stopStream() {
        emit stopRequested();
    }

signals:
    void streamRequested(const QString& title, const QString& url);
    void stopRequested();
};

}  // namespace ui
