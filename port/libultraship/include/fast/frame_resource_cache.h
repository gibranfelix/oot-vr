#pragma once

#include <stddef.h>
#include <stdint.h>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

// SOH [Quest] Resource lookups for one frame, by hash or by path. The display lists ask for the same
// vertex, matrix, light, display list, and texture resources many times in a frame, and two times in
// VR (one pass for each eye). Each request to the ResourceManager makes strings, locks a mutex, and
// makes a future. This cache does the request one time for each hash or path, and keeps the resource
// alive until Clear. Clear it at the start of each frame: the game unloads resources only between
// frames. Pure logic: the caller gives the load function, so the host unit tests can drive it.
template <typename Resource> class FrameResourceCache {
  public:
    template <typename Load> std::shared_ptr<Resource> Get(uint64_t hash, Load&& load) {
        auto it = mByHash.find(hash);
        if (it != mByHash.end()) {
            mHits++;
            return it->second;
        }
        mLoads++;
        auto resource = load(hash);
        mByHash.emplace(hash, resource);
        return resource;
    }

    // The cache keeps a copy of the path: the caller can change its buffer after the call.
    template <typename Load> std::shared_ptr<Resource> Get(const char* path, Load&& load) {
        auto it = mByPath.find(std::string_view(path));
        if (it != mByPath.end()) {
            mHits++;
            return it->second;
        }
        mLoads++;
        auto resource = load(path);
        mByPath.emplace(std::string(path), resource);
        return resource;
    }

    void Clear() {
        mByHash.clear();
        mByPath.clear();
    }

    // Totals since the start. Clear does not reset them.
    size_t Hits() const {
        return mHits;
    }
    size_t Loads() const {
        return mLoads;
    }

  private:
    // Find by string_view without a std::string for each lookup.
    struct PathHash {
        using is_transparent = void;
        size_t operator()(std::string_view path) const {
            return std::hash<std::string_view>{}(path);
        }
    };

    std::unordered_map<uint64_t, std::shared_ptr<Resource>> mByHash;
    std::unordered_map<std::string, std::shared_ptr<Resource>, PathHash, std::equal_to<>> mByPath;
    size_t mHits = 0;
    size_t mLoads = 0;
};
