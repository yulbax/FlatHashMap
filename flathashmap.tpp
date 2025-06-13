#pragma once

// PUBLIC METHODS
template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
FlatHashMap<Key, Value, Size, Hash>::FlatHashMap()
    : m_Data(Size), m_Hasher(), m_Count(0), m_Size(Size) {}

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
template<typename K, typename V>
bool FlatHashMap<Key, Value, Size, Hash>::insert(K && key, V && value) {
    if (loadFactor() > LOAD_FACTOR) {
        rehash();
    }

    const std::size_t index = getNextPosition(key);

    if (m_Data[index].status == Status::OCCUPIED) {
        return false;
    }

    ++m_Count;
    m_Data[index].key = std::forward<K>(key);
    m_Data[index].value = std::forward<V>(value);
    m_Data[index].status = Status::OCCUPIED;

    return true;
}

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
template<typename K>
Value & FlatHashMap<Key, Value, Size, Hash>::operator[](K && key) {
    if (loadFactor() > LOAD_FACTOR) {
        rehash();
    }

    const std::size_t index = getNextPosition(key);
    if (m_Data[index].status != Status::OCCUPIED) {
        m_Data[index].key = std::forward<K>(key);
        m_Data[index].status = Status::OCCUPIED;
        ++m_Count;
    }

    return m_Data[index].value;
}

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
Value & FlatHashMap<Key, Value, Size, Hash>::at(const Key & key) {
    std::size_t index = findIndex(key);
    if (index == OUT_OF_RANGE) {
        throw std::out_of_range("Key not found");
    }
    return m_Data[index].value;
}

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
const Value & FlatHashMap<Key, Value, Size, Hash>::at(const Key & key) const {
    std::size_t index = findIndex(key);
    if (index == OUT_OF_RANGE) {
        throw std::out_of_range("Key not found");
    }
    return m_Data[index].value;
}

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
bool FlatHashMap<Key, Value, Size, Hash>::contains(const Key & key) const {
    return findIndex(key) != OUT_OF_RANGE;
}

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
bool FlatHashMap<Key, Value, Size, Hash>::erase(const Key & key) {
    std::size_t pos = findIndex(key);
    if (pos != OUT_OF_RANGE) {
        m_Data[pos].key = Key{};
        m_Data[pos].status = Status::DELETED;
        --m_Count;
        return true;
    }
    return false;
}

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
void FlatHashMap<Key, Value, Size, Hash>::clear() {
    m_Data.clear();
    m_Data.resize(m_Size);
    m_Count = 0;
}

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
typename FlatHashMap<Key, Value, Size, Hash>::template Iterator<false>
FlatHashMap<Key, Value, Size, Hash>::begin() {
    return Iterator<false>(m_Data, 0);
}

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
typename FlatHashMap<Key, Value, Size, Hash>::template Iterator<true>
FlatHashMap<Key, Value, Size, Hash>::begin() const {
    return Iterator<true>(m_Data, 0);
}

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
typename FlatHashMap<Key, Value, Size, Hash>::template Iterator<false>
FlatHashMap<Key, Value, Size, Hash>::end() {
    return Iterator<false>(m_Data, m_Data.size());
}

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
typename FlatHashMap<Key, Value, Size, Hash>::template Iterator<true>
FlatHashMap<Key, Value, Size, Hash>::end() const {
    return Iterator<true>(m_Data, m_Data.size());
}

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
typename FlatHashMap<Key, Value, Size, Hash>::template Iterator<false>
FlatHashMap<Key, Value, Size, Hash>::find(const Key & key) {
    std::size_t index = findIndex(key);
    if (index == OUT_OF_RANGE) {
        return end();
    }
    return Iterator<false>(m_Data, index);
}

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
typename FlatHashMap<Key, Value, Size, Hash>::template Iterator<true>
FlatHashMap<Key, Value, Size, Hash>::find(const Key & key) const {
    std::size_t index = findIndex(key);
    if (index == OUT_OF_RANGE) {
        return end();
    }
    return Iterator<true>(m_Data, index);
}


