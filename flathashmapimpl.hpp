#pragma once
#include <vector>
#include <utility>

// NESTED OBJECTS
namespace FlatHashMapImpl {
    enum class Status { FREE, OCCUPIED, DELETED };

    template<typename Key, typename Value>
    class KeyValue {
    public:
        KeyValue() = default;

        template <typename K, typename V>
        KeyValue(K && k, V && v)
            : key(std::forward<K>(k)), value(std::forward<V>(v)) {}

        Key key;
        Value value;

        template<std::size_t I>
        auto & get() & {
            if constexpr (I == 0) {
                return static_cast<const Key&>(key);
            } else if constexpr (I == 1) {
                return value;
            }
        }

        template<std::size_t I>
        const auto & get() const& {
            if constexpr (I == 0) {
                return key;
            } else if constexpr (I == 1) {
                return value;
            }
        }

        template<std::size_t I>
        auto && get() && {
            if constexpr (I == 0) {
                return static_cast<const Key&&>(std::move(key));
            } else if constexpr (I == 1) {
                return std::move(value);
            }
        }

        template<std::size_t I>
        const auto && get() const&& {
            if constexpr (I == 0) {
                return std::move(key);
            } else if constexpr (I == 1) {
                return std::move(value);
            }
        }
    };

    template<typename Key, typename Value>
    class Element {
    public:
        Element() : kv(), status(Status::FREE) {}

        template <typename K, typename V>
        Element(K && key, V && value)
            : kv(std::forward<K>(key), std::forward<V>(value)), status(Status::FREE) {}

        KeyValue<Key, Value> kv;
        Status status;
    };

    template<typename VecRef, typename Key, typename ValType>
    class IteratorBase {
        struct Proxy {
            const Key & key;
            ValType & value;

            Proxy * operator->() { return this; }
        };
    public:
        IteratorBase(VecRef & data, std::size_t index) : m_Data(data), m_Index(index) {
            skipToOccupied();
        }

        IteratorBase & operator++() {
            ++m_Index;
            skipToOccupied();
            return *this;
        }

        auto & operator*() {
            return m_Data[m_Index].kv;
        }

        const auto & operator*() const {
            return m_Data[m_Index].kv;
        }

        auto operator->() {
            return Proxy{m_Data[m_Index].kv.key, m_Data[m_Index].kv.value};
        }

        auto operator->() const {
            return Proxy{m_Data[m_Index].kv.key, m_Data[m_Index].kv.value};
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

    template<typename Key, typename Value>
    using Iterator = IteratorBase<std::vector<Element<Key, Value>>, Key, Value>;

    template<typename Key, typename Value>
    using ConstIterator = IteratorBase<const std::vector<Element<Key, Value>>, Key, const Value>;
}

namespace std {
    template<typename Key, typename Value>
    struct tuple_size<FlatHashMapImpl::KeyValue<Key, Value>> {
        static constexpr std::size_t value = 2;
    };

    template<std::size_t I, typename Key, typename Value>
    struct tuple_element<I, FlatHashMapImpl::KeyValue<Key, Value>> {
        using type = std::conditional_t<I == 0, const Key, Value>;
    };

    template<std::size_t I, typename Key, typename Value>
    struct tuple_element<I, const FlatHashMapImpl::KeyValue<Key, Value>> {
        using type = std::conditional_t<I == 0, const Key, const Value>;
    };
}
