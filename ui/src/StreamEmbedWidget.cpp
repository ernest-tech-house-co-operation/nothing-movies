#include "ui/StreamEmbedWidget.h"
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineUrlRequestInterceptor>
#include <QWebEngineSettings>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QUrl>
#include <QStringList>

class AdBlockInterceptor : public QWebEngineUrlRequestInterceptor {
public:
    explicit AdBlockInterceptor(QObject* parent = nullptr)
        : QWebEngineUrlRequestInterceptor(parent) {}

    void interceptRequest(QWebEngineUrlRequestInfo& info) override {
        const QString url = info.requestUrl().host();
        static const QStringList blocked = {
            "doubleclick.net", "googlesyndication.com", "adservice.google.com",
            "ads.google.com", "googleadservices.com", "adnxs.com",
            "moatads.com", "adsystem.com", "amazon-adsystem.com",
            "scorecardresearch.com", "outbrain.com", "taboola.com",
            "popads.net", "popcash.net", "trafficjunky.net",
            "exoclick.com", "juicyads.com", "ero-advertising.com"
        };
        for (const QString& domain : blocked) {
            if (url.contains(domain)) {
                info.block(true);
                return;
            }
        }
    }
};

StreamEmbedWidget::StreamEmbedWidget(QWidget* parent)
    : QWidget(parent)
{
    setStyleSheet("background-color: #000000;");

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* topBar = new QWidget(this);
    topBar->setFixedHeight(48);
    topBar->setStyleSheet("background: rgba(0,0,0,200);");
    auto* tl = new QHBoxLayout(topBar);
    tl->setContentsMargins(12, 0, 12, 0);

    backBtn_ = new QPushButton("← Back", topBar);
    backBtn_->setFixedSize(90, 34);
    backBtn_->setStyleSheet(
        "QPushButton { background-color: #1a1a2e; color: white; border: 1px solid #2a2a3e;"
        "  border-radius: 8px; font-size: 13px; }"
        "QPushButton:hover { background-color: #2d2d4e; }");
    connect(backBtn_, &QPushButton::clicked, this, [this]() {
        stop();
        emit backRequested();
    });
    tl->addWidget(backBtn_);

    titleLabel_ = new QLabel(topBar);
    titleLabel_->setStyleSheet("color: white; font-size: 14px; background: transparent;");
    tl->addWidget(titleLabel_, 1);
    root->addWidget(topBar);

    auto* profile = new QWebEngineProfile("StreamEmbed", this);
    auto* interceptor = new AdBlockInterceptor(this);
    profile->setUrlRequestInterceptor(interceptor);
    profile->settings()->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);
    profile->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);

    view_ = new QWebEngineView(this);
    auto* page = new QWebEnginePage(profile, view_);
    view_->setPage(page);
    root->addWidget(view_, 1);
}

void StreamEmbedWidget::loadUrl(const QString& url) {
    titleLabel_->setText(url);
    view_->load(QUrl(url));
}

void StreamEmbedWidget::stop() {
    view_->stop();
    view_->load(QUrl("about:blank"));
}
