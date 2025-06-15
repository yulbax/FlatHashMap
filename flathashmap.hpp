#pragma once
#include <vector>
#include <functional>
#include "flathashmapimpl.hpp"

template<typename Key,
         typename Value,
         typename Hash = std::hash<Key>>
class FlatHashMap {

    static constexpr std::size_t OUT_OF_RANGE = -1;
    static constexpr std::size_t DEFAULT_SIZE = 1024;
    static constexpr float LOAD_FACTOR = 0.875;

    using Status = FlatHashMapImpl::Status;
    using KeyValue = FlatHashMapImpl::KeyValue<Key, Value>;
    using Element = FlatHashMapImpl::Element<Key, Value>;

    using Iterator = FlatHashMapImpl::Iterator<Key, Value>;
    using ConstIterator = FlatHashMapImpl::ConstIterator<Key, Value>;

public:
    explicit FlatHashMap(std::size_t size = DEFAULT_SIZE);

    template<typename K, typename V>
    bool insert(K && key, V && value);

    template<typename K>
    Value & operator[](K && key);

    Value & at(const Key & key);
    [[nodiscard]] const Value & at(const Key & key) const;

    [[nodiscard]] bool contains(const Key & key) const;

    bool erase(const Key & key);

    void clear();

    Iterator find(const Key & key);
    [[nodiscard]] ConstIterator find(const Key & key) const;

    Iterator begin();
    [[nodiscard]] ConstIterator begin() const;

    Iterator end();
    [[nodiscard]] ConstIterator end() const;

private:
    [[nodiscard]] std::size_t hash(const Key & key) const;

    void rehash();

    [[nodiscard]] std::size_t nextCell(std::size_t index, std::size_t shift) const;

    [[nodiscard]] std::size_t findIndex(const Key & key) const;

    [[nodiscard]] std::size_t getNextPosition(const Key & key) const ;

    [[nodiscard]] float loadFactor() const;

    std::vector<Element> m_Data;
    Hash m_Hasher;
    std::size_t m_Count;
    std::size_t m_Size;
};

#include "flathashmap.tpp"
