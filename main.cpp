#include <iostream>
#include <utility>
#include <vector>
#include <random>
#include <string>
#include <functional>
#include <stdexcept>
#include <unordered_map>
#include <cassert>
#include <chrono>
#include <map>

struct StringHasher {
  std::size_t operator()(const std::string & s) const {
    std::size_t hash = 0;

    for (const char c : s) {
      hash = hash * 31 + c;
    }
    return hash;
  }
};

template<std::size_t size>
concept PowerOfTwo = (size != 0) && ((size & (size - 1)) == 0);

template<typename Key, typename Value,
         std::size_t Size = 1024, typename Hash = std::hash<Key>>
requires PowerOfTwo<Size>
class FlatHashMap {
  enum Status { FREE, OCCUPIED, DELETED };

  class Element {
  public:
    Element() : key(Key{}), value(Value{}), status(FREE) {}

    Element(Key key, Value value, const Status status) : key(std::move(key)), value(std::move(value)), status(status) {}

    Key key;
    Value value;
    Status status;
  };

  static constexpr std::size_t MAX_CHAIN = 256;
  static constexpr std::size_t OUT_OF_RANGE = -1;
  static constexpr float LOAD_FACTOR = 0.875;

public:
  FlatHashMap()
    : m_Data(Size), m_Hasher(), m_LoadFactor(LOAD_FACTOR), m_Count(0), m_Size(Size) {}

  class Iterator;

  class ConstIterator;

  template<typename K>
  Value & operator[](K && key) {
    if (loadFactor() > m_LoadFactor) {
      rehash();
    }

    const std::size_t index = getNextPosition(key);
    if (m_Data[index].status != OCCUPIED) {
      m_Data[index].key = std::forward<K>(key);
      m_Data[index].status = OCCUPIED;
      ++m_Count;
    }

    return m_Data[index].value;
  }

  template<typename K, typename V>
  bool insert(K && key, V && value) {
    if (loadFactor() > m_LoadFactor) {
      rehash();
    }

    const std::size_t index = getNextPosition(key);

    if (m_Data[index].status == OCCUPIED) {
      return false;
    }

    ++m_Count;
    m_Data[index].key = std::forward<K>(key);
    m_Data[index].value = std::forward<V>(value);
    m_Data[index].status = OCCUPIED;

    return true;
  }

  Value & at(const Key & key) {
    std::size_t index = findIndex(key);
    if (index == OUT_OF_RANGE) {
      throw std::out_of_range("Key not found");
    }
    return m_Data[index].value;
  }

  [[nodiscard]] const Value & at(const Key & key) const {
    std::size_t index = findIndex(key);
    if (index == OUT_OF_RANGE) {
      throw std::out_of_range("Key not found");
    }
    return m_Data[index].value;
  }

  bool contains(const Key & key) {
    return findIndex(key) != OUT_OF_RANGE;
  }

  [[nodiscard]] bool contains(const Key & key) const {
    return findIndex(key) != OUT_OF_RANGE;
  }

  bool erase(const Key & key) {
    std::size_t pos = findIndex(key);
    if (pos != OUT_OF_RANGE) {
      m_Data[pos].key = Key{};
      m_Data[pos].status = DELETED;
      --m_Count;
      return true;
    }
    return false;
  }

  void clear() {
    m_Data.clear();
    m_Data.resize(m_Size);
    m_Count = 0;
  }

  Iterator begin() {
    return Iterator(m_Data, 0);
  }

  Iterator end() {
    return Iterator(m_Data, m_Data.size());
  }

  [[nodiscard]] ConstIterator begin() const {
    return ConstIterator(m_Data, 0);
  }

  [[nodiscard]] ConstIterator end() const {
    return ConstIterator(m_Data, m_Data.size());
  }

  Iterator find(const Key & key) {
    std::size_t index = findIndex(key);
    if (index == OUT_OF_RANGE) {
      return end();
    }
    return Iterator(m_Data, index);
  }

  [[nodiscard]] ConstIterator find(const Key & key) const {
    std::size_t index = findIndex(key);
    if (index == OUT_OF_RANGE) {
      return end();
    }
    return ConstIterator(m_Data, index);
  }

private:
  [[nodiscard]] std::size_t hash(const Key & key) const {
    return m_Hasher(key) & (m_Data.size() - 1);
  }

  void rehash() {
    std::vector<Element> oldData = std::move(m_Data);
    m_Data.resize(oldData.size() * 2);
    m_Count = 0;

    for (auto & element : oldData) {
      if (element.status == OCCUPIED) {
        (*this)[std::move(element.key)] = std::move(element.value);
      }
    }
  }

  [[nodiscard]] std::size_t nextCell(const size_t index, const std::size_t shift) const {
    return (index + shift * shift) & (m_Data.size() - 1);
  }

  [[nodiscard]] std::size_t findIndex(const Key & key) const {
    const std::size_t index = hash(key);

    for (std::size_t shift = 0; shift < MAX_CHAIN; ++shift) {
      std::size_t pos = nextCell(index, shift);

      if (m_Data[pos].status == FREE) break;

      if (m_Data[pos].status == OCCUPIED && m_Data[pos].key == key) {
        return pos;
      }
    }

    return OUT_OF_RANGE;
  }

