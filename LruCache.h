#include <stdexcept>
#pragma

#include "CachePolicy.h"
#include <memory>
#include <mutex>
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
    void set_value(const Value &value) { _value = value; }
    size_t get_accessCount() const { return _accessCount; }
    void increment_accessCount() { ++_accessCount; }

    friend class LruCache<Key, Value>;
};

template <typename Key, typename Value>
class LruCache : public CachePolicy<Key, Value> {
public:
    using LruNodeType = LruNode<Key, Value>;
    using NodePtr = std::shared_ptr<LruNodeType>;
    using NodeMap = std::unordered_map<Key, NodePtr>;

    LruCache(int capacity) : _capacity(capacity) { initialize_list(); }

    void put(Key key, Value value) override {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _nodeMap.find(key);
        if (it != _nodeMap.end()) {
            update_existing_node(it->second, value);
            return;
        }
        add_newNode(key, value);
    }

    // value is passed by reference to return the value
    bool get(Key key, Value &value) override {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _nodeMap.find(key);
        if (it != _nodeMap.end()) {
            move_to_recent(it->second);
            value = it->second->get_value();
            return true;
        }
        return false;
    }

    Value get(Key key) override {
        Value value{};
        get(key, value);
        return value;
    }

    void remove(Key key) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _nodeMap.find(key);
        if (it != _nodeMap.end()) {
            remove_node(it->second);
            _nodeMap.erase(it);
        }
    }

private:
    void initialize_list();
    void update_existing_node(NodePtr node, const Value &value);
    void add_newNode(const Key &key, const Value &value);
    void move_to_recent(NodePtr node);
    void remove_node(NodePtr node);
    void insert_node(NodePtr node);
    void evict_node();

private:
    int _capacity;
    NodeMap _nodeMap;
    std::mutex _mutex;
    NodePtr _dummyHead;
    NodePtr _dummyTail;
};

template <typename Key, typename Value>
void LruCache<Key, Value>::initialize_list() {
    _dummyHead = std::make_shared<LruNodeType>(Key(), Value());
    _dummyTail = std::make_shared<LruNodeType>(Key(), Value());
    _dummyHead->_next = _dummyTail;
    _dummyTail->_prev = _dummyHead;
}

template <typename Key, typename Value>
void LruCache<Key, Value>::update_existing_node(NodePtr node,
                                                const Value &value) {
    node->set_value(value);
    move_to_recent(node);
}

template <typename Key, typename Value>
void LruCache<Key, Value>::add_newNode(const Key &key, const Value &value) {
    if (_nodeMap.size() >= _capacity) {
        evict_node();
    }

    NodePtr newNode = std::make_shared<LruNodeType>(key, value);
    insert_node(newNode);
    _nodeMap[key] = newNode;
}

template <typename Key, typename Value>
void LruCache<Key, Value>::move_to_recent(NodePtr node) {
    remove_node(node);
    insert_node(node);
}

template <typename Key, typename Value>
void LruCache<Key, Value>::remove_node(NodePtr node) {
    node->_prev->_next = node->_next;
    node->_next->_prev = node->_prev;
}

// in tail insert to remove the least recently used node
template <typename Key, typename Value>
void LruCache<Key, Value>::insert_node(NodePtr node) {
    node->_next = _dummyTail;
    node->_prev = _dummyTail->_prev;
    _dummyTail->_prev->_next = node;
    _dummyTail->_prev = node;
}

template <typename Key, typename Value>
void LruCache<Key, Value>::evict_node() {
    NodePtr node = _dummyHead->_next;
    remove_node(node);
    _nodeMap.erase(node->get_key());
}

} // namespace MinCache
