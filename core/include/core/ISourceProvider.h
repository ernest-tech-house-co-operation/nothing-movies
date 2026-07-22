#pragma once
#include <string>
#include <vector>

namespace core {

struct MediaResult {
    std::string id;
    std::string title;
    std::string posterUrl;
    std::string sourceName;
};

class ISourceProvider {
public:
    virtual ~ISourceProvider() = default;
    virtual std::vector<MediaResult> search(const std::string& query) = 0;
    virtual std::string getStreamUrl(const std::string& id) = 0;
};

}