  std::size_t getNextPosition(const Key & key) {
    const std::size_t index = hash(key);
    std::size_t firstDeleted = OUT_OF_RANGE;

    for (std::size_t shift = 0; shift < MAX_CHAIN; ++shift) {
      std::size_t pos = nextCell(index, shift);

      if (m_Data[pos].key == key) {
        return pos;
      }

      if (m_Data[pos].status == DELETED && firstDeleted == OUT_OF_RANGE) {
        firstDeleted = pos;
      }

      if (m_Data[pos].status == FREE) {
        return (firstDeleted != OUT_OF_RANGE) ? firstDeleted : pos;
      }
    }

    rehash();
    return getNextPosition(key);
  }

  [[nodiscard]] float loadFactor() const {
    return static_cast<float>(m_Count) / m_Data.size();
  }

  std::vector<Element> m_Data;
  Hash m_Hasher;
  float m_LoadFactor;
  std::size_t m_Count;
  std::size_t m_Size;
};


template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
class FlatHashMap<Key, Value, Size, Hash>::Iterator {
public:
  Iterator(std::vector<Element> & data, std::size_t index)
    : m_Data(data), m_Index(index) {
    while (m_Index < m_Data.size() &&
           m_Data[m_Index].status != OCCUPIED) {
      ++m_Index;
    }
  }

  Iterator & operator++() {
    ++m_Index;
    while (m_Index < m_Data.size() &&
           m_Data[m_Index].status != OCCUPIED) {
      ++m_Index;
    }
    return *this;
  }

  std::pair<const Key &, Value &> operator*() const {
    return {m_Data[m_Index].key, m_Data[m_Index].value};
  }

  bool operator!=(const Iterator & other) const {
    return m_Index != other.m_Index;
  }

private:
  std::vector<Element> & m_Data;
  std::size_t m_Index;
};

template<typename Key, typename Value, std::size_t Size, typename Hash>
requires PowerOfTwo<Size>
class FlatHashMap<Key, Value, Size, Hash>::ConstIterator {
public:
  ConstIterator(const std::vector<Element> & data, std::size_t index)
    : m_Data(data), m_Index(index) {
    while (m_Index < m_Data.size() &&
           m_Data[m_Index].status != OCCUPIED) {
      ++m_Index;
    }
  }

  ConstIterator & operator++() {
    ++m_Index;
    while (m_Index < m_Data.size() &&
           m_Data[m_Index].status != OCCUPIED) {
      ++m_Index;
    }
    return *this;
  }

  std::pair<const Key &, const Value &> operator*() const {
    return {m_Data[m_Index].key, m_Data[m_Index].value};
  }

  bool operator!=(const ConstIterator & other) const {
    return m_Index != other.m_Index;
  }

private:
  const std::vector<Element> & m_Data;
  std::size_t m_Index;
};

std::string generateRandomString(std::size_t length) {
  static constexpr char alphanum[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, sizeof(alphanum) - 2);

  std::string result;
  result.reserve(length);
  for (std::size_t i = 0; i < length; ++i) {
    result += alphanum[dis(gen)];
  }

  return result;
}

void testBasicOperations() {
  std::cout << "Testing basic operations..." << std::endl;

  FlatHashMap<std::string, int> map;

  map["one"] = 1;
  map["two"] = 2;
  map["three"] = 3;

  assert(map["one"] == 1);
  assert(map["two"] == 2);
  assert(map["three"] == 3);

  map["one"] = 10;
  assert(map["one"] == 10);

  assert(map.insert("four", 4));
  assert(map["four"] == 4);

  assert(!map.insert("four", 44));
  assert(map["four"] == 4);

  assert(map.contains("one"));
  assert(map.contains("two"));
  assert(map.contains("three"));
  assert(map.contains("four"));
  assert(!map.contains("five"));

  assert(map.at("one") == 10);
  try {
    map.at("five");
    assert(false);
  } catch (const std::out_of_range &) {

  }

  map.erase("one");
  assert(!map.contains("one"));

  map.erase("nonexistent");

  std::cout << "Basic operations test passed!" << std::endl;
}

void testRehashing() {
  std::cout << "Testing rehashing functionality..." << std::endl;

  FlatHashMap<int, int> map;

  constexpr int NUM_ELEMENTS = 1000;
  for (int i = 0; i < NUM_ELEMENTS; ++i) {
    map[i] = i * 10;
  }

  for (int i = 0; i < NUM_ELEMENTS; ++i) {
    assert(map.contains(i));
    assert(map[i] == i * 10);
  }

  for (int i = 0; i < NUM_ELEMENTS; i += 2) {
    map.erase(i);
  }

  for (int i = 0; i < NUM_ELEMENTS; ++i) {
    if (i % 2 == 0) {
      assert(!map.contains(i));
    } else {
      assert(map.contains(i));
      assert(map[i] == i * 10);
    }
  }

  std::cout << "Rehashing test passed!" << std::endl;
}

