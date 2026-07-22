#include "tmdb_client/TitleCleaner.h"
#include <regex>
#include <algorithm>

namespace tmdb_client {

TitleCleaner::CleanResult TitleCleaner::clean(const std::string& rawName) {
    std::string name = rawName;

    // normalize separators first: dots/underscores -> spaces
    std::replace(name.begin(), name.end(), '.', ' ');
    std::replace(name.begin(), name.end(), '_', ' ');

    CleanResult result;

    // capture a 4-digit year (1900-2099) before we strip it
    std::smatch yearMatch;
    if (std::regex_search(name, yearMatch, std::regex(R"(\b(19\d{2}|20\d{2})\b)"))) {
        result.year = std::stoi(yearMatch[1].str());
    }

    // patterns to strip, in rough order of specificity.
    // each is case-insensitive.
    static const std::vector<std::string> junkPatterns = {
        R"(\b(19\d{2}|20\d{2})\b)",                     // year
        R"(\b(2160p|1080p|720p|480p|4k|uhd)\b)",         // resolution
        R"(\b(bluray|blu-ray|bdrip|brrip|webrip|web-dl|web dl|hdtv|dvdrip|hdrip|camrip|cam)\b)", // source
        R"(\b(x264|x265|h264|h265|hevc|avc|xvid)\b)",    // codec
        R"(\b(aac|ac3|dts|mp3|5 1|7 1)\b)",              // audio
        R"(\bS\d{1,2}E\d{1,2}\b)",                       // SxxEyy episode tags
        R"(\[.*?\])",                                     // [group tags]
        R"(\(.*?\))",                                     // (parenthetical tags)
        R"(\bYIFY\b|\bYTS\b|\bRARBG\b)",                 // known release group names
    };

    for (const auto& pat : junkPatterns) {
        name = std::regex_replace(name, std::regex(pat, std::regex::icase), " ");
    }

    // collapse multiple spaces, trim
    name = std::regex_replace(name, std::regex(R"(\s+)"), " ");
    name.erase(0, name.find_first_not_of(' '));
    if (auto pos = name.find_last_not_of(' '); pos != std::string::npos)
        name.erase(pos + 1);

    result.title = name;
    return result;
}

}