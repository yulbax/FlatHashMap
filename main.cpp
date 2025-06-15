#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <cassert>
#include <chrono>
#include "flathashmap.hpp"
#include <boost/unordered/unordered_flat_map.hpp>


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
  constexpr int NUM_ITERATIONS = 100;

  std::vector<std::string> keys;
  keys.reserve(NUM_ELEMENTS);
  for (int i = 0; i < NUM_ELEMENTS; ++i) {
    keys.emplace_back(generateRandomString(10));
  }

  std::vector keysToErase(keys.begin(), keys.begin() + NUM_ELEMENTS / 2);

  double totalFlatTime = 0;
  double totalStdUnorderedTime = 0;

  for (int iter = 0; iter < NUM_ITERATIONS; ++iter) {

    std::cout << "\r" << "Progress: " << iter << "/" << 100 << " %" << std::flush;

    // === FlatHashMap test ===
    {
      auto start = std::chrono::high_resolution_clock::now();

      FlatHashMap<std::string, int> flatMap;
      for (int i = 0; i < NUM_ELEMENTS; ++i) {
        flatMap[keys[i]] = i;
      }

      int sum = 0;
      for (int i = 0; i < NUM_ELEMENTS; ++i) {
        sum += flatMap[keys[i]];
      }

      for (int i = 0; i < NUM_ELEMENTS / 2; ++i) {
        flatMap.erase(keysToErase[i]);
      }

      auto end = std::chrono::high_resolution_clock::now();
      totalFlatTime += std::chrono::duration<double>(end - start).count();
    }

    // === std::unordered_map test ===
    {
      auto start = std::chrono::high_resolution_clock::now();

      std::unordered_map<std::string, int> stdMap;
      for (int i = 0; i < NUM_ELEMENTS; ++i) {
        stdMap[keys[i]] = i;
      }

      int sum = 0;
      for (int i = 0; i < NUM_ELEMENTS; ++i) {
        sum += stdMap[keys[i]];
      }

      for (int i = 0; i < NUM_ELEMENTS / 2; ++i) {
        stdMap.erase(keysToErase[i]);
      }

      auto end = std::chrono::high_resolution_clock::now();
      totalStdUnorderedTime += std::chrono::duration<double>(end - start).count();
    }
  }

  std::cout << "\r" << "Progress: " << 100 << "/" << 100 << " %" << std::endl;

  std::cout << "=================================\n";

  std::cout << "Average FlatHashMap time: " << (totalFlatTime / NUM_ITERATIONS) << " seconds" << std::endl;
  std::cout << "Average std::unordered_map time: " << (totalStdUnorderedTime / NUM_ITERATIONS) << " seconds" << std::endl;
  std::cout << "Performance ratio (flat/std): "
            << (totalFlatTime / totalStdUnorderedTime) << std::endl;

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

  FlatHashMap<Person, std::string, PersonHasher> personMap;

  Person p1{"Alice", 30};
  Person p2{"Bob", 25};
  Person p3{"Charlie", 35};

  personMap[p1] = "Developer";
  personMap[p2] = "Designer";
  personMap[p3] = "Manager";

  assert(personMap[p1] == "Developer");
  assert(personMap[p2] == "Designer");
  assert(personMap[p3] == "Manager");

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
    testStress();

    std::cout << "\nAll tests passed successfully!" << std::endl;
  } catch (const std::exception & e) {
    std::cerr << "Test failed with exception: " << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "Test failed with unknown exception" << std::endl;
    return 1;
  }

  return 0;
}

int main() {
  return performTest();
}