// PRIVATE METHODS
template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
std::size_t FlatHashMap<Key, Value, Size, Hash>::hash(const Key & key) const {
    return m_Hasher(key) & (m_Data.size() - 1);
}

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
void FlatHashMap<Key, Value, Size, Hash>::rehash() {
    std::vector<Element> oldData = std::move(m_Data);
    m_Data.resize(oldData.size() * 2);
    m_Count = 0;

    for (auto & element : oldData) {
        if (element.status == Status::OCCUPIED) {
            (*this)[std::move(element.key)] = std::move(element.value);
        }
    }
}

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
std::size_t FlatHashMap<Key, Value, Size, Hash>::nextCell(const std::size_t index, const std::size_t shift) const {
    return (index + shift) & (m_Data.size() - 1);
}

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
std::size_t FlatHashMap<Key, Value, Size, Hash>::findIndex(const Key & key) const {
    const std::size_t index = hash(key);
    std::size_t shift = 0;

    while (true) {
        std::size_t pos = nextCell(index, shift);

        if (m_Data[pos].status == Status::FREE) break;

        if (m_Data[pos].status == Status::OCCUPIED && m_Data[pos].key == key) {
            return pos;
        }

        ++shift;
    }

    return OUT_OF_RANGE;
}

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
std::size_t FlatHashMap<Key, Value, Size, Hash>::getNextPosition(const Key & key) const {
    const std::size_t index = hash(key);
    std::size_t firstDeleted = OUT_OF_RANGE;
    std::size_t shift = 0;

    while (true) {
        std::size_t pos = nextCell(index, shift);

        if (m_Data[pos].key == key) return pos;

        if (m_Data[pos].status == Status::DELETED && firstDeleted == OUT_OF_RANGE) {
            firstDeleted = pos;
        }

        if (m_Data[pos].status == Status::FREE) {
            return (firstDeleted != OUT_OF_RANGE) ? firstDeleted : pos;
        }

        ++shift;
    }
}

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
float FlatHashMap<Key, Value, Size, Hash>::loadFactor() const {
    return static_cast<float>(m_Count) / m_Data.size();
}


// NESTED OBJECTS
template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
enum class FlatHashMap<Key, Value, Size, Hash>::Status { FREE, OCCUPIED, DELETED };

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
class FlatHashMap<Key, Value, Size, Hash>::Element {
public:
    explicit Element(Key key = Key{}, Value value = Value{}, const Status status = Status::FREE)
        : key(std::move(key)), value(std::move(value)), status(status) {}

    Key key;
    Value value;
    Status status;
};

template<typename Key,
         typename Value,
         std::size_t Size,
         typename Hash>
         requires PowerOfTwo<Size>

template<bool isConst>
class FlatHashMap<Key, Value, Size, Hash>::Iterator {
public:
    using ElementType = std::conditional_t<isConst, const Element, Element>;
    using VectorType  = std::conditional_t<isConst, const std::vector<Element>, std::vector<Element>>;
    using KeyRef      = const Key&;
    using ValueRef    = std::conditional_t<isConst, const Value&, Value&>;
    using PairType    = std::pair<KeyRef, ValueRef>;

    Iterator(VectorType& data, std::size_t index)
        : m_Data(data), m_Index(index) {
        while (m_Index < m_Data.size() &&
               m_Data[m_Index].status != Status::OCCUPIED) {
            ++m_Index;
        }
    }

    Iterator & operator++() {
        ++m_Index;
        while (m_Index < m_Data.size() &&
               m_Data[m_Index].status != Status::OCCUPIED) {
            ++m_Index;
        }
        return *this;
    }

    PairType operator*() const {
        return {m_Data[m_Index].key, m_Data[m_Index].value};
    }

    bool operator!=(const Iterator & other) const {
        return m_Index != other.m_Index;
    }

private:
    VectorType & m_Data;
    std::size_t m_Index;
};
