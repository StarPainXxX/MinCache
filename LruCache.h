#pragma

#include "CachePolicy.h"
#include <cmath>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

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

// Lru-k
template <typename Key, typename Value>
class LruKCache : public LruCache<Key, Value> {
public:
    LruKCache(int capacity, int historyCapacity, int k)
        : LruCache<Key, Value>(capacity),
          _historyList(
              std::make_unique<LruCache<Key, size_t>>(historyCapacity)),
          _k(k) {}

    Value get(Key key) {
        int histroyCount = _historyList->get(key);
        _historyList->put(key, ++histroyCount);

        return LruCache<Key, Value>::get(key);
    }

    void put(Key key, Value value) {
        if (LruCache<Key, Value>::get(key) != "")
            LruCache<Key, Value>::put(key, value);

        int histroyCount = _historyList->get(key);
        _historyList->put(key, ++histroyCount);

        if (histroyCount >= _k) {
            _historyList->remove(key);
            LruCache<Key, Value>::put(key, value);
        }
    }

private:
    int _k;
    std::unique_ptr<LruCache<Key, size_t>> _historyList;
};

template <typename Key, typename Value>

class HashLruCache {
public:
    HashLruCache(size_t capacity, int sliceNum)
        : _capacity(capacity),
          _sliceNum(sliceNum > 0 ? sliceNum
                                 : std::thread::hardware_concurrency()) {
        size_t sliceSize = std::ceil(capacity / static_cast<double>(_sliceNum));
        for (int i = 0; i < _sliceNum; ++i) {
            _lruSliceCaches.emplace_back(
                std::make_unique<LruCache<Key, Value>>(sliceSize));
        }
    }

    void put(Key key, Value value) {
        size_t sliceIndex = Hash(key) % _sliceNum;
        return _lruSliceCaches[sliceIndex]->put(key, value);
    }

    bool get(Key key, Value &value) {
        size_t sliceIndex = Hash(key) % _sliceNum;
        return _lruSliceCaches[sliceIndex]->get(key, value);
    }

    Value get(Key key) {
        Value value{};
        memset(&value, 0, sizeof(Value));
        get(key, value);
        return value;
    }

private:
    size_t Hash(const Key key) {
        std::hash<Key> hashFunc;
        return hashFunc(key);
    }

private:
    size_t _capacity;
    int _sliceNum;
    std::vector<std::unique_ptr<LruCache<Key, Value>>> _lruSliceCaches;
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
