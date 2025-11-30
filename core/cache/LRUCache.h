#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <unordered_map>
#include <list>
#include <string>
#include <optional>
#include <chrono>

struct CacheEntry {
    std::string url;
    std::string data;        // Full HTTP response body
    size_t size;             // bytes
    std::chrono::steady_clock::time_point timestamp;
};

class LRUCache {
public:
    explicit LRUCache(size_t maxBytes);

    std::optional<std::string> get(const std::string& url);

    void put(const std::string& url, const std::string& data);

    size_t getCurrentSize() const { return currentSize; }

private:
    using ListIt = std::list<CacheEntry>::iterator;

    size_t maxBytes;
    size_t currentSize;

    std::list<CacheEntry> lruList;  // front = most recently used
    std::unordered_map<std::string, ListIt> cacheMap;

    void moveToFront(ListIt it);
    void evictIfNeeded(size_t required);
};

#endif
