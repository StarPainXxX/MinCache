#pragma

#include "CachePolicy.h"
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>

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

        put_internal(key, value);
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
    void update_MinFreq();

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

template <typename Key, typename Value>
void LfuCache<Key, Value>::get_internal(NodePtr node, Value &value) {
    value = node->value;
    remove_from_freqList(node);
    node->freq++;
    add_to_freqList(node);
    if (node->freq - 1 == _minFreq &&
        _freqToFreqList[node->freq - 1]->is_empty())
        _minFreq++;

    add_freqNum();
}

template <typename Key, typename Value>
void LfuCache<Key, Value>::put_internal(Key key, Value value) {
    if (_nodeMap.size() == _capacity) {
        kick_out();
    }

    NodePtr node = std::make_shared<Node>(key, value);
    _nodeMap[key] = node;
    add_to_freqList(node);
    add_freqNum();
    _minFreq = std::min(_minFreq, 1);
}

template <typename Key, typename Value> void LfuCache<Key, Value>::kick_out() {
    NodePtr node = _freqToFreqList[_minFreq]->get_firstNode();
    remove_from_freqList(node);
    _nodeMap.erase(node->key);
    decrease_freqNum(node->freq);
}

template <typename Key, typename Value>
void LfuCache<Key, Value>::remove_from_freqList(NodePtr node) {
    if (!node) {
        return;
    }

    auto freq = node->freq;
    _freqToFreqList[freq]->remove_node(node);
}

template <typename Key, typename Value>
void LfuCache<Key, Value>::add_to_freqList(NodePtr node) {
    if (!node) {
        return;
    }

    auto freq = node->freq;
    if (_freqToFreqList.find(node->freq) == _freqToFreqList.end()) {
        _freqToFreqList[node->freq] = new FreqList<Key, Value>(node->freq);
    }

    _freqToFreqList[freq]->add_node(node);
}

template <typename Key, typename Value>
void LfuCache<Key, Value>::add_freqNum() {
    _curTotalNum++;

    if (_nodeMap.empty()) {
        _curAverAgeNum = 0;

    } else {
        _curAverAgeNum = _curTotalNum / _nodeMap.size();
    }

    if (_curAverAgeNum > _maxAverageNum) {
        handle_over_MaxAverageNum();
    }
}

template <typename Key, typename Value>
void LfuCache<Key, Value>::decrease_freqNum(int num) {
    _curTotalNum -= num;
    if (_nodeMap.empty()) {
        _curTotalNum = 0;
    } else {
        _curAverAgeNum = _curTotalNum / _nodeMap.size();
    }
}

template <typename Key, typename Value>
void LfuCache<Key, Value>::handle_over_MaxAverageNum() {
    if (_nodeMap.empty()) {
        return;
    }

    for (auto it = _nodeMap.begin(); it != _nodeMap.end(); it++) {
        if (!it->second)
            continue;
        NodePtr node = it->second;

        remove_from_freqList(node);

        node->freq -= _maxAverageNum / 2;
        if (node->freq < 1)
            node->freq = 1;

        add_to_freqList(node);
    }

    update_MinFreq();
}

template <typename Key, typename Value>
void LfuCache<Key, Value>::update_MinFreq() {
    _minFreq = INT8_MAX;

    for (const auto &pair : _freqToFreqList) {
        if (pair.second && !pair.second->is_empty()) {
            _minFreq = std::min(_minFreq, pair.first);
        }
    }

    if (_minFreq == INT8_MAX) {
        _minFreq = 1;
    }
}

} // namespace MinCache
