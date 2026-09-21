#include "ui/ThanksWidget.h"
#include "ui/Theme.h"
#include "ui/ImageLoader.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QDesktopServices>
#include <QUrl>
#include <QVector>



struct Social { QString icon; QString url; };

struct Credit {
    QString name;
    QString role;
    QString avatarUrl;
    QString gitUrl;
    QString contribution;
    QString note;
    QVector<Social> socials;
};

QString roleColor(const QString& role) {
    if (role == "Contributor")     return "#7c3aed";
    if (role == "Idea Gifter")     return "#06b6d4";
    if (role == "Inspiration")     return "#f472b6";
    if (role == "Git Contributor") return "#22c55e";
    return "#7c3aed";
}

const QVector<Credit>& creditsData() {
    static const QVector<Credit> data = {
        { "SaltyAom", "Inspiration",
          "https://github.com/SaltyAom.png",
          "https://github.com/SaltyAom",
          "Creator of ElysiaJS",
          "The only person in the entier world i would notmind meeting and accomplishing in life",
          { {"🐦", "https://twitter.com/saltyAom"}, {"🌐", "https://elysiajs.com"} } },

        { "Pease Ernest", "Contributor",
          "https://avatars.githubusercontent.com/u/173539960?v=4",
          "https://github.com/Ernest12287",
          "Creator and sole developer of Nothing Movies. Built the entire app from scratch — UI, sources, bridges, the works.",
          "One of the best growing developers bringing free, open options to users who deserve better than paywalls.",
          { {"🐙", "https://github.com/Ernest12287"} } },

        { "VidLove", "Inspiration",
          "https://vidlove.cc/favicon.ico",
          "https://vidlove.cc",
          "Provides the embed streaming API that powers playback in Nothing Movies.",
          "Massive appreciation to VidLove for enabling OSS developers like us — without their embed API this app simply wouldn't work. Thank you genuinely.",
          { {"🌐", "https://vidlove.cc"} } },

        { "RiveStream", "Inspiration",
          "https://www.rivestream.app/favicon.ico",
          "https://www.rivestream.app",
          "Movie source API powering one of the content sources in Nothing Movies.",
          "Still under active development but already ships a fast, working movie experience. We may have poked around the API a little... 👀",
          { {"🌐", "https://www.rivestream.app"} } },

        { "KFlix", "Inspiration",
          "https://www.kflix.cc/favicon.ico",
          "https://www.kflix.cc",
          "Rich metadata API used throughout Nothing Movies for movie info, cast, posters and more.",
          "Genuinely great metadata. We'd strongly suggest maybe hiding the endpoints next time though 😄 Much love.",
          { {"🌐", "https://www.kflix.cc"} } },

        { "YTS Official", "Inspiration",
          "https://en.yts-official.com/favicon.ico",
          "https://en.yts-official.com",
          "Torrent API powering the download functionality in Nothing Movies.",
          "The torrent scene wouldn't be the same without YTS. Thanks for keeping it alive and well-seeded 🌱",
          { {"🌐", "https://en.yts-official.com"} } },
    };
    return data;
}

