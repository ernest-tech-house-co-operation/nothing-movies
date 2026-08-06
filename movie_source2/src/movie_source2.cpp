#include "movie_source2/movie_source2.h"
#include "scraper_core/ScraperEngine.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QDebug>
#include <regex>
#include <sstream>
#include <thread>
#include <chrono>

namespace movie_source2 {

// ---- Constructor ----
AniworldProvider::AniworldProvider(scraper_core::NothingBrowser* engine)
    : m_engine(engine) {
    if (!scraper_core::isAvailable()) {
        qWarning() << "[Aniworld] Nothing Browser not available!";
    }
}

void AniworldProvider::setToken(const std::string& token) {
    m_token = token;
}

// ---- getCapabilities ----
core::SourceCapabilities AniworldProvider::getCapabilities() const {
    core::SourceCapabilities caps;
    caps.hasHomepage = true;
    caps.hasPoster = true;
    caps.hasRating = false;
    caps.hasSubtitles = true;
    caps.hasDownload = false;
    caps.hasStream = true;
    caps.streamType = "http";
    caps.downloadType = "http";
    caps.hasInfo = true;
    caps.info.hasMovie = true;
    caps.info.hasSeries = true;
    caps.info.hasYear = true;
    caps.info.hasRating = false;
    caps.info.hasCast = true;
    caps.info.hasSynopsis = true;
    caps.info.hasTrailer = false;
    caps.info.hasQuality = true;
    caps.info.hasLength = false;
    caps.info.hasSubtitles = true;
    return caps;
}

// ---- sendBrowserCommand ----
std::string AniworldProvider::sendBrowserCommand(const std::string& cmd, const json& payload, int timeoutMs) {
    if (!m_engine || !m_engine->isConnected()) {
        qWarning() << "[Aniworld] No connected scraper engine!";
        return "";
    }

    QJsonObject qPayload;
    for (auto& [key, value] : payload.items()) {
        if (value.is_string()) {
            qPayload[key.c_str()] = QString::fromStdString(value.get<std::string>());
        } else if (value.is_number_integer()) {
            qPayload[key.c_str()] = value.get<int>();
        } else if (value.is_number_float()) {
            qPayload[key.c_str()] = value.get<double>();
        } else if (value.is_boolean()) {
            qPayload[key.c_str()] = value.get<bool>();
        } else if (value.is_array()) {
            QJsonArray arr;
            for (const auto& item : value) {
                if (item.is_string()) {
                    arr.append(QString::fromStdString(item.get<std::string>()));
                }
            }
            qPayload[key.c_str()] = arr;
        }
    }

    QJsonObject result = m_engine->sendRaw(
        QString::fromStdString(cmd),
        qPayload,
        timeoutMs
    );

    if (result.value("ok").toBool()) {
        QJsonDocument doc(result);
        return doc.toJson(QJsonDocument::Compact).toStdString();
    }

    qWarning() << "[Aniworld] Command failed:" << result.value("data").toString().toStdString().c_str();
    return "";
}

json AniworldProvider::sendBrowserCommandJson(const std::string& cmd, const json& payload, int timeoutMs) {
    std::string result = sendBrowserCommand(cmd, payload, timeoutMs);
    if (result.empty()) {
        return json::object();
    }
    try {
        return json::parse(result);
    } catch (const std::exception& e) {
        qWarning() << "[Aniworld] Failed to parse JSON response:" << e.what();
        return json::object();
    }
}

std::string AniworldProvider::createTab() {
    json result = sendBrowserCommandJson("tab.new");
    if (result.contains("data") && result["data"].is_string()) {
        return result["data"].get<std::string>();
    }
    qWarning() << "[Aniworld] Failed to create tab!";
    return "";
}

void AniworldProvider::closeTab(const std::string& tabId) {
    sendBrowserCommand("tab.close", {{"tabId", tabId}});
}

std::string AniworldProvider::navigateAndWait(const std::string& tabId, const std::string& url) {
    json result = sendBrowserCommandJson("navigate", {
        {"tabId", tabId},
        {"url", url}
    });

    if (result.contains("data") && result["data"].is_string()) {
        return result["data"].get<std::string>();
    }
    return "";
}

std::string AniworldProvider::getPageContent(const std::string& tabId) {
    json result = sendBrowserCommandJson("page.content", {
        {"tabId", tabId}
    });

    if (result.contains("data") && result["data"].is_string()) {
        return result["data"].get<std::string>();
    }
    return "";
}

std::string AniworldProvider::executeScript(const std::string& tabId, const std::string& script) {
    // Was "page.evaluate" with payload key "script" - neither exists in the
    // Piggy protocol. The real command is "evaluate" (no "page." prefix,
    // it sits alone in the DOM interaction group) and it reads the script
    // from payload.js, not payload.script.
    json result = sendBrowserCommandJson("evaluate", {
        {"tabId", tabId},
        {"js", script}
    });

    if (result.contains("data")) {
        if (result["data"].is_string()) {
            return result["data"].get<std::string>();
        } else {
            return result["data"].dump();
        }
    }
    return "";
}

// ---------- getHomepage ----------
std::vector<core::HomepageItem> AniworldProvider::getHomepage() {
    std::vector<core::HomepageItem> results;

    if (!scraper_core::isAvailable() || !m_engine || !m_engine->isConnected()) {
        qWarning() << "[Aniworld] Cannot get homepage: Nothing Browser not available/connected";
        return results;
    }

    std::string tabId = createTab();
    if (tabId.empty()) {
        return results;
    }

    std::string url = m_isSerienstream ? SERIENSTREAM_URL : MAIN_URL;
    navigateAndWait(tabId, url);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    executeScript(tabId, R"(
        new Promise((resolve) => {
            if (document.readyState === 'complete') resolve();
            else window.addEventListener('load', resolve);
        });
    )");

    std::string script = R"(
        (function() {
            const items = [];
            document.querySelectorAll('.coverListItem, .col-6').forEach(el => {
                const link = el.querySelector('a');
                if (!link) return;
                const href = link.getAttribute('href');
                if (!href) return;
                const img = el.querySelector('img');
                const title = img ? img.getAttribute('alt') : '';
                const posterUrl = img ? (img.getAttribute('data-src') || img.getAttribute('src')) : '';
                const category = el.querySelector('.genre, .category')?.textContent?.trim() || '';
                items.push({ id: href, title: title || 'Unknown', imageUrl: posterUrl, category: category });
            });
            return JSON.stringify(items);
        })();
    )";

