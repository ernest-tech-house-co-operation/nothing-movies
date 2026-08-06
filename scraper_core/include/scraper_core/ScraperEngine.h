#pragma once
#include <QObject>
#include <QProcess>
#include <QWebSocket>
#include <QJsonObject>
#include <QString>
#include <QMap>
#include <memory>
#include "vendor_updater/VendorUpdater.h"

namespace scraper_core {

// True if the Nothing Browser binary is installed (checked via vendor_updater's
// install location), independent of whether it's currently running.
bool isAvailable();

// scraper_core's whole job: launch the Nothing Browser daemon, know whether
// it's up, keep it alive for the lifetime of the app, and expose the raw
// wire protocol (tab.new + sendRaw) so movie_source plugins can drive their
// own tabs directly. Everything past "give me a tabId and let me send
// commands" is the plugin's own problem — this class does not know or care
// what any plugin does with a tab.
class NothingBrowser : public QObject {
    Q_OBJECT
public:
    explicit NothingBrowser(QObject* parent = nullptr);
    ~NothingBrowser() override;

    // Connects to an already-running daemon if one is listening on
    // host:port, otherwise spawns the vendored binary and connects once
    // it comes up. `key` is only needed if the target daemon requires
    // connection-key auth.
    //
    // Owns the entire vendor install/update lifecycle internally: if the
    // binary isn't present yet, does a synchronous first-time fetch before
    // attempting to spawn (so a fresh install doesn't race the background
    // watcher), then keeps a periodic background update check running for
    // the lifetime of this object. Callers (including main.cpp) don't
    // need their own VendorUpdater instance.
    bool start(const QString& host = "127.0.0.1", quint16 port = 2005,
               const QString& key = QString());

    // Real, intentional teardown — call this once, when the app itself is
    // exiting. Disarms the watchdog first so the shutdown isn't mistaken
    // for a crash and "helpfully" un-done.
    void shutdown();

    bool isConnected() const;

    // The only two primitives a plugin needs:
    //   tabId = engine->sendRaw("tab.new").value("data").toString();
    //   engine->sendRaw("navigate", {{"tabId", tabId}, {"url", url}});
    QJsonObject sendRaw(const QString& cmd, const QJsonObject& payload = {}, int timeoutMs = 15000);

signals:
    // Passthrough for unsolicited server events (navigate/dialog/exposed_call
    // are tab-scoped and only sent to the owning connection; proxy:* are
    // global). Plugins can hook this if they care about async stuff like
    // dialogs popping up on their tab.
    void eventReceived(const QString& eventName, const QJsonObject& fullEvent);

    // Fired when the daemon connection drops for any reason OTHER than our
    // own shutdown() being called — i.e. it crashed, or something else
    // killed it. By the time this fires, the watchdog has already
    // respawned/reconnected. Any tabIds a plugin was holding are gone —
    // this is the plugin's cue to re-create whatever tabs it needs.
    void daemonRecovered();

private:
    bool connectOnce(int timeoutMs, const QString& host, quint16 port, const QString& key);
    bool spawnAndConnect(const QString& host, quint16 port, const QString& key);
    void onDisconnected();
    void onTextMessageReceived(const QString& message);
    QString daemonBinaryPath() const;
    QJsonObject sendRawOnOwningThread(const QString& cmd, const QJsonObject& payload, int timeoutMs);

    QWebSocket m_ws;
    QProcess m_daemonProcess;
    bool m_ownsDaemon = false;
    bool m_connected = false;
    bool m_intentionalShutdown = false;
    bool m_recovering = false;

    // remembered for the watchdog's respawn attempts
    QString m_host;
    quint16 m_port = 2005;
    QString m_key;

    QMap<QString, QJsonObject> m_pendingReplies; // request id -> reply

    std::unique_ptr<vendor_updater::VendorUpdater> m_vendorUpdater;
};

}