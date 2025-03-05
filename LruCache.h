#pragma

#include <memory>
#include <unordered_map>

namespace MinCache {

template <typename Key, typename Value> class LruCache; // forward declaration
template <typename Key, typename Value>

class LruNode {
private:
    Key _key;
    Value _value;
    size_t _accessCount;
    std::shared_ptr<LruNode<Key, Value>> _prev;
    std::shared_ptr<LruNode<Key, Value>> _next;

public:
    LruNode(Key key, Value value)
        : _key(key), _value(value), _accessCount(1), _prev(nullptr),
          _next(nullptr) {}

    Key get_key() const { return _key; }
    Value get_value() const { return _value; }
    void set_value(const Value &value) { value = _value; }
    size_t get_accessCount() const { return _accessCount; }
    void increment_accessCount() { ++_accessCount; }
};

} // namespace MinCache
