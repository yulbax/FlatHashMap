#pragma once
#include <vector>
#include <functional>

template<std::size_t size>
concept PowerOfTwo = (size != 0) && ((size & (size - 1)) == 0);

template<typename Key,
         typename Value,
         std::size_t Size = 1024,
         typename Hash = std::hash<Key>>
         requires PowerOfTwo<Size>
class FlatHashMap {

    enum class Status;
    class KeyValue;
    class Element;
    template<typename VecType, typename KeyValType>
    class IteratorBase;
    using Iterator = IteratorBase<std::vector<Element>, KeyValue>;
    using ConstIterator = IteratorBase<const std::vector<Element>, const KeyValue>;

public:
    FlatHashMap();

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

    static constexpr std::size_t OUT_OF_RANGE = -1;
    static constexpr float LOAD_FACTOR = 0.875;
    std::vector<Element> m_Data;
    Hash m_Hasher;
    std::size_t m_Count;
    std::size_t m_Size;
};

#include "flathashmap.tpp"
