#include "scraper_core/ScraperEngine.h"
#include "vendor_updater/VendorUpdater.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QEventLoop>
#include <QTimer>
#include <QUuid>
#include <QUrl>
#include <QNetworkRequest>
#include <QThread>
#include <iostream>

namespace scraper_core {

static std::string platformTag() {
#if defined(_WIN32)
    return "windows";
#else
    return "linux";
#endif
}

static QString vendorDirPath() {
    return QDir(QCoreApplication::applicationDirPath()).filePath("nothing");
}

static QString vendoredBinaryPath() {
#if defined(_WIN32)
    return QDir(vendorDirPath()).filePath("nothing-browser-headless.exe");
#else
    return QDir(vendorDirPath()).filePath("nothing-browser-headless");
#endif
}

// Prefer our own vendored copy (what vendor_updater manages) so version is
// predictable, but fall back to a system install (apt/deb put it on PATH,
// not next to our binary) if we don't have one vendored yet.
static QString daemonBinaryPathImpl() {
    const QString vendored = vendoredBinaryPath();
    if (QFileInfo::exists(vendored)) {
        return vendored;
    }
#if defined(_WIN32)
    const QString systemPath = QStandardPaths::findExecutable("nothing-browser-headless.exe");
#else
    const QString systemPath = QStandardPaths::findExecutable("nothing-browser-headless");
#endif
    if (!systemPath.isEmpty()) {
        return systemPath;
    }
    return vendored; // neither exists - return the vendored path so error messages are consistent
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

    if (ok) {
        m_connected = true;
    } else {
        // Don't leave a half-open/half-connecting socket behind - each
        // failed attempt otherwise leaks a file descriptor, and repeated
        // retries (watchdog + spawn loop) can exhaust the process's fd
        // limit surprisingly fast.
        m_ws.abort();
    }
    return ok;
}

bool NothingBrowser::spawnAndConnect(const QString& host, quint16 port, const QString& key) {
    if (!isAvailable()) {
        std::cerr << "[scraper_core] Nothing Browser binary not found at "
                   << daemonBinaryPath().toStdString() << "\n";
        return false;
    }

    if (m_daemonProcess.state() != QProcess::NotRunning) {
        // Already own a running process (e.g. a previous attempt half-
        // succeeded) - don't spawn a second one on top of it.
        m_daemonProcess.kill();
        m_daemonProcess.waitForFinished(2000);
    }

    m_daemonProcess.setProgram(daemonBinaryPath());
    m_daemonProcess.setArguments({}); // nothing-browser-headless takes no args - it's
                                       // headless by being this binary, not via a flag
    m_daemonProcess.start();
    if (!m_daemonProcess.waitForStarted(5000)) {
        std::cerr << "[scraper_core] failed to spawn Nothing Browser binary\n";
        return false;
    }

    // First-run daemon asks an interactive "require a connection key? (y/N)"
    // question on stdin before it opens its listen socket. Nobody's there
    // to type an answer when we spawn it headlessly, so it hangs forever -
    // pre-empt that by answering "no key" ourselves. Safe no-op on later
    // runs once profile.json exists and the prompt doesn't appear.
    m_daemonProcess.write("n\n");

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

    if (!m_vendorUpdater) {
        m_vendorUpdater = std::make_unique<vendor_updater::VendorUpdater>(
            "BunElysiaReact/nothing-browser",
            vendorDirPath().toStdString(),
            platformTag()
        );

        if (!isAvailable()) {
            // Fresh install, nothing downloaded yet - don't rely on the
            // 6-hour background watch for this, or start()/spawnAndConnect
            // below will race a download that hasn't happened yet. Block
            // once, here, to guarantee we have a binary before proceeding.
            std::cerr << "[scraper_core] no vendored binary found — fetching before first launch\n";
            auto result = m_vendorUpdater->checkAndUpdateOnce();
            if (!result.error.empty()) {
                std::cerr << "[scraper_core] vendor fetch failed: " << result.error << "\n";
            } else if (result.updated) {
                std::cout << "[scraper_core] vendor fetched " << result.newTag << "\n";
            }
        }

        m_vendorUpdater->startBackgroundWatch(6 * 3600, [](vendor_updater::UpdateResult r) {
            if (!r.error.empty()) {
                std::cerr << "[vendor_updater] error: " << r.error << "\n";
            } else if (r.updated) {
                std::cout << "[vendor_updater] updated " << r.oldTag << " -> " << r.newTag << "\n";
            }
        });
    }

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
    if (m_recovering) {
        return; // a recovery attempt is already in flight, don't stack another
    }
    m_recovering = true;

    std::cerr << "[scraper_core] daemon connection lost unexpectedly — attempting recovery\n";

    bool recovered = false;
    const int maxAttempts = 5;
    for (int attempt = 1; attempt <= maxAttempts && !recovered; ++attempt) {
        if (m_ownsDaemon) {
            // Make sure any previous copy is actually gone before spawning
            // another - otherwise repeated failures here stack up zombie
            // processes (and their fds) instead of replacing one daemon.
            if (m_daemonProcess.state() != QProcess::NotRunning) {
                m_daemonProcess.kill();
                m_daemonProcess.waitForFinished(2000);
            }
            recovered = spawnAndConnect(m_host, m_port, m_key);
        } else {
            recovered = connectOnce(1000, m_host, m_port, m_key);
            if (!recovered) {
                // The shared daemon might be fully dead now - fall back to
                // owning our own copy from here on.
                recovered = spawnAndConnect(m_host, m_port, m_key);
            }
        }

        if (!recovered && attempt < maxAttempts) {
            // Backoff instead of hammering - each failure is a spawned
            // process + a socket attempt, both of which cost real fds.
            QEventLoop wait;
            QTimer::singleShot(500 * attempt, &wait, &QEventLoop::quit);
            wait.exec();
        }
    }

    m_recovering = false;

    if (recovered) {
        // Old tabIds any plugin was holding are gone with the old daemon —
        // that plugin needs to know to re-create them.
        emit daemonRecovered();
    } else {
        std::cerr << "[scraper_core] recovery failed after " << maxAttempts
                   << " attempts — giving up, daemon is unavailable\n";
    }
}

QJsonObject NothingBrowser::sendRaw(const QString& cmd, const QJsonObject& payload, int timeoutMs) {
    // QWebSocket's notifiers only work correctly on the thread that owns
    // it (whatever thread constructed this NothingBrowser, typically the
    // main thread). Plugins doing scraping work on a worker thread would
    // otherwise silently break Qt's socket notifiers and see replies
    // never arrive. Marshal onto the owning thread automatically so
    // callers don't need to know or care what thread they're on.
    if (QThread::currentThread() != this->thread()) {
        QJsonObject result;
        QMetaObject::invokeMethod(this, [this, cmd, payload, timeoutMs, &result]() {
            result = sendRawOnOwningThread(cmd, payload, timeoutMs);
        }, Qt::BlockingQueuedConnection);
        return result;
    }
    return sendRawOnOwningThread(cmd, payload, timeoutMs);
}

QJsonObject NothingBrowser::sendRawOnOwningThread(const QString& cmd, const QJsonObject& payload, int timeoutMs) {
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