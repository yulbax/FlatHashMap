#pragma once

// NESTED OBJECTS
template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
enum class FlatHashMap<Key, Value, Size, Hash>::Status { FREE, OCCUPIED, DELETED };

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
class FlatHashMap<Key, Value, Size, Hash>::KeyValue {
public:
    KeyValue() = default;

    template <typename K, typename V>
    KeyValue(K && k, V && v)
        : key(std::forward<K>(k)), value(std::forward<V>(v)) {}

    Key key;
    Value value;
};

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
class FlatHashMap<Key, Value, Size, Hash>::Element {
public:
    Element() : kv(), status(Status::FREE) {}

    template <typename K, typename V>
    Element(K && key, V && value)
        : kv(std::forward<K>(key), std::forward<V>(value)), status(Status::FREE) {}

    KeyValue kv;
    Status status;
};


template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
template<typename VecRef, typename KeyValType>
class FlatHashMap<Key, Value, Size, Hash>::IteratorBase {
public:
    IteratorBase(VecRef & data, std::size_t index)
        : m_Data(data), m_Index(index) {
        skipToOccupied();
    }

    IteratorBase & operator++() {
        ++m_Index;
        skipToOccupied();
        return *this;
    }

    KeyValType & operator*() const {
        return m_Data[m_Index].kv;
    }

    bool operator!=(const IteratorBase& other) const {
        return m_Index != other.m_Index;
    }

private:
    void skipToOccupied() {
        while (m_Index < m_Data.size() && m_Data[m_Index].status != Status::OCCUPIED) {
            ++m_Index;
        }
    }

    VecRef & m_Data;
    std::size_t m_Index;
};

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
    m_Data[index].kv = KeyValue(std::forward<K>(key), std::forward<V>(value));
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
        m_Data[index].kv.key = std::forward<K>(key);
        m_Data[index].status = Status::OCCUPIED;
        ++m_Count;
    }

    return m_Data[index].kv.value;
}

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
Value & FlatHashMap<Key, Value, Size, Hash>::at(const Key & key) {
    std::size_t index = findIndex(key);
    if (index == OUT_OF_RANGE) {
        throw std::out_of_range("Key not found");
    }
    return m_Data[index].kv.value;
}

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
const Value & FlatHashMap<Key, Value, Size, Hash>::at(const Key & key) const {
    std::size_t index = findIndex(key);
    if (index == OUT_OF_RANGE) {
        throw std::out_of_range("Key not found");
    }
    return m_Data[index].kv.value;
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
        m_Data[pos].kv.key = Key{};
        m_Data[pos].status = Status::DELETED;
        --m_Count;
        return true;
    }
    return false;
}

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
void FlatHashMap<Key, Value, Size, Hash>::clear() {
    m_Data.resize(m_Size);
    m_Data.clear();
    m_Count = 0;
}

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
FlatHashMap<Key, Value, Size, Hash>::Iterator
FlatHashMap<Key, Value, Size, Hash>::begin() {
    return Iterator(m_Data, 0);
}

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
FlatHashMap<Key, Value, Size, Hash>::ConstIterator
FlatHashMap<Key, Value, Size, Hash>::begin() const {
    return ConstIterator(m_Data, 0);
}

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
FlatHashMap<Key, Value, Size, Hash>::Iterator
FlatHashMap<Key, Value, Size, Hash>::end() {
    return Iterator(m_Data, m_Data.size());
}

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
FlatHashMap<Key, Value, Size, Hash>::ConstIterator
FlatHashMap<Key, Value, Size, Hash>::end() const {
    return ConstIterator(m_Data, m_Data.size());
}

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
FlatHashMap<Key, Value, Size, Hash>::Iterator
FlatHashMap<Key, Value, Size, Hash>::find(const Key & key) {
    std::size_t index = findIndex(key);
    if (index == OUT_OF_RANGE) {
        return end();
    }
    return Iterator(m_Data, index);
}

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
FlatHashMap<Key, Value, Size, Hash>::ConstIterator
FlatHashMap<Key, Value, Size, Hash>::find(const Key & key) const {
    std::size_t index = findIndex(key);
    if (index == OUT_OF_RANGE) {
        return end();
    }
    return ConstIterator(m_Data, index);
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
            (*this)[std::move(element.kv.key)] = std::move(element.kv.value);
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

        if (m_Data[pos].status == Status::OCCUPIED && m_Data[pos].kv.key == key) {
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

        if (m_Data[pos].kv.key == key) return pos;

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