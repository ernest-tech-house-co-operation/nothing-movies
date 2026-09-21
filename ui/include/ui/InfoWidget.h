#pragma once

#include <QWidget>
#include <QVariantMap>

class QLabel;
class QPushButton;
class QComboBox;
class QListWidget;
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class SearchBridge;
class AppController;
class QueueBridge;
class HomepageBridge;

class InfoWidget : public QWidget {
    Q_OBJECT
public:
    InfoWidget(SearchBridge* search,
               AppController* app,
               QueueBridge* queue,
               HomepageBridge* homepage,
               QWidget* parent = nullptr);

    void setInfo(const QVariantMap& info);
    QVariantMap info() const { return info_; }

signals:
    void backRequested();

private slots:
    void onStreamClicked();
    void onDownloadClicked();
    void onDownloadOptionsReady(const QVariantList& options);
    void onTrailerClicked();

    void onStreamUrlReady(const QString& title, const QString& url);
    void onStreamUrlError(const QString& message);
    void onTorrentReadyToPlay(const QString& title, const QString& filePath);
    void onQueueStreamError(const QString& message);

private:
    void clearCast();
    void clearSimilar();
    void buildEpisodeSection(QVBoxLayout* body);
    void populateEpisodes();
    void clearEpisodes();
    void onEpisodeStream();
    void updateButtons();
    void setStatus(const QString& text, bool ok);

    SearchBridge* search_ = nullptr;
    AppController* app_ = nullptr;
    QueueBridge* queue_ = nullptr;
    HomepageBridge* homepage_ = nullptr;

    QVariantMap info_;
    QString pendingAction_;
    QString pendingMagnetTitle_;
    bool resolving_ = false;

    QLabel* titleLabel_ = nullptr;
    QLabel* heroTitle_ = nullptr;
    QLabel* badge_ = nullptr;
    QLabel* poster_ = nullptr;
    QLabel* metaLabel_ = nullptr;
    QLabel* sourceLabel_ = nullptr;
    QLabel* synopsis_ = nullptr;
    QLabel* castTitle_ = nullptr;
    QWidget* castHost_ = nullptr;
    QGridLayout* castLayout_ = nullptr;
    QLabel* similarTitle_ = nullptr;
    QWidget* similarHost_ = nullptr;
    QGridLayout* similarLayout_ = nullptr;

    QWidget* episodesSection_ = nullptr;
    QComboBox* seasonCombo_ = nullptr;
    QListWidget* episodeList_ = nullptr;
    QLabel* episodesTitle_ = nullptr;

    int selectedSeason_ = 1;
    int selectedEpisode_ = 1;
    QLabel* status_ = nullptr;

    QPushButton* streamBtn_ = nullptr;
    QPushButton* downloadBtn_ = nullptr;
    QPushButton* trailerBtn_ = nullptr;
};