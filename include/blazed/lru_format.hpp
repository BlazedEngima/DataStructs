#include <format>

#include "lru.hpp"

namespace std {

template <typename K, typename V, typename Hash, typename KeyEqual,
          typename Allocator, typename CharT>
struct formatter<blazed::LRUCache<K, V, Hash, KeyEqual, Allocator>, CharT> {
  bool debug_mode = false;

  constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) {
    auto it = ctx.begin();

    if (it != ctx.end() && *it == 'd') {
      debug_mode = true;
      ++it;
    }

    if (it != ctx.end() && *it != '}') {
      throw std::format_error("invalid LRUCache format specifier");
    }

    return it;
  }

  template <typename FormatContext>
  auto format(const blazed::LRUCache<K, V, Hash, KeyEqual, Allocator>& cache,
              FormatContext& ctx) const {
    auto out = ctx.out();

    if (!debug_mode) {
      out = std::format_to(out, "[");

      bool first = true;

      for (const auto& [key, value] : cache) {
        if (!first) {
          out = std::format_to(out, ", ");
        }

        out = std::format_to(out, "{} -> {}", key, value);

        first = false;
      }

      return std::format_to(out, "]");
    }

    out = std::format_to(out,
                         "LRUCache {{\n"
                         "  size: {},\n"
                         "  capacity: {},\n"
                         "  entries (MRU -> LRU): [\n",
                         cache.size(), cache.capacity());

    for (const auto& [key, value] : cache) {
      out = std::format_to(out, "    {{ key: {}, value: {} }},\n", key, value);
    }

    out = std::format_to(out,
                         "  ]\n"
                         "}}");

    return out;
  }
};

}  // namespace std
