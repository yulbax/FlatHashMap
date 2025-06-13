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
    class Element;
    template<bool isConst>
    class Iterator;

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

    Iterator<false> find(const Key & key);
    [[nodiscard]] Iterator<true> find(const Key & key) const;

    Iterator<false> begin();
    [[nodiscard]] Iterator<true> begin() const;

    Iterator<false> end();
    [[nodiscard]] Iterator<true> end() const;

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