QWidget* buildCard(const Credit& c, QWidget* parent) {
    const QString accent = roleColor(c.role);

    auto* card = new QFrame(parent);
    card->setObjectName("creditCard");
    card->setStyleSheet(QString(
        "QFrame#creditCard { background-color: #171225; border: 1px solid #221c3a;"
        " border-radius: 12px; }"));

    auto* lay = new QHBoxLayout(card);
    lay->setContentsMargins(20, 16, 20, 16);
    lay->setSpacing(18);

    // accent stripe
    auto* stripe = new QFrame(card);
    stripe->setFixedWidth(4);
    stripe->setStyleSheet(QString("background-color: %1; border-radius: 2px;").arg(accent));
    lay->addWidget(stripe);

    // avatar
    auto* avatar = new QLabel(card);
    avatar->setFixedSize(64, 64);
    avatar->setStyleSheet(QString(
        "background-color: #2a2a3a; border: 2px solid %1; border-radius: 32px;").arg(accent));
    avatar->setAlignment(Qt::AlignCenter);
    if (c.avatarUrl.isEmpty()) {
        avatar->setText(QString::fromUtf8("🙂"));
    } else {
        ImageLoader::instance()->load(c.avatarUrl, avatar);
    }
    lay->addWidget(avatar, 0, Qt::AlignTop);

    // text column
    auto* col = new QVBoxLayout();
    col->setSpacing(6);

    auto* nameRow = new QHBoxLayout();
    nameRow->setSpacing(10);

    auto* name = new QLabel(c.name, card);
    name->setStyleSheet(
        "color: white; font-size: 17px; font-weight: bold; background: transparent;");
    nameRow->addWidget(name);

    auto* role = new QLabel(c.role, card);
    role->setStyleSheet(QString(
        "background-color: %1; color: white; font-size: 10px; font-weight: bold;"
        " border-radius: 11px; padding: 3px 10px;").arg(accent));
    nameRow->addWidget(role);
    nameRow->addStretch();
    col->addLayout(nameRow);

    if (!c.contribution.isEmpty()) {
        auto* contrib = new QLabel(c.contribution, card);
        contrib->setWordWrap(true);
        contrib->setStyleSheet(
            "color: #e5e0ff; font-size: 13px; background: transparent;");
        col->addWidget(contrib);
    }
    if (!c.note.isEmpty()) {
        auto* note = new QLabel(c.note, card);
        note->setWordWrap(true);
        note->setStyleSheet(
            "color: #8b86a8; font-size: 12px; font-style: italic; background: transparent;");
        col->addWidget(note);
    }

    // links
    auto* links = new QHBoxLayout();
    links->setSpacing(8);

    auto mkChip = [&](const QString& text, const QString& url) {
        auto* chip = new QPushButton(text, card);
        chip->setCursor(Qt::PointingHandCursor);
        chip->setStyleSheet(
            "QPushButton { background-color: #211c38; color: white; border: none;"
            "  border-radius: 13px; padding: 5px 12px; font-size: 11px; }"
            "QPushButton:hover { background-color: #332a55; }");
        QObject::connect(chip, &QPushButton::clicked, chip, [url]() {
            QDesktopServices::openUrl(QUrl(url));
        });
        return chip;
    };

    if (!c.gitUrl.isEmpty()) links->addWidget(mkChip(QString::fromUtf8("🐙 GitHub"), c.gitUrl));
    for (const Social& s : c.socials) links->addWidget(mkChip(s.icon, s.url));
    links->addStretch();

    if (!c.gitUrl.isEmpty() || !c.socials.isEmpty()) col->addLayout(links);

    lay->addLayout(col, 1);
    return card;
}



ThanksWidget::ThanksWidget(QWidget* parent) : QWidget(parent) {
    setObjectName("ThanksWidget");
    setStyleSheet(QString("QWidget#ThanksWidget { background-color: %1; }").arg(NM_BG));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background: transparent; border: none; }");
    root->addWidget(scroll);

    auto* content = new QWidget(scroll);
    content->setObjectName("thanksContent");
    content->setStyleSheet(
        QString("QWidget#thanksContent { background-color: %1; }").arg(NM_BG));
    scroll->setWidget(content);

    auto* main = new QVBoxLayout(content);
    main->setContentsMargins(32, 32, 32, 32);
    main->setSpacing(24);

    auto* title = new QLabel("Thanks", content);
    title->setStyleSheet(
        "color: white; font-size: 30px; font-weight: bold; background: transparent;");
    main->addWidget(title);

    auto* sub = new QLabel(
        "Everyone who helped make Nothing Movies happen.", content);
    sub->setStyleSheet(
        "color: #a1a1c9; font-size: 14px; background: transparent;");
    main->addWidget(sub);

    auto* rule = new QFrame(content);
    rule->setFixedHeight(2);
    rule->setStyleSheet(
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        " stop:0 #7c3aed, stop:1 #06b6d4); border-radius: 1px;");
    main->addWidget(rule);

    for (const Credit& c : creditsData())
        main->addWidget(buildCard(c, content));

    main->addStretch();
}