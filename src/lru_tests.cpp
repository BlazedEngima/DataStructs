#include <gtest/gtest.h>

#include <blazed/lru.hpp>
#include <memory>
#include <vector>

using blazed::LRUCache;

struct NonHashable {
  std::vector<int> x;
};

// Heterogeneous lookup test
struct string_hash {
  using is_transparent = void;

  template <typename T>
  size_t operator()(const T& t) const noexcept {
    return std::hash<std::string_view>{}(t);
  }
};

// Heterogeneous lookup test
struct string_equal {
  using is_transparent = void;

  template <typename A, typename B>
  bool operator()(const A& a, const B& b) const noexcept {
    return std::string_view(a) == std::string_view(b);
  }
};

// ----------------------------
// Basic insertion / lookup
// ----------------------------

TEST(LRUCache_Basic, InsertAndFind) {
  LRUCache<int, int> cache(3);

  cache.insert(1, 10);
  cache.insert(2, 20);

  EXPECT_TRUE(cache.contains(1));
  EXPECT_TRUE(cache.contains(2));

  auto it1 = cache.find(1);
  ASSERT_NE(it1, cache.end());
  EXPECT_EQ(it1->second, 10);

  auto it2 = cache.find(2);
  ASSERT_NE(it2, cache.end());
  EXPECT_EQ(it2->second, 20);
}

// ----------------------------
// insert() should NOT overwrite existing key
// ----------------------------

TEST(LRUCache_Insert, NoOverwrite) {
  LRUCache<int, int> cache(3);

  auto [it1, ok1] = cache.insert(1, 10);
  auto [it2, ok2] = cache.insert(1, 999);

  EXPECT_TRUE(ok1);
  EXPECT_FALSE(ok2);

  EXPECT_EQ(cache.find(1)->second, 10);
}

// ----------------------------
// insert_or_update should overwrite
// ----------------------------

TEST(LRUCache_InsertOrUpdate, Overwrite) {
  LRUCache<int, int> cache(3);

  cache.insert_or_update(1, 10);
  cache.insert_or_update(1, 99);

  EXPECT_EQ(cache.find(1)->second, 99);
}

// ----------------------------
// LRU eviction behavior
// ----------------------------

TEST(LRUCache_Eviction, EvictsLeastRecentlyUsed) {
  LRUCache<int, int> cache(3);

  cache.insert(1, 10);
  cache.insert(2, 20);
  cache.insert(3, 30);

  // touch 1 so 2 becomes LRU
  auto it = cache.find_with_recency_update(1);

  cache.insert(4, 40);  // should evict 2

  EXPECT_FALSE(cache.contains(2));
  EXPECT_TRUE(cache.contains(1));
  EXPECT_TRUE(cache.contains(3));
  EXPECT_TRUE(cache.contains(4));
}

// ----------------------------
// operator[] inserts default value
// ----------------------------

TEST(LRUCache_Subscript, DefaultInsert) {
  LRUCache<int, int> cache(3);

  cache[5] = 50;

  EXPECT_TRUE(cache.contains(5));
  EXPECT_EQ(cache.find(5)->second, 50);
}

// ----------------------------
// operator[] overwrites existing
// ----------------------------

TEST(LRUCache_Subscript, Overwrite) {
  LRUCache<int, int> cache(3);

  cache[1] = 10;
  cache[1] = 20;

  EXPECT_EQ(cache.find(1)->second, 20);
}

// ----------------------------
// erase works
// ----------------------------

TEST(LRUCache_Erase, RemovesElement) {
  LRUCache<int, int> cache(3);

  cache.insert(1, 10);
  cache.insert(2, 20);

  EXPECT_TRUE(cache.erase(1));
  EXPECT_FALSE(cache.contains(1));
  EXPECT_TRUE(cache.contains(2));
}

// ----------------------------
// get() updates recency
// ----------------------------

TEST(LRUCache_Recency, GetPromotesToMRU) {
  LRUCache<int, int> cache(3);

  cache.insert(1, 10);
  cache.insert(2, 20);
  cache.insert(3, 30);

  auto ref = cache.get(1);  // 1 becomes MRU

  cache.insert(4, 40);  // should evict 2

  EXPECT_FALSE(cache.contains(2));
  EXPECT_TRUE(cache.contains(1));
}

