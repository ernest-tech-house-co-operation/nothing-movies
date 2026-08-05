#pragma once

#include <string>
#include <map>
#include <vector>

namespace movie_source2 {

class Extractor {
public:
    virtual ~Extractor() = default;
    virtual std::string extract(const std::string& url, const std::map<std::string, std::string>& headers = {}) = 0;
    virtual std::string getName() const = 0;
};

class FileMoonExtractor : public Extractor {
public:
    std::string extract(const std::string& url, const std::map<std::string, std::string>& headers = {}) override;
    std::string getName() const override { return "FileMoon"; }
    
private:
    std::string base64UrlDecode(const std::string& input);
    std::string aesGcmDecrypt(const std::string& key, const std::string& iv, const std::string& payload);
    std::string fetchJson(const std::string& url, const std::map<std::string, std::string>& headers = {});
};

} // namespace movie_source2