void testPerformanceComparison() {
  std::cout << "Performance comparison with std::unordered_map..." << std::endl;

  constexpr int NUM_ELEMENTS = 100000;
  std::vector<std::string> keys;

  for (int i = 0; i < NUM_ELEMENTS; ++i) {
    keys.push_back(generateRandomString(10));
  }

  auto startFlat = std::chrono::high_resolution_clock::now();

  FlatHashMap<std::string, int> flatMap;
  for (int i = 0; i < NUM_ELEMENTS; ++i) {
    flatMap[keys[i]] = i;
  }

  int sumFlat = 0;
  for (int i = 0; i < NUM_ELEMENTS; ++i) {
    sumFlat += flatMap[keys[i]];
  }

  std::cout << "Sum Flat: " << sumFlat << std::endl;

  auto endFlat = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsedFlat = endFlat - startFlat;

  auto startStd = std::chrono::high_resolution_clock::now();

  std::unordered_map<std::string, int> stdMap;
  for (int i = 0; i < NUM_ELEMENTS; ++i) {
    stdMap[keys[i]] = i;
  }

  int sumStd = 0;
  for (int i = 0; i < NUM_ELEMENTS; ++i) {
    sumStd += stdMap[keys[i]];
  }

  std::cout << "Sum Std: " << sumStd << std::endl;

  auto endStd = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsedStd = endStd - startStd;

  auto startMap = std::chrono::high_resolution_clock::now();

  std::map<std::string, int> stdMap1;
  for (int i = 0; i < NUM_ELEMENTS; ++i) {
    stdMap[keys[i]] = i;
  }

  int sumMap = 0;
  for (int i = 0; i < NUM_ELEMENTS; ++i) {
    sumMap += stdMap1[keys[i]];
  }

  std::cout << "Sum Map: " << sumMap << std::endl;

  auto endMap = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsedMap = endMap - startMap;


  std::cout << "FlatHashMap time: " << elapsedFlat.count() << " seconds" << std::endl;
  std::cout << "std::unordered_map time: " << elapsedStd.count() << " seconds" << std::endl;
  std::cout << "std::map time: " << elapsedMap.count() << " seconds" << std::endl;
  std::cout << "Performance ratio (flat/std): " << elapsedFlat.count() / elapsedStd.count() << std::endl;

  assert(sumFlat == sumStd);

  std::cout << "Performance comparison completed!" << std::endl;
}

void testEdgeCases() {
  std::cout << "Testing edge cases..." << std::endl;

  FlatHashMap<std::string, int> emptyMap;
  assert(!emptyMap.contains("key"));

  try {
    emptyMap.at("key");
    assert(false);
  } catch (const std::out_of_range &) {

  }

  emptyMap.erase("key");

  FlatHashMap<std::string, int> singleMap;
  singleMap["key"] = 42;

  assert(singleMap.contains("key"));
  assert(singleMap["key"] == 42);

  singleMap.erase("key");
  assert(!singleMap.contains("key"));

  assert(singleMap["newkey"] == 0);
  assert(singleMap.contains("newkey"));

  std::cout << "Edge cases test passed!" << std::endl;
}

void testComplexTypes() {
  std::cout << "Testing with complex types..." << std::endl;

  struct Person {
    std::string name;
    int age{};

    bool operator==(const Person & other) const {
      return name == other.name && age == other.age;
    }
  };

  struct PersonHasher {
    std::size_t operator()(const Person & p) const {
      return std::hash<std::string>{}(p.name) ^ std::hash<int>{}(p.age);
    }
  };

  FlatHashMap<Person, std::string, 1024 ,PersonHasher> personMap;

  Person p1{"Alice", 30};
  Person p2{"Bob", 25};
  Person p3{"Charlie", 35};

  personMap[p1] = "Developer";
  personMap[p2] = "Designer";
  personMap[p3] = "Manager";

  assert(personMap[p1] == "Developer");
  assert(personMap[p2] == "Designer");
  assert(personMap[p3] == "Manager");

  const Person & p1Copy = p1;
  assert(personMap[p1Copy] == "Developer");

  personMap[p1] = "Senior Developer";
  assert(personMap[p1] == "Senior Developer");

  personMap.erase(p2);
  assert(!personMap.contains(p2));
  assert(personMap.contains(p1));
  assert(personMap.contains(p3));

  std::cout << "Complex types test passed!" << std::endl;
}

void testStress() {
  FlatHashMap<int, int> map;
  for (int i = 0; i < 100000; ++i) map[i] = i;
  for (int i = 0; i < 50000; ++i) map.erase(i);
  for (int i = 0; i < 50000; ++i) map[i] = i * 2;
  for (int i = 0; i < 100000; ++i)
    assert(map.contains(i));
  std::cout << "Stress test passed!" << std::endl;
}

int performTest() {
  try {
    testBasicOperations();
    testRehashing();
    testEdgeCases();
    testComplexTypes();
    testPerformanceComparison();

    std::cout << "\nAll tests passed successfully!" << std::endl;
  } catch (const std::exception & e) {
    std::cerr << "Test failed with exception: " << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "Test failed with unknown exception" << std::endl;
    return 1;
  }

  testStress();

  return 0;
}


int main() {
  return performTest();
}
