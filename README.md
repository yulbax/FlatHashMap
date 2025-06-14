# FlatHashMap

A high-performance, template-based hash map implementation using open addressing with quadratic probing. This container provides O(1) average-case performance for insertions, lookups, and deletions while maintaining memory efficiency through flat storage.

## Features

- **Open Addressing**: Uses quadratic probing for collision resolution
- **Template-based**: Generic implementation supporting any key-value types
- **Power-of-Two Sizing**: Automatic construction with power-of-two size
- **Automatic Rehashing**: Dynamically resizes when load factor exceeds threshold
- **STL-Compatible**: Provides iterators and familiar interface
- **Memory Efficient**: Flat array storage with minimal overhead

## Requirements

- C++20 compatible compiler
- Standard library support for `<vector>` and `<functional>`

## Usage

### Basic Operations

```cpp
#include "flathashmap.h"

// Create a hash map with default size (1024)
FlatHashMap<int, std::string> map;

// Insert key-value pairs
map.insert(1, "hello");
map.insert(2, "world");

// Access elements (creates if not exists)
map[3] = "new value";

// Check if key exists
if (map.contains(1)) {
    std::cout << "Key 1 exists\n";
}

// Get value with bounds checking
try {
    std::string& value = map.at(1);
    std::cout << "Value: " << value << "\n";
} catch (const std::out_of_range& e) {
    std::cout << "Key not found\n";
}

// Remove elements
map.erase(2);

// Clear all elements
map.clear();
```

### Custom Size and Hash Function

```cpp
// Custom size (will be scaled automatically to power of 2)
FlatHashMap<std::string, int> customSizeMap(512);

// Custom hash function
struct CustomHash {
    std::size_t operator()(const std::string& key) const {
        return std::hash<std::string>{}(key) * 31;
    }
};

FlatHashMap<std::string, int, CustomHash> customHashMap;
```

### Iteration

```cpp
FlatHashMap<int, std::string> map;
map[1] = "one";
map[2] = "two";
map[3] = "three";

// Range-based for loop
for (const auto& [key, value] : map) {
    std::cout << key << ": " << value << "\n";
}

// Iterator-based
for (auto it = map.begin(); it != map.end(); ++it) {
    auto & [key, value] = *it;
    std::cout << key << ": " << value << "\n";
}

// Find specific element
auto it = map.find(2);
if (it != map.end()) {
    auto & [key, value] = *it;
    std::cout << "Found: " << key << " -> " << value << "\n";
}
```

## How It Works

### Open Addressing with Quadratic Probing

The FlatHashMap uses **open addressing** instead of chaining for collision resolution. When a collision occurs, it uses **quadratic probing** to find the next available slot:

```
nextPosition = (hash + i²) % tableSize
```

This approach provides:
- Better cache locality compared to chaining
- Reduced memory overhead (no linked lists)
- Predictable memory access patterns

### Storage Strategy

Each element in the hash table has three states:
- **FREE**: Never used
- **OCCUPIED**: Contains valid key-value pair
- **DELETED**: Previously occupied but erased (tombstone)

This tombstone approach allows for efficient deletion without disrupting probe sequences.

### Automatic Rehashing

The container automatically rehashes when the load factor exceeds 87.5% (configurable). During rehashing:
1. Table size doubles (maintaining power-of-2 constraint)
2. All elements are rehashed into new positions
3. Deleted elements are cleaned up

### Performance Characteristics

- **Average Case**: O(1) for insert, lookup, delete
- **Space Complexity**: O(n) with low overhead
- **Load Factor**: Maintained below 87.5% for optimal performance

### Performance Comparison
Benchmark results comparing FlatHashMap with std::unordered_map (100,000 iterations):
| Container               | Avg Time (seconds) |
|-------------------------|--------------------|
| `FlatHashMap`           | **0.0122**         |
| `std::unordered_map`    | 0.0182             |
| `ratio (flat/std)`      | 0.67               |


## Template Parameters

| Parameter | Description        | Default          | Constraints                     |
|-----------|--------------------|------------------|---------------------------------|
| `Key`     | Key type           | -                | Must be equality comparable     |
| `Value`   | Value type         | -                | Must be copy/move constructible |
| `Size`    | Initial table size | 1024             | Must be power of 2              |
| `Hash`    | Hash function type | `std::hash<Key>` | Must be callable with Key       |

## Thread Safety

This container is **not thread-safe**. External synchronization is required for concurrent access.

## Implementation Notes

- Uses `std::vector` for underlying storage
- Bitwise AND operation for fast modulo (size automatically scales to power-of-2)
- Perfect forwarding for efficient key-value insertion
