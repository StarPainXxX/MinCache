#include <cstdint>
#include <unordered_map>
#pragma

#include "CachePolicy.h"
#include <memory>
#include <mutex>

namespace MinCache {

template <typename Key, typename Value> class LfuCache; // forward declaration
//--------------------------------------------------------------03/10/2025
// FreqList class: to store nodes with the same access frequency
//--------------------------------------------------------------joel

template <typename Key, typename Value> class FreqList {
private:
    struct Node {
        int freq;
        Key key;
        Value value;
        std::shared_ptr<Node> pre;
        std::shared_ptr<Node> next;

        Node() : freq(1), pre(nullptr), next(nullptr) {}

        Node(Key key, Value value)
            : freq(1), key(key), value(value), pre(nullptr), next(nullptr) {}
    };

    using NodePtr = std::shared_ptr<Node>;
    int _freq;
    NodePtr _head;
    NodePtr _tail;

public:
    explicit FreqList(int n) : _freq(n) {
        _head = std::make_shared<Node>();
        _tail = std::make_shared<Node>();
        _head->next = _tail;
        _tail->pre = _head;
    }

    bool is_empty() const { return _head->next == _tail; }

    void add_node(NodePtr node) {
        if (!node || !_head || !_tail)
            return;

        node->pre = _tail->pre;
        node->next = _tail;
        _tail->pre->next = node;
        _tail->pre = node;
    }

    void remove_node(NodePtr node) {
        if (!node || !_head || !_tail)
            return;
        if (!node->pre || !node->next)
            return;

        node->pre->next = node->next;
        node->next->pre = node->pre;

        node->pre = nullptr;
        node->next = nullptr;
    }

    NodePtr get_firstNode() const { return _head->next; }

    friend class LfuCache<Key, Value>;
};

//-------------------------------------------------------------------
// LFU Cache class
//--------------------------------------------------------------------

template <typename Key, typename Value>
class LfuCache : public CachePolicy<Key, Value> {
public:
    using Node = typename FreqList<Key, Value>::Node;
    using NodePtr = std::shared_ptr<Node>;
    using NodeMap = std::unordered_map<Key, NodePtr>;

    LfuCache(int capacity, int maxAverageNum = 10)
        : _capacity(capacity), _minFreq(INT8_MAX),
          _maxAverageNum(maxAverageNum), _curAverAgeNum(0), _curTotalNum(0) {}

    ~LfuCache() override = default;

    void put(Key key, Value value) override {
        if (_capacity <= 0) {
            return;
        }

        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _nodeMap.find(key);
        if (it != _nodeMap.end()) {
            it->second->value = value;

            get_internal(it->second, value);
            return;
        }
    }

    bool get(Key key, Value &value) override {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _nodeMap.find(key);
        if (it != _nodeMap.end()) {
            get_internal(it->second, value);
            return true;
        }

        return false;
    }

    Value get(Key key) override {
        Value value;
        get(key, value);
        return value;
    }

    void purge() {
        _nodeMap.clear();
        _freqToFreqList.clear();
    }

private:
    void put_internal(Key key, Value value);
    void get_internal(NodePtr node, Value &value);
    void kick_out();
    void remove_from_freqList(NodePtr node);
    void add_to_freqList(NodePtr node);
    void add_freqNum();
    void decrease_freqNum(int num);
    void handle_over_MaxAverageNum();
    void update_MinFreq(int freq);

private:
    int _capacity;
    int _minFreq;
    int _maxAverageNum;
    int _curAverAgeNum;
    int _curTotalNum;
    std::mutex _mutex;

    NodeMap _nodeMap;
    std::unordered_map<int, FreqList<Key, Value> *> _freqToFreqList;
};

} // namespace MinCache
