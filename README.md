# LRUCache

A simple C++ implementation of a **Least Recently Used (LRU) Cache**.

## Features

* O(1) lookup
* O(1) insertion
* O(1) eviction
* Configurable capacity
* STL-style iterators
* Header-only

## Example

```cpp
#include <blazed/lru.hpp>
#include <blazed/lru_format.hpp>
#include <print>

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  blazed::LRUCache<int, int> lru(3);
  for (int i = 0; i < static_cast<int>(lru.capacity()); i++) {
    lru.insert(i, i);
  }

  std::println("{}", lru);

  lru.insert(5, 5);
  std::println("{:d}", lru);

  return 0;
}
```

## Implementation

The cache is implemented using:

* `std::unordered_map` for fast key lookup
* `std::list` for tracking usage order

The front of the list contains the most recently used item, while the back contains the least recently used item.

## Complexity

| Operation | Complexity |
| --------- | ---------- |
| Lookup    | O(1)       |
| Insert    | O(1)       |
| Erase     | O(1)       |
| Update    | O(1)       |

## License

MIT License.
