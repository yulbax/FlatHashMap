#pragma once

// PUBLIC METHODS
template<typename Key, typename Value, typename Hash>
FlatHashMap<Key, Value, Hash>::FlatHashMap(std::size_t size)
    : m_Data(std::bit_ceil(size)), m_Hasher(), m_Count(0), m_Size(m_Data.size()) {}



template<typename Key, typename Value, typename Hash>
template<typename K, typename V>
bool FlatHashMap<Key, Value, Hash>::insert(K && key, V && value) {
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

template<typename Key, typename Value, typename Hash>
template<typename K>
Value & FlatHashMap<Key, Value, Hash>::operator[](K && key) {
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

template<typename Key, typename Value, typename Hash>
Value & FlatHashMap<Key, Value, Hash>::at(const Key & key) {
    std::size_t index = findIndex(key);
    if (index == OUT_OF_RANGE) {
        throw std::out_of_range("Key not found");
    }
    return m_Data[index].kv.value;
}

template<typename Key, typename Value, typename Hash>
const Value & FlatHashMap<Key, Value, Hash>::at(const Key & key) const {
    std::size_t index = findIndex(key);
    if (index == OUT_OF_RANGE) {
        throw std::out_of_range("Key not found");
    }
    return m_Data[index].kv.value;
}

template<typename Key, typename Value, typename Hash>
bool FlatHashMap<Key, Value, Hash>::contains(const Key & key) const {
    return findIndex(key) != OUT_OF_RANGE;
}

template<typename Key, typename Value, typename Hash>
bool FlatHashMap<Key, Value, Hash>::erase(const Key & key) {
    std::size_t pos = findIndex(key);
    if (pos != OUT_OF_RANGE) {
        m_Data[pos].status = Status::DELETED;
        --m_Count;
        return true;
    }
    return false;
}

template<typename Key, typename Value, typename Hash>
void FlatHashMap<Key, Value, Hash>::clear() {
    m_Data.resize(m_Size);
    m_Data.clear();
    m_Count = 0;
}

template<typename Key, typename Value, typename Hash>
FlatHashMap<Key, Value, Hash>::Iterator
FlatHashMap<Key, Value, Hash>::begin() {
    return Iterator(m_Data, 0);
}

template<typename Key, typename Value, typename Hash>
FlatHashMap<Key, Value, Hash>::ConstIterator
FlatHashMap<Key, Value, Hash>::begin() const {
    return ConstIterator(m_Data, 0);
}

template<typename Key, typename Value, typename Hash>
FlatHashMap<Key, Value, Hash>::Iterator
FlatHashMap<Key, Value, Hash>::end() {
    return Iterator(m_Data, m_Data.size());
}

template<typename Key, typename Value, typename Hash>
FlatHashMap<Key, Value, Hash>::ConstIterator
FlatHashMap<Key, Value, Hash>::end() const {
    return ConstIterator(m_Data, m_Data.size());
}

template<typename Key, typename Value, typename Hash>
FlatHashMap<Key, Value, Hash>::Iterator
FlatHashMap<Key, Value, Hash>::find(const Key & key) {
    std::size_t index = findIndex(key);
    if (index == OUT_OF_RANGE) {
        return end();
    }
    return Iterator(m_Data, index);
}

template<typename Key, typename Value, typename Hash>
FlatHashMap<Key, Value, Hash>::ConstIterator
FlatHashMap<Key, Value, Hash>::find(const Key & key) const {
    std::size_t index = findIndex(key);
    if (index == OUT_OF_RANGE) {
        return end();
    }
    return ConstIterator(m_Data, index);
}


// PRIVATE METHODS
template<typename Key, typename Value, typename Hash>
std::size_t FlatHashMap<Key, Value, Hash>::hash(const Key & key) const {
    return m_Hasher(key) & (m_Data.size() - 1);
}

template<typename Key, typename Value, typename Hash>
void FlatHashMap<Key, Value, Hash>::rehash() {
    std::vector<Element> oldData = std::move(m_Data);
    m_Data.resize(oldData.size() * 2);
    m_Count = 0;

    for (auto & element : oldData) {
        if (element.status == Status::OCCUPIED) {
            (*this)[std::move(element.kv.key)] = std::move(element.kv.value);
        }
    }
}

template<typename Key, typename Value, typename Hash>
std::size_t FlatHashMap<Key, Value, Hash>::nextCell(const std::size_t index, const std::size_t shift) const {
    return (index + shift) & (m_Data.size() - 1);
}

template<typename Key, typename Value, typename Hash>
std::size_t FlatHashMap<Key, Value, Hash>::findIndex(const Key & key) const {
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

template<typename Key, typename Value, typename Hash>
std::size_t FlatHashMap<Key, Value, Hash>::getNextPosition(const Key & key) const {
    const std::size_t index = hash(key);
    std::size_t firstDeleted = OUT_OF_RANGE;
    std::size_t shift = 0;

    while (true) {
        std::size_t pos = nextCell(index, shift);

        if (m_Data[pos].kv.key == key && m_Data[pos].status == Status::OCCUPIED) return pos;

        if (m_Data[pos].status == Status::DELETED && firstDeleted == OUT_OF_RANGE) {
            firstDeleted = pos;
        }

        if (m_Data[pos].status == Status::FREE) {
            return (firstDeleted != OUT_OF_RANGE) ? firstDeleted : pos;
        }

        ++shift;
    }
}

template<typename Key, typename Value, typename Hash>
float FlatHashMap<Key, Value, Hash>::loadFactor() const {
    return static_cast<float>(m_Count) / m_Data.size();
}