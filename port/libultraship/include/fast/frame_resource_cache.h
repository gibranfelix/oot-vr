#pragma once

#include <stdint.h>
#include <memory>
#include <unordered_map>

// SOH [Quest] Resource lookups by hash for one frame. The display lists ask for the same vertex,
// matrix, light, display list, and texture resources many times in a frame, and two times in VR (one
// pass for each eye). Each request to the ResourceManager makes strings, locks a mutex, and makes a
// future. This cache does the request one time for each hash, and keeps the resource alive until
// Clear. Clear it at the start of each frame: the game unloads resources only between frames. Pure
// logic: the caller gives the load function, so the host unit tests can drive it.
template <typename Resource> class FrameResourceCache {
  public:
    template <typename Load> std::shared_ptr<Resource> Get(uint64_t hash, Load&& load) {
        auto it = mEntries.find(hash);
        if (it != mEntries.end()) {
            return it->second;
        }
        auto resource = load(hash);
        mEntries.emplace(hash, resource);
        return resource;
    }

    void Clear() {
        mEntries.clear();
    }

  private:
    std::unordered_map<uint64_t, std::shared_ptr<Resource>> mEntries;
};