    std::string result = executeScript(tabId, script);
    closeTab(tabId);

    if (result.empty()) return results;

    try {
        json items = json::parse(result);
        for (const auto& item : items) {
            core::HomepageItem homeItem;
            homeItem.id = item.value("id", "");
            homeItem.title = item.value("title", "Unknown");
            homeItem.imageUrl = item.value("imageUrl", "");
            homeItem.category = item.value("category", "");
            results.push_back(homeItem);
        }
    } catch (const std::exception& e) {
        qWarning() << "[Aniworld] Failed to parse homepage JSON:" << e.what();
    }

    return results;
}

// ---------- getMediaInfo ----------
core::MediaInfo AniworldProvider::getMediaInfo(const std::string& id) {
    core::MediaInfo info;

    if (!scraper_core::isAvailable() || !m_engine || !m_engine->isConnected()) {
        qWarning() << "[Aniworld] Cannot get media info: Nothing Browser not available/connected";
        return info;
    }

    std::string tabId = createTab();
    if (tabId.empty()) {
        return info;
    }

    std::string baseUrl = m_isSerienstream ? SERIENSTREAM_URL : MAIN_URL;
    std::string fullUrl = id;
    if (id.find("http") != 0) {
        fullUrl = baseUrl + (id[0] == '/' ? "" : "/") + id;
    }

    navigateAndWait(tabId, fullUrl);
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    executeScript(tabId, R"(
        new Promise((resolve) => {
            if (document.readyState === 'complete') resolve();
            else window.addEventListener('load', resolve);
        });
    )");

    std::string script = R"(
        (function() {
            const data = {};
            const titleEl = document.querySelector('.series-title span, .row h1');
            data.title = titleEl ? titleEl.textContent.trim() : '';
            const posterEl = document.querySelector('.seriesCoverBox img');
            data.posterUrl = posterEl ? (posterEl.getAttribute('data-src') || posterEl.getAttribute('src')) : '';
            const descEl = document.querySelector('.seri_des, .description-text');
            data.synopsis = descEl ? descEl.textContent.trim() : '';
            const yearEl = document.querySelector('span[itemprop=startDate] a');
            data.year = yearEl ? parseInt(yearEl.textContent.trim()) : 0;
            const cast = [];
            document.querySelectorAll('li:contains("Schauspieler:") ul li a span').forEach(el => cast.push(el.textContent.trim()));
            data.cast = cast;
            const imdbEl = document.querySelector('div.series-title > a');
            data.imdbId = imdbEl ? imdbEl.getAttribute('data-imdb') : '';
            if (!data.imdbId) {
                const imdbLink = document.querySelector('p a[href^="https://www.imdb.com/title/"]');
                if (imdbLink) {
                    const href = imdbLink.getAttribute('href');
                    const match = href.match(/\\/title\\/([^\\/]+)/);
                    if (match) data.imdbId = match[1];
                }
            }
            const episodes = [];
            document.querySelectorAll('tr.episode-row').forEach(row => {
                let epNo = 0;
                const meta = row.querySelector('td > meta');
                if (meta) epNo = parseInt(meta.getAttribute('content')) || 0;
                if (!epNo) {
                    const epText = row.querySelector('th.episode-number-cell');
                    if (epText) epNo = parseInt(epText.textContent.trim()) || 0;
                }
                let season = 0;
                const parent = row.closest('[id^="season"]');
                if (parent) {
                    const match = parent.id.match(/season(\\d+)/);
                    if (match) season = parseInt(match[1]);
                }
                const titleEl = row.querySelector('.seasonEpisodeTitle span, .episode-title-ger');
                const epTitle = titleEl ? titleEl.textContent.trim() : `Episode ${epNo}`;
                let href = '';
                const linkEl = row.querySelector('.seasonEpisodeTitle a');
                if (linkEl) href = linkEl.getAttribute('href');
                if (!href) {
                    const onclick = row.getAttribute('onclick');
                    if (onclick) {
                        const match = onclick.match(/window\\.location='([^']+)'/);
                        if (match) href = match[1];
                    }
                }
                let posterUrl = '';
                if (data.imdbId) {
                    posterUrl = `https://episodes.metahub.space/${data.imdbId}/${season}/${epNo}/w780.jpg`;
                }
                if (epNo > 0) {
                    episodes.push({
                        season: season,
                        episode: epNo,
                        title: epTitle,
                        href: href,
                        posterUrl: posterUrl
                    });
                }
            });
            data.episodes = episodes;
            return JSON.stringify(data);
        })();
    )";

    std::string result = executeScript(tabId, script);
    closeTab(tabId);

    if (!result.empty()) {
        try {
            json data = json::parse(result);
            info.id = id;
            info.title = data.value("title", "");
            info.posterUrl = data.value("posterUrl", "");
            info.synopsis = data.value("synopsis", "");
            info.year = data.value("year", 0);

            if (data.contains("cast") && data["cast"].is_array()) {
                for (const auto& actor : data["cast"]) {
                    info.cast.push_back(actor.get<std::string>());
                }
            }

            bool isMovie = true;
            if (data.contains("episodes") && data["episodes"].is_array()) {
                for (const auto& ep : data["episodes"]) {
                    core::MediaInfo::Episode epInfo;
                    epInfo.season = ep.value("season", 0);
                    epInfo.episode = ep.value("episode", 0);
                    epInfo.title = ep.value("title", "");
                    epInfo.streamUrl = ep.value("href", "");
                    // synopsis not available; leave empty
                    info.episodes.push_back(epInfo);
                    if (ep.value("season", 0) > 0) isMovie = false;
                }
            }

            info.type = isMovie ? "movie" : "series";
        } catch (const std::exception& e) {
            qWarning() << "[Aniworld] Failed to parse media info JSON:" << e.what();
        }
    }

    return info;
}

