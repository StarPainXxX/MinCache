#pragma once

namespace MinCache {

template <typename Key, typename Value>

class CachePolicy {
public:
    virtual ~CachePolicy();
    // add cache api:
    virtual void put(Key key, Value value) = 0;
    //
    virtual bool get(Key key, Value &value) = 0;

    // if find the key in the cache, return value
    virtual Value get(Key key) = 0;
};

} // namespace MinCache
