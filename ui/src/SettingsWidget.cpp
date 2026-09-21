#include "ui/SettingsWidget.h"
#include "ui/Theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QScrollArea>
#include <QDesktopServices>
#include <QUrl>
#include <QPushButton>

SettingsWidget::SettingsWidget(QWidget* parent) : QWidget(parent) {
    setObjectName("SettingsWidget");
    setStyleSheet(QString("QWidget#SettingsWidget { background-color: %1; }").arg(NM_BG));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background: transparent; border: none; }");
    root->addWidget(scroll);

    auto* content = new QWidget(scroll);
    content->setStyleSheet("background: transparent;");
    scroll->setWidget(content);

    auto* main = new QVBoxLayout(content);
    main->setContentsMargins(48, 48, 48, 48);
    main->setSpacing(24);

    // Header
    auto* icon = new QLabel("⚙️", content);
    icon->setStyleSheet("font-size: 42px; background: transparent;");
    main->addWidget(icon);

    auto* title = new QLabel("Nothing to Configure.", content);
    title->setStyleSheet(
        "color: white; font-size: 28px; font-weight: bold; background: transparent;");
    main->addWidget(title);

    auto* sub = new QLabel("But here's everything you should probably know about this app.", content);
    sub->setStyleSheet(
        QString("color: %1; font-size: 14px; background: transparent;").arg(NM_SUBTEXT));
    main->addWidget(sub);

    // Divider
    auto* rule = new QFrame(content);
    rule->setFixedHeight(2);
    rule->setStyleSheet(
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        " stop:0 #7c3aed, stop:1 #06b6d4); border-radius: 1px;");
    main->addWidget(rule);

    // Note builder
    struct Note { QString emoji; QString title; QString body; QString linkText; QString linkUrl; };

    const QVector<Note> notes = {
        { "📖", "Open Source, By Choice (and necessity)",
          "Nothing Movies is open source because we believe in free software — and also because GitHub is a good old dear friend and we have nowhere else to host it.",
          "View on GitHub", "https://github.com/Ernest12287" },

        { "🔄", "No Auto-Updates",
          "There is no auto-update system. We don't have our own servers and fighting GitHub release URLs is a battle we're not ready for. Check GitHub manually every once in a while.",
          "", "" },

        { "⚠️", "Third Party Sources — No Agreements Made",
          "The services credited in our Thanks screen are all third party. We never contacted them or reached any agreement with them. The consequences of open sourcing their API endpoints could be positive, or absolutely negative. We'll find out together.",
          "", "" },

        { "📬", "Source Contact Policy",
          "If any source has a problem with us exposing their API, showing endpoints, or any other reason at all, they can just email ernesttechhouse@gmail.com, open an issue on GitHub, or reach out through our mobile app on Telegram or WhatsApp. Yes, we have an Android app. Why not. We pay $0 for free sites and we found them. Just star the repo, join our WhatsApp and Telegram, and watch movies while we keep working on the UI and shipping each release.",
          "", "" },

        { "🎨", "The UI Is a Work in Progress",
          "It looks rough and we know it. Styling an app in C++ is not as easy as it sounds on paper. It will get better. Probably.",
          "", "" },
        { "🙏", "Why Qt Widgets and Not QML",
            "The app originally used QML — essentially CSS for Qt. It froze. Repeatedly. We are not here to fight C++, we are here to ship. So we switched to Qt Widgets, said a lot of prayers (boy, a lot of prayers), and here we are. It works. Mostly.",
            "", "" },

        { "⚔️", "Why This App Exists",
          "Nothing Movies was built to fight back against MovieBox adding a paywall for movies they don't even host. So we went hunting, found free sources, stitched them together, and brought it to you for free.",
          "The villain in question", "https://movieboxhd.net/" },

        { "⚖️", "Copyright Infringement",
          "Yeah, that might happen sooner or later. But we haven't received anything yet so we still rock. The developer is in Kenya — unless GitHub removes the repo, we're not going anywhere.",
          "", "" },

        { "💸", "Nothing Movies Is Free",
          "Free as in free. No paywall, no subscription, no \"premium tier\". Just movies.",
          "", "" },

        { "🏟️", "Sports Section Coming Soon",
          "We found a free sports source. A sports section is in the works. Stay tuned.",
          "", "" },
    };

    for (const Note& n : notes) {
        auto* card = new QFrame(content);
        card->setStyleSheet(
            "QFrame { background: #13101f; border: 1px solid #1e1a30;"
            " border-radius: 12px; }");

        auto* cl = new QHBoxLayout(card);
        cl->setContentsMargins(20, 18, 20, 18);
        cl->setSpacing(16);

        // Accent stripe
        auto* stripe = new QFrame(card);
        stripe->setFixedWidth(3);
        stripe->setStyleSheet("background: #7c3aed; border-radius: 2px;");
        cl->addWidget(stripe);

        auto* textCol = new QVBoxLayout();
        textCol->setSpacing(6);

        auto* noteTitle = new QLabel(n.emoji + "  " + n.title, card);
        noteTitle->setStyleSheet(
            "color: white; font-size: 15px; font-weight: bold; background: transparent;");
        textCol->addWidget(noteTitle);

        auto* noteBody = new QLabel(n.body, card);
        noteBody->setWordWrap(true);
        noteBody->setStyleSheet(
            QString("color: %1; font-size: 13px; background: transparent;").arg(NM_SUBTEXT));
        textCol->addWidget(noteBody);

        if (!n.linkUrl.isEmpty()) {
            auto* btn = new QPushButton(n.linkText, card);
            btn->setCursor(Qt::PointingHandCursor);
            btn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
            btn->setStyleSheet(
                "QPushButton { background: #211c38; color: #a78bfa; border: none;"
                " border-radius: 10px; padding: 5px 14px; font-size: 12px; }"
                "QPushButton:hover { background: #2e2550; }");
            const QString url = n.linkUrl;
            QObject::connect(btn, &QPushButton::clicked, btn, [url]() {
                QDesktopServices::openUrl(QUrl(url));
            });
            textCol->addWidget(btn);
        }

        cl->addLayout(textCol, 1);
        main->addWidget(card);
    }

    main->addStretch();
}