// ---------- getStreamUrl ----------
std::string AniworldProvider::getStreamUrl(const std::string& id) {
    if (!scraper_core::isAvailable() || !m_engine || !m_engine->isConnected()) {
        qWarning() << "[Aniworld] Cannot get stream: Nothing Browser not available/connected";
        return "";
    }

    std::string tabId = createTab();
    if (tabId.empty()) {
        return "";
    }

    std::string baseUrl = m_isSerienstream ? SERIENSTREAM_URL : MAIN_URL;
    std::string fullUrl = id;
    if (id.find("http") != 0) {
        fullUrl = baseUrl + (id[0] == '/' ? "" : "/") + id;
    }

    navigateAndWait(tabId, fullUrl);
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    executeScript(tabId, R"(
        new Promise((resolve) => {
            if (document.readyState === 'complete') resolve();
            else window.addEventListener('load', resolve);
        });
    )");

    std::string script = R"(
        (function() {
            const links = [];
            document.querySelectorAll('div.hosterSiteVideo ul li').forEach(el => {
                const lang = el.getAttribute('data-lang-key') || '';
                const link = el.getAttribute('data-link-target') || '';
                const nameEl = el.querySelector('h4');
                const name = nameEl ? nameEl.textContent.trim() : '';
                if (link) links.push({ type: 'li', lang, url: link, name });
            });
            document.querySelectorAll('#episode-links button.link-box').forEach(el => {
                const lang = el.getAttribute('data-language-label') || '';
                const link = el.getAttribute('data-play-url') || '';
                const nameEl = el.querySelector('span');
                const name = nameEl ? nameEl.textContent.trim() : '';
                if (link) links.push({ type: 'button', lang, url: link, name });
            });
            return JSON.stringify(links);
        })();
    )";

    std::string result = executeScript(tabId, script);
    if (result.empty()) {
        closeTab(tabId);
        return "";
    }

    try {
        json links = json::parse(result);
        for (const auto& link : links) {
            std::string url = link.value("url", "");
            if (url.empty()) continue;

            // Follow redirect (manual fetch via browser)
            std::string navigateScript = R"(
                (async function() {
                    try {
                        const response = await fetch(')" + url + R"(', { method: 'GET', redirect: 'manual' });
                        return response.headers.get('Location') || ')" + url + R"(';
                    } catch(e) {
                        return ')" + url + R"(';
                    }
                })();
            )";
            std::string redirectResult = executeScript(tabId, navigateScript);
            if (redirectResult.empty()) continue;

            // If it's FileMoon, navigate and extract video source
            if (url.find("filemoon") != std::string::npos || url.find("byse") != std::string::npos) {
                navigateAndWait(tabId, redirectResult);
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));

                std::string extractScript = R"(
                    (function() {
                        const video = document.querySelector('video');
                        if (video && video.getAttribute('src')) return video.getAttribute('src');
                        const scripts = document.getElementsByTagName('script');
                        for (let s of scripts) {
                            const content = s.textContent;
                            const match = content.match(/https?:[^"']+\.m3u8[^"']*/);
                            if (match) return match[0];
                        }
                        const iframe = document.querySelector('iframe');
                        if (iframe) return iframe.getAttribute('src') || '';
                        return '';
                    })();
                )";
                std::string streamUrl = executeScript(tabId, extractScript);
                if (!streamUrl.empty()) {
                    closeTab(tabId);
                    return streamUrl;
                }
            } else if (url.find("dood") != std::string::npos || url.find("doodstream") != std::string::npos) {
                navigateAndWait(tabId, redirectResult);
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                std::string extractScript = R"(
                    (function() {
                        const iframe = document.querySelector('iframe');
                        if (iframe) return iframe.getAttribute('src') || '';
                        const video = document.querySelector('video');
                        if (video) return video.getAttribute('src') || '';
                        return '';
                    })();
                )";
                std::string streamUrl = executeScript(tabId, extractScript);
                if (!streamUrl.empty()) {
                    closeTab(tabId);
                    return streamUrl;
                }
            } else {
                // Assume it's a direct stream URL
                closeTab(tabId);
                return redirectResult;
            }
        }
    } catch (const std::exception& e) {
        qWarning() << "[Aniworld] Failed to parse stream links:" << e.what();
    }

    closeTab(tabId);
    return "";
}