// ----------------------------
// iterator validity + order sanity
// ----------------------------

TEST(LRUCache_Iterator, IteratesInMRUOrder) {
  LRUCache<int, int> cache(3);

  cache.insert(1, 10);
  cache.insert(2, 20);
  cache.insert(3, 30);

  // MRU order should be 3,2,1
  std::vector<int> keys;
  for (auto& [k, v] : cache) {
    keys.push_back(k);
  }

  EXPECT_EQ(keys.size(), 3);
  EXPECT_EQ(keys[0], 3);
  EXPECT_EQ(keys[1], 2);
  EXPECT_EQ(keys[2], 1);
}

// ----------------------------
// capacity constraint sanity
// ----------------------------

TEST(LRUCache_Capacity, NeverExceedsCapacity) {
  LRUCache<int, int> cache(3);

  for (int i = 0; i < 10; i++) {
    cache.insert_or_update(i, i);
  }

  EXPECT_LE(cache.size(), 3);
}

TEST(LRUCache_Stress, RandomOperationsDontCrash) {
  LRUCache<int, int> cache(50);

  for (int i = 0; i < 10000; i++) {
    cache.insert_or_update(i % 200, i);

    if (i % 3 == 0) cache.erase(i % 200);
    if (i % 5 == 0) auto it = cache.find(i % 200);
    if (i % 7 == 0) {
      if (cache.contains(i % 200)) auto ref = cache.get(i % 200);
    }
  }

  EXPECT_LE(cache.size(), 50);
}

TEST(LRUCache_Heterogeneous, StringAndStringViewLookup) {
  LRUCache<std::string, int, string_hash, string_equal> cache(5);

  cache.insert("hello", 1);
  cache.insert("world", 2);

  std::string_view key1 = "hello";
  std::string_view key2 = "world";

  EXPECT_TRUE(cache.contains(std::string("hello")));
  EXPECT_TRUE(cache.contains(std::string("world")));

  auto v1 = cache.get(key1);
  auto v2 = cache.get(key2);

  EXPECT_EQ(v1, 1);
  EXPECT_EQ(v2, 2);

  auto it = cache.find_with_recency_update(key1);
  ASSERT_NE(it, cache.end());
  EXPECT_EQ(it->second, 1);
}

TEST(LRUCache_Heterogeneous, EraseWithDifferentKeyType) {
  LRUCache<std::string, int, string_hash, string_equal> cache(5);

  cache.insert("abc", 123);

  std::string_view key = "abc";

  EXPECT_TRUE(cache.erase(key));
  EXPECT_FALSE(cache.contains("abc"));
}

TEST(LRUCache_Heterogeneous, IntegerKeyCompatibility) {
  LRUCache<int, int> cache(5);

  cache.insert(1, 10);

  long key = 1;

  EXPECT_TRUE(cache.contains(key));
  EXPECT_EQ(cache.get(key), 10);

  cache.erase(key);
  EXPECT_FALSE(cache.contains(1));
}

TEST(LRUCache_Heterogeneous, MixedAccessPatterns) {
  LRUCache<std::string, int, string_hash, string_equal> cache(10);

  cache.insert("a", 1);
  cache.insert("b", 2);
  cache.insert("c", 3);

  std::string_view a = "a";
  const char* b = "b";

  EXPECT_EQ(cache.get(a), 1);
  EXPECT_EQ(cache.get(b), 2);

  cache.erase(a);
  EXPECT_FALSE(cache.contains("a"));
}

TEST(LRUCache_Types, MoveOnlyValue) {
  LRUCache<int, std::unique_ptr<int>> cache(5);

  cache.insert(1, std::make_unique<int>(42));

  cache[2] = std::move(std::make_unique<int>(5));

  EXPECT_NE(cache.find(1), cache.end());
  EXPECT_EQ(*cache.find(1)->second, 42);
  EXPECT_EQ(*cache.find(2)->second, 5);
}
