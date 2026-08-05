#include "aniworld_extractor.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <sstream>
#include <iomanip>
#include <regex>

namespace movie_source2 {

std::string FileMoonExtractor::base64UrlDecode(const std::string& input) {
    std::string fixed = input;
    std::replace(fixed.begin(), fixed.end(), '-', '+');
    std::replace(fixed.begin(), fixed.end(), '_', '/');
    int pad = (4 - fixed.length() % 4) % 4;
    fixed.append(pad, '=');
    
    // Use OpenSSL base64 decode
    BIO *bio, *b64;
    char* buffer = (char*)malloc(fixed.length());
    bio = BIO_new_mem_buf(fixed.c_str(), -1);
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    int len = BIO_read(bio, buffer, fixed.length());
    BIO_free_all(bio);
    
    std::string result(buffer, len);
    free(buffer);
    return result;
}

std::string FileMoonExtractor::aesGcmDecrypt(const std::string& key, const std::string& iv, const std::string& payload) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), nullptr, nullptr, nullptr);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv.length(), nullptr);
    EVP_DecryptInit_ex(ctx, nullptr, nullptr, 
                       reinterpret_cast<const unsigned char*>(key.c_str()),
                       reinterpret_cast<const unsigned char*>(iv.c_str()));
    
    std::string plaintext(payload.length(), 0);
    int len = 0;
    EVP_DecryptUpdate(ctx, 
                      reinterpret_cast<unsigned char*>(&plaintext[0]), 
                      &len, 
                      reinterpret_cast<const unsigned char*>(payload.c_str()), 
                      payload.length());
    
    // No auth tag check for simplicity
    EVP_DecryptFinal_ex(ctx, 
                        reinterpret_cast<unsigned char*>(&plaintext[0]) + len, 
                        &len);
    
    EVP_CIPHER_CTX_free(ctx);
    
    // Remove BOM if present
    if (plaintext.length() > 1 && plaintext[0] == '\xEF' && plaintext[1] == '\xBB') {
        plaintext = plaintext.substr(3);
    }
    
    return plaintext;
}

std::string FileMoonExtractor::fetchJson(const std::string& url, const std::map<std::string, std::string>& headers) {
    CURL* curl = curl_easy_init();
    std::string response;
    
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](void* contents, size_t size, size_t nmemb, void* userp) -> size_t {
            ((std::string*)userp)->append((char*)contents, size * nmemb);
            return size * nmemb;
        });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        
        // Add headers
        struct curl_slist* chunk = nullptr;
        for (const auto& [key, value] : headers) {
            chunk = curl_slist_append(chunk, (key + ": " + value).c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        
        curl_easy_perform(curl);
        curl_slist_free_all(chunk);
        curl_easy_cleanup(curl);
    }
    
    return response;
}

std::string FileMoonExtractor::extract(const std::string& url, const std::map<std::string, std::string>& headers) {
    // Parse URL to get code
    std::regex codeRegex(R"(/([^/]+)/embed)");
    std::smatch matches;
    std::string code;
    if (std::regex_search(url, matches, codeRegex)) {
        code = matches[1];
    } else {
        // Try alternative pattern
        std::regex altRegex(R"(/v/([^/?]+))");
        if (std::regex_search(url, matches, altRegex)) {
            code = matches[1];
        }
    }
    
    if (code.empty()) return "";
    
    // Get details
    std::string baseUrl = "https://filemoon.to";
    std::string detailsUrl = baseUrl + "/api/videos/" + code + "/embed/details";
    std::string detailsJson = fetchJson(detailsUrl);
    
    using json = nlohmann::json;
    json details = json::parse(detailsJson);
    
    if (!details.contains("embed_frame_url")) return "";
    
    std::string embedFrameUrl = details["embed_frame_url"];
    std::string embedBase = baseUrl;
    
    // Get playback
    std::string playbackUrl = embedBase + "/api/videos/" + code + "/embed/playback";
    std::map<std::string, std::string> playbackHeaders = {
        {"accept", "*/*"},
        {"accept-language", "en-US,en;q=0.5"},
        {"referer", embedFrameUrl},
        {"x-embed-parent", url}
    };
    
    std::string playbackJson = fetchJson(playbackUrl, playbackHeaders);
    json playback = json::parse(playbackJson);
    
    if (!playback.contains("playback")) return "";
    
    auto p = playback["playback"];
    
    // Build key
    std::string key1 = base64UrlDecode(p["key_parts"][0]);
    std::string key2 = base64UrlDecode(p["key_parts"][1]);
    std::string key = key1 + key2;
    
    // Decrypt
    std::string iv = base64UrlDecode(p["iv"]);
    std::string payload = base64UrlDecode(p["payload"]);
    
    std::string plaintext = aesGcmDecrypt(key, iv, payload);
    
    json decrypted = json::parse(plaintext);
    
    if (decrypted.contains("sources") && decrypted["sources"].is_array()) {
        for (const auto& source : decrypted["sources"]) {
            if (source.contains("url")) {
                return source["url"];
            }
        }
    }
    
    return "";
}

} // namespace movie_source2