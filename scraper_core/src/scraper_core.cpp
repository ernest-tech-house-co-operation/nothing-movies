#include "scraper_core/ScraperEngine.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QEventLoop>
#include <QTimer>
#include <QDeadlineTimer>
#include <QUuid>
#include <QUrl>
#include <QNetworkRequest>
#include <iostream>

namespace scraper_core {

QString runnerBinaryPathImpl() {
    QDir installRoot(QCoreApplication::applicationDirPath());
#if defined(_WIN32)
    return installRoot.filePath("nothing/nothing-browser.exe");
#else
    return installRoot.filePath("nothing/nothing-browser");
#endif
}

bool isAvailable() {
    return QFileInfo::exists(runnerBinaryPathImpl());
}

NothingBrowser::NothingBrowser(QObject* parent) : QObject(parent) {
    connect(&m_ws, &QWebSocket::textMessageReceived,
            this, &NothingBrowser::onTextMessageReceived);
    connect(&m_ws, &QWebSocket::connected, this, &NothingBrowser::onConnected);

    connect(&m_daemonProcess, &QProcess::readyReadStandardError, this, [this]() {
        std::cerr << "[scraper_core] daemon stderr: "
                   << m_daemonProcess.readAllStandardError().toStdString();
    });
}

NothingBrowser::~NothingBrowser() { shutdown(); }

QString NothingBrowser::runnerBinaryPath() const { return runnerBinaryPathImpl(); }

void NothingBrowser::onConnected() { m_connected = true; }

bool NothingBrowser::isConnected() const { return m_connected; }

bool NothingBrowser::waitForDaemon(int timeoutMs) {
    QDeadlineTimer deadline(timeoutMs);
    QEventLoop loop;
    QTimer::singleShot(qMin(timeoutMs, 200), &loop, &QEventLoop::quit);
    loop.exec();
    return !deadline.hasExpired();
}

bool NothingBrowser::start(const QString& host, quint16 port, const QString& key) {
    QUrl url;
    url.setScheme("ws");
    url.setHost(host);
    url.setPort(port);

    QNetworkRequest req(url);
    if (!key.isEmpty()) {
        req.setRawHeader("X-Piggy-Key", key.toUtf8());
    }

    auto attemptConnect = [&](int timeoutMs) -> bool {
        QEventLoop loop;
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);
        bool ok = false;
        auto onOk = connect(&m_ws, &QWebSocket::connected, &loop, [&]() { ok = true; loop.quit(); });
        auto onErr = connect(&m_ws, &QWebSocket::errorOccurred, &loop, [&](QAbstractSocket::SocketError) { loop.quit(); });
        connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
        m_ws.open(req);
        timeoutTimer.start(timeoutMs);
        loop.exec();
        disconnect(onOk);
        disconnect(onErr);
        return ok;
    };

    // 1. Try joining an already-running (possibly shared) daemon first.
    if (attemptConnect(1500)) {
        m_ownsDaemon = false;
        return true;
    }

    // 2. Nothing listening — spawn our own copy of the vendored binary.
    if (!isAvailable()) {
        std::cerr << "[scraper_core] Nothing Browser binary not found at "
                   << runnerBinaryPath().toStdString() << "\n";
        return false;
    }

    m_daemonProcess.setProgram(runnerBinaryPath());
    m_daemonProcess.setArguments({"--headless"});
    m_daemonProcess.start();
    if (!m_daemonProcess.waitForStarted(5000)) {
        std::cerr << "[scraper_core] failed to spawn Nothing Browser binary\n";
        return false;
    }

    // 3. Retry-connect a few times while it boots and opens the socket.
    for (int i = 0; i < 10; ++i) {
        if (attemptConnect(1000)) {
            m_ownsDaemon = true;
            return true;
        }
        waitForDaemon(300);
    }

    std::cerr << "[scraper_core] daemon spawned but never accepted a connection on "
               << host.toStdString() << ":" << port << "\n";
    m_daemonProcess.kill();
    return false;
}

void NothingBrowser::shutdown() {
    if (m_connected) {
        // If we spawned this daemon privately, fully kill it. If we just
        // joined a shared one, only tear down our own tabs/connection.
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

bool NothingBrowser::registerSite(const QString& name, const QString& url) {
    QJsonObject tabReply = sendRaw("tab.new");
    if (!tabReply.value("ok").toBool(false)) {
        std::cerr << "[scraper_core] tab.new failed for " << name.toStdString() << "\n";
        return false;
    }
    const QString tabId = tabReply.value("data").toString();
    if (tabId.isEmpty()) {
        std::cerr << "[scraper_core] tab.new returned no tabId for " << name.toStdString() << "\n";
        return false;
    }

    QJsonObject navReply = sendRaw("navigate", {{"tabId", tabId}, {"url", url}}, 30000);
    if (!navReply.value("ok").toBool(false)) {
        std::cerr << "[scraper_core] navigate failed for " << name.toStdString()
                   << ": " << navReply.value("data").toString().toStdString() << "\n";
        return false;
    }

    m_siteTabs[name] = tabId;
    return true;
}

QJsonObject NothingBrowser::call(const QString& site, const QString& cmd,
                                  const QJsonObject& payload, int timeoutMs) {
    if (!m_siteTabs.contains(site)) {
        return QJsonObject{{"ok", false}, {"data", "unknown site: " + site + " (call registerSite first)"}};
    }
    QJsonObject fullPayload = payload;
    fullPayload["tabId"] = m_siteTabs.value(site);
    return sendRaw(cmd, fullPayload, timeoutMs);
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