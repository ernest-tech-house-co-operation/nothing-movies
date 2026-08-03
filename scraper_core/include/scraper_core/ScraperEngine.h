#pragma once
#include <QObject>
#include <QProcess>
#include <QWebSocket>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QString>

namespace scraper_core {

// True if the Nothing Browser binary is installed (checked via vendor_updater's
// install location), independent of whether it's currently running.
bool isAvailable();

// NothingBrowser talks directly to the Nothing Browser daemon over its
// WebSocket protocol (ws://host:2005, one JSON object per text frame).
// No Node.js, no bundled JS client — this is a straight Qt/C++ client for
// the wire protocol described in Nothing Browser's docs.
//
// Source plugins never see the socket — they only ever call the public
// methods below (start/registerSite/call/shutdown), same shape as before,
// just re-plumbed under the hood.
class NothingBrowser : public QObject {
    Q_OBJECT
public:
    explicit NothingBrowser(QObject* parent = nullptr);
    ~NothingBrowser() override;

    // Connects to an already-running daemon if one is listening on
    // host:port, otherwise spawns the vendored binary and connects once
    // it comes up. `key` is only needed if the target daemon was started
    // with connection-key auth enabled.
    bool start(const QString& host = "127.0.0.1", quint16 port = 2005,
               const QString& key = QString());

    // Closes just this client's own tabs/connection (maps to the "close"
    // command) — does not kill a shared daemon other clients are using.
    void shutdown();

    // Opens a new tab, navigates it to `url`, and remembers the resulting
    // tabId under `name` for later call(name, ...) lookups.
    bool registerSite(const QString& name, const QString& url);

    // Sends `cmd` with `payload` against the tab registered as `site`.
    // `payload` should NOT include "tabId" - it's injected automatically.
    // e.g. call("animecloud", "provide.attr", {{"selector","h1"},{"attr","data-id"}})
    QJsonObject call(const QString& site, const QString& cmd,
                      const QJsonObject& payload = {}, int timeoutMs = 15000);

    // Raw escape hatch for commands that aren't tab-scoped (proxy.*,
    // tab.new, shutdown, ...) or when you already have a tabId in hand.
    QJsonObject sendRaw(const QString& cmd, const QJsonObject& payload = {}, int timeoutMs = 15000);

    bool isConnected() const;

signals:
    // Passthrough for unsolicited server events (section 4 of the protocol
    // doc): navigate/dialog/exposed_call are tab-scoped, proxy:* are global.
    void eventReceived(const QString& eventName, const QJsonObject& fullEvent);

private:
    void onTextMessageReceived(const QString& message);
    void onConnected();
    QString runnerBinaryPath() const;
    bool waitForDaemon(int timeoutMs);

    QWebSocket m_ws;
    QProcess m_daemonProcess;   // only used if we had to spawn our own copy
    bool m_ownsDaemon = false;
    bool m_connected = false;

    QMap<QString, QJsonObject> m_pendingReplies; // request id -> reply
    QMap<QString, QString> m_siteTabs;           // site name -> tabId
};

}