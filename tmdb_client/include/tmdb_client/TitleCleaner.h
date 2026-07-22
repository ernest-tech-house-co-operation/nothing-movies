#pragma once
#include <string>

namespace tmdb_client {

// Strips release-junk from a raw torrent/file name and returns a clean,
// searchable guess at the actual title. Modeled on the same stripping
// patterns guessit (Python) uses, ported to C++ regex.
class TitleCleaner {
public:
    struct CleanResult {
        std::string title;
        int year = 0; // 0 if not detected
    };

    static CleanResult clean(const std::string& rawName);
};

}