// ---------- search ----------
std::vector<core::MediaResult> AniworldProvider::search(const std::string& query) {
    std::vector<core::MediaResult> results;

    if (!scraper_core::isAvailable() || !m_engine || !m_engine->isConnected()) {
        qWarning() << "[Aniworld] Cannot search: Nothing Browser not available/connected";
        return results;
    }

    if (query.empty()) return results;

    std::string tabId = createTab();
    if (tabId.empty()) {
        return results;
    }

    if (m_isSerienstream) {
        std::string searchUrl = std::string(SERIENSTREAM_URL) + "/suche?term=" + query;
        navigateAndWait(tabId, searchUrl);
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));

        std::string script = R"(
            (function() {
                const results = [];
                document.querySelectorAll('.results-group .card').forEach(el => {
                    const link = el.querySelector('a');
                    if (!link) return;
                    const href = link.getAttribute('href');
                    if (!href) return;
                    const img = el.querySelector('img');
                    const title = img ? img.getAttribute('alt') : '';
                    const posterUrl = img ? (img.getAttribute('data-src') || img.getAttribute('src')) : '';
                    results.push({ id: href, title: title || 'Unknown', posterUrl: posterUrl || '' });
                });
                return JSON.stringify(results);
            })();
        )";
        std::string result = executeScript(tabId, script);
        closeTab(tabId);
        if (!result.empty()) {
            try {
                json items = json::parse(result);
                for (const auto& item : items) {
                    core::MediaResult mr;
                    mr.id = item.value("id", "");
                    mr.title = item.value("title", "Unknown");
                    mr.posterUrl = item.value("posterUrl", "");
                    results.push_back(mr);
                }
            } catch (...) {}
        }
    } else {
        navigateAndWait(tabId, MAIN_URL);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::string script = R"(
            (async function() {
                const response = await fetch('/ajax/search', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded', 'X-Requested-With': 'XMLHttpRequest' },
                    body: 'keyword=)" + query + R"('
                });
                const data = await response.json();
                return JSON.stringify(data);
            })();
        )";
        std::string result = executeScript(tabId, script);
        closeTab(tabId);
        if (!result.empty()) {
            try {
                json data = json::parse(result);
                if (data.is_array()) {
                    for (const auto& item : data) {
                        std::string title = item.value("title", "");
                        std::regex emRegex("</?em>");
                        title = std::regex_replace(title, emRegex, "");
                        core::MediaResult mr;
                        mr.id = item.value("link", "");
                        mr.title = title;
                        mr.posterUrl = "https://raw.githubusercontent.com/phisher98/TVVVV/refs/heads/main/Icons/aniworld.jpg";
                        results.push_back(mr);
                    }
                }
            } catch (...) {}
        }
    }

    return results;
}

// ---------- getSubtitleUrls stub ----------
std::vector<std::string> AniworldProvider::getSubtitleUrls(const std::string& /*id*/) {
    return {};
}

} // namespace movie_source2