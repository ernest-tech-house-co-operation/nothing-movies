#include "scraper_core/ScraperEngine.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QEventLoop>
#include <QTimer>
#include <QUuid>
#include <QUrl>
#include <QNetworkRequest>
#include <iostream>

namespace scraper_core {

static QString daemonBinaryPathImpl() {
    QDir installRoot(QCoreApplication::applicationDirPath());
#if defined(_WIN32)
    return installRoot.filePath("nothing/nothing-browser.exe");
#else
    return installRoot.filePath("nothing/nothing-browser");
#endif
}

bool isAvailable() {
    return QFileInfo::exists(daemonBinaryPathImpl());
}

NothingBrowser::NothingBrowser(QObject* parent) : QObject(parent) {
    connect(&m_ws, &QWebSocket::textMessageReceived,
            this, &NothingBrowser::onTextMessageReceived);
    connect(&m_ws, &QWebSocket::disconnected,
            this, &NothingBrowser::onDisconnected);

    connect(&m_daemonProcess, &QProcess::readyReadStandardError, this, [this]() {
        std::cerr << "[scraper_core] daemon stderr: "
                   << m_daemonProcess.readAllStandardError().toStdString();
    });
}

NothingBrowser::~NothingBrowser() { shutdown(); }

QString NothingBrowser::daemonBinaryPath() const { return daemonBinaryPathImpl(); }

bool NothingBrowser::isConnected() const { return m_connected; }

bool NothingBrowser::connectOnce(int timeoutMs, const QString& host, quint16 port, const QString& key) {
    QUrl url;
    url.setScheme("ws");
    url.setHost(host);
    url.setPort(port);

    QNetworkRequest req(url);
    if (!key.isEmpty()) {
        req.setRawHeader("X-Piggy-Key", key.toUtf8());
    }

    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    bool ok = false;
    auto onOk = connect(&m_ws, &QWebSocket::connected, &loop, [&]() { ok = true; loop.quit(); });
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    auto onErr = connect(&m_ws, &QWebSocket::errorOccurred, &loop, [&](QAbstractSocket::SocketError) { loop.quit(); });
#else
    auto onErr = connect(&m_ws, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
                          &loop, [&](QAbstractSocket::SocketError) { loop.quit(); });
#endif
    connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    m_ws.open(req);
    timeoutTimer.start(timeoutMs);
    loop.exec();
    disconnect(onOk);
    disconnect(onErr);

    if (ok) m_connected = true;
    return ok;
}

bool NothingBrowser::spawnAndConnect(const QString& host, quint16 port, const QString& key) {
    if (!isAvailable()) {
        std::cerr << "[scraper_core] Nothing Browser binary not found at "
                   << daemonBinaryPath().toStdString() << "\n";
        return false;
    }

    m_daemonProcess.setProgram(daemonBinaryPath());
    m_daemonProcess.setArguments({"--headless"});
    m_daemonProcess.start();
    if (!m_daemonProcess.waitForStarted(5000)) {
        std::cerr << "[scraper_core] failed to spawn Nothing Browser binary\n";
        return false;
    }

    for (int i = 0; i < 10; ++i) {
        if (connectOnce(1000, host, port, key)) {
            m_ownsDaemon = true;
            return true;
        }
        QEventLoop wait;
        QTimer::singleShot(300, &wait, &QEventLoop::quit);
        wait.exec();
    }

    std::cerr << "[scraper_core] daemon spawned but never accepted a connection on "
               << host.toStdString() << ":" << port << "\n";
    m_daemonProcess.kill();
    return false;
}

bool NothingBrowser::start(const QString& host, quint16 port, const QString& key) {
    m_host = host;
    m_port = port;
    m_key = key;
    m_intentionalShutdown = false;

    // 1. Try joining an already-running (possibly shared) daemon first.
    if (connectOnce(1500, host, port, key)) {
        m_ownsDaemon = false;
        return true;
    }

    // 2. Nothing listening — spawn our own copy and become responsible
    //    for keeping it alive (the watchdog only respawns daemons we own).
    return spawnAndConnect(host, port, key);
}

void NothingBrowser::shutdown() {
    m_intentionalShutdown = true; // disarm the watchdog before we tear anything down

    if (m_connected) {
        sendRaw(m_ownsDaemon ? "shutdown" : "close", {}, 3000);
        m_ws.close();
        m_connected = false;
    }
    if (m_ownsDaemon && m_daemonProcess.state() != QProcess::NotRunning) {
        if (!m_daemonProcess.waitForFinished(3000)) {
            m_daemonProcess.kill();
        }
    }
}

void NothingBrowser::onDisconnected() {
    m_connected = false;

    if (m_intentionalShutdown) {
        return; // this was us — nothing to recover from
    }

    // The daemon went away without us asking it to (crash, someone else's
    // stray `shutdown` on a shared instance, OOM-kill, whatever). If we're
    // the one who's supposed to be keeping it alive, bring it back.
    std::cerr << "[scraper_core] daemon connection lost unexpectedly — attempting recovery\n";

    bool recovered = false;
    if (m_ownsDaemon) {
        // Our own copy died; respawn it fresh.
        recovered = spawnAndConnect(m_host, m_port, m_key);
    } else {
        // We were only ever a guest on a shared daemon. It might still be
        // alive for other clients and just dropped us, or it might be
        // fully dead. Try a plain reconnect a few times; if that fails,
        // fall back to spawning our own.
        for (int i = 0; i < 5 && !recovered; ++i) {
            recovered = connectOnce(1000, m_host, m_port, m_key);
        }
        if (!recovered) {
            recovered = spawnAndConnect(m_host, m_port, m_key);
        }
    }

    if (recovered) {
        // Old tabIds any plugin was holding are gone with the old daemon —
        // that plugin needs to know to re-create them.
        emit daemonRecovered();
    } else {
        std::cerr << "[scraper_core] recovery failed — daemon is unavailable\n";
    }
}

QJsonObject NothingBrowser::sendRaw(const QString& cmd, const QJsonObject& payload, int timeoutMs) {
    if (!m_connected) {
        return QJsonObject{{"ok", false}, {"data", "not connected to Nothing Browser"}};
    }

    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QJsonObject request{{"id", id}, {"cmd", cmd}, {"payload", payload}};
    m_ws.sendTextMessage(QJsonDocument(request).toJson(QJsonDocument::Compact));

    if (m_pendingReplies.contains(id)) return m_pendingReplies.take(id);

    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    auto conn = connect(&m_ws, &QWebSocket::textMessageReceived, &loop, [&]() {
        if (m_pendingReplies.contains(id)) loop.quit();
    });
    timeoutTimer.start(timeoutMs);
    loop.exec();
    disconnect(conn);

    if (m_pendingReplies.contains(id)) return m_pendingReplies.take(id);
    return QJsonObject{{"ok", false}, {"data", "timeout waiting for reply to " + cmd}};
}

void NothingBrowser::onTextMessageReceived(const QString& message) {
    QJsonObject msg = QJsonDocument::fromJson(message.toUtf8()).object();

    const QString type = msg.value("type").toString();
    if (type == "event") {
        emit eventReceived(msg.value("event").toString(), msg);
        return;
    }
    if (type == "error") {
        std::cerr << "[scraper_core] daemon error: "
                   << msg.value("message").toString().toStdString() << "\n";
        return;
    }

    const QString id = msg.value("id").toString();
    if (!id.isEmpty()) {
        m_pendingReplies[id] = msg;
    }
}

}