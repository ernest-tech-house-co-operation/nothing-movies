#include "scraper_core/ScraperEngine.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QEventLoop>
#include <QTimer>
#include <QUuid>
#include <iostream>

namespace scraper_core {

bool isAvailable() {
    QDir installRoot(QCoreApplication::applicationDirPath());
#if defined(_WIN32)
    QString path = installRoot.filePath("nothing/nothing-browser.exe");
#else
    QString path = installRoot.filePath("nothing/nothing-browser");
#endif
    return QFileInfo::exists(path);
}

NothingBrowser::NothingBrowser(QObject* parent) : QObject(parent) {
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &NothingBrowser::onReadyRead);

    // Surface runner-side crashes/errors instead of swallowing them silently —
    // this is exactly the kind of thing that made the old bug so hard to spot.
    connect(&m_process, &QProcess::readyReadStandardError, this, [this]() {
        std::cerr << "[scraper_core] runner stderr: "
                   << m_process.readAllStandardError().toStdString();
    });
}

NothingBrowser::~NothingBrowser() { shutdown(); }

QString NothingBrowser::runnerScriptPath() const {
    // Ships alongside the app binary, e.g. <install>/scripts/piggy_runner.js
    return QCoreApplication::applicationDirPath() + "/scripts/piggy_runner.js";
}

bool NothingBrowser::start(const QJsonObject& launchOpts) {
    m_process.setProgram("node");
    m_process.setArguments({runnerScriptPath()});
    m_process.start();

    if (!m_process.waitForStarted(5000)) {
        std::cerr << "[scraper_core] failed to start node runner — is Node.js installed?\n";
        return false;
    }

    QJsonObject reply = send("launch", {{"opts", launchOpts}}, 20000);
    if (!reply.value("ok").toBool(false)) {
        std::cerr << "[scraper_core] launch failed: "
                   << reply.value("error").toString().toStdString() << "\n";
        return false;
    }
    return true;
}

void NothingBrowser::shutdown() {
    if (m_process.state() == QProcess::Running) {
        send("close", {}, 3000);
        m_process.closeWriteChannel();
        if (!m_process.waitForFinished(3000)) {
            m_process.kill();
        }
    }
}

bool NothingBrowser::registerSite(const QString& name, const QString& url) {
    QJsonObject reply = send("register", {{"name", name}, {"url", url}}, 15000);
    if (!reply.value("ok").toBool(false)) {
        std::cerr << "[scraper_core] register failed for " << name.toStdString()
                   << ": " << reply.value("error").toString().toStdString() << "\n";
        return false;
    }
    return true;
}

QJsonObject NothingBrowser::call(const QString& site, const QString& method,
                                  const QJsonArray& args, int timeoutMs) {
    QJsonObject reply = send("call", {{"site", site}, {"method", method}, {"args", args}}, timeoutMs);
    if (!reply.value("ok").toBool(false)) {
        std::cerr << "[scraper_core] call failed " << site.toStdString() << "."
                   << method.toStdString() << ": "
                   << reply.value("error").toString().toStdString() << "\n";
    }
    return reply;
}

void NothingBrowser::onReadyRead() {
    m_buffer += m_process.readAllStandardOutput();
    int idx;
    while ((idx = m_buffer.indexOf('\n')) != -1) {
        QByteArray line = m_buffer.left(idx);
        m_buffer.remove(0, idx + 1);
        if (line.trimmed().isEmpty()) continue;

        QJsonObject reply = QJsonDocument::fromJson(line).object();
        const QString id = reply.value("id").toString();
        if (!id.isEmpty()) m_pendingReplies[id] = reply;
    }
}

QJsonObject NothingBrowser::send(const QString& action, QJsonObject payload, int timeoutMs) {
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    payload["id"] = id;
    payload["action"] = action;

    m_process.write(QJsonDocument(payload).toJson(QJsonDocument::Compact) + "\n");

    if (m_pendingReplies.contains(id)) return m_pendingReplies.take(id);

    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(&m_process, &QProcess::readyReadStandardOutput, &loop, [&]() {
        if (m_pendingReplies.contains(id)) loop.quit();
    });
    timeoutTimer.start(timeoutMs);
    loop.exec();

    if (m_pendingReplies.contains(id)) return m_pendingReplies.take(id);
    return QJsonObject{{"ok", false}, {"error", "timeout waiting for reply to " + action}};
}

}