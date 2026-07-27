#pragma once
#include <QObject>
#include <QProcess>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>

namespace scraper_core {

// True if the Nothing Browser binary is installed (checked via vendor_updater's
// install location), independent of whether Node/the runner script are set up.
bool isAvailable();

// NothingBrowser now talks to a persistent Node process (piggy_runner.js)
// instead of reimplementing Nothing Browser's socket protocol directly.
// piggy_runner.js uses the real, tested `nothing-browser` npm client
// underneath, so scraper_core no longer has to guess reply shapes, tab
// routing, or command names — it just forwards { site, method, args }
// and gets { ok, data } back.
//
// Source plugins never see any of this — they only ever call the public
// methods below (start/registerSite/call/shutdown), same as before.
class NothingBrowser : public QObject {
    Q_OBJECT
public:
    explicit NothingBrowser(QObject* parent = nullptr);
    ~NothingBrowser() override;

    // Spawns the Node runner and launches Nothing Browser itself.
    // Default opts match piggy.launch({ mode: "tab", binary: "headless" }).
    bool start(const QJsonObject& launchOpts = QJsonObject{{"mode", "tab"}, {"binary", "headless"}});

    // Cleanly closes piggy and terminates the runner process.
    void shutdown();

    // Equivalent to piggy.register(name, url) in JS.
    bool registerSite(const QString& name, const QString& url);

    // Calls a method on a registered site, e.g.:
    //   call("animecloud", "navigate", {url})
    //   call("animecloud", "provide.attr", {selector, attr})
    //   call("animecloud", "session.export")
    // Method names and argument order match site.js exactly — see
    // nothing-browser's site.js for the full list of supported methods.
    QJsonObject call(const QString& site, const QString& method,
                      const QJsonArray& args = {}, int timeoutMs = 15000);

private:
    QJsonObject send(const QString& action, QJsonObject payload, int timeoutMs);
    void onReadyRead();
    QString runnerScriptPath() const;

    QProcess m_process;
    QByteArray m_buffer;
    QMap<QString, QJsonObject> m_pendingReplies;
};

}