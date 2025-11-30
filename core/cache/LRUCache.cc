#include "LRUCache.h"

LRUCache::LRUCache(size_t maxBytes)
    : maxBytes(maxBytes), currentSize(0) {}

std::optional<std::string> LRUCache::get(const std::string& url) {
    auto it = cacheMap.find(url);
    if (it == cacheMap.end())
        return std::nullopt;

    moveToFront(it->second);
    return it->second->data;
}

void LRUCache::put(const std::string& url, const std::string& data) {
    size_t dataSize = data.size();

    // If URL already exists, update and move
    auto it = cacheMap.find(url);
    if (it != cacheMap.end()) {
        currentSize -= it->second->size;

        it->second->data = data;
        it->second->size = dataSize;
        it->second->timestamp = std::chrono::steady_clock::now();

        currentSize += dataSize;
        moveToFront(it->second);
        evictIfNeeded(0);
        return;
    }

    // New entry
    evictIfNeeded(dataSize);

    lruList.push_front(CacheEntry{
        url,
        data,
        dataSize,
        std::chrono::steady_clock::now()
    });

    cacheMap[url] = lruList.begin();
    currentSize += dataSize;
}

void LRUCache::moveToFront(ListIt it) {
    lruList.splice(lruList.begin(), lruList, it);
}

void LRUCache::evictIfNeeded(size_t required) {
    while (currentSize + required > maxBytes && !lruList.empty()) {
        auto& last = lruList.back();
        currentSize -= last.size;
        cacheMap.erase(last.url);
        lruList.pop_back();
    }
}
