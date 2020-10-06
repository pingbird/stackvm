#pragma once

#include <vector>
#include <string>
#include <unordered_set>

namespace BF {
  using RangePos = int32_t;

  struct Range {
    static const int min = INT32_MIN;
    static const int max = INT32_MAX;

    RangePos lower = 0;
    RangePos upper = 0;

    Range() = default;
    Range(RangePos lower, RangePos upper) : lower(lower), upper(upper) {}

    [[nodiscard]] bool isLowerUnbounded() const {
      return lower == min;
    }

    [[nodiscard]] bool isUpperUnbounded() const {
      return upper == max;
    }

    [[nodiscard]] std::string toString() const {
      if (lower >= upper) return "none";
      if (isUpperUnbounded()) {
        if (isLowerUnbounded()) {
          return "all";
        } else {
          return std::to_string(lower) + "<= x";
        }
      } else {
        if (isLowerUnbounded()) {
          return "x <" + std::to_string(upper);
        } else {
          if (lower == upper - 1) {
            return "x = " + std::to_string(lower);
          } else {
            return std::to_string(lower) + " <= x < " + std::to_string(upper);
          }
        }
      }
    }
  };

  struct Mask {
    std::vector<RangePos> indices = {Range::min};
#ifndef NDEBUG
    std::vector<Range> ranges;
#endif

    [[nodiscard]] bool isAll() const {
      return indices.empty();
    }

    [[nodiscard]] bool isNone() const {
      return indices.size() == 1 && indices[0] == Range::min;
    }

    size_t search(RangePos position, size_t min, size_t max) {
      assert(!indices.empty());
      assert(position >= indices[0]);
      if (min == max - 1) return min;
      size_t cur = (min + max) / 2;
      int index = indices[cur];
      if (index == position) {
        return cur;
      } else if (index > position) {
        return search(position, min, cur);
      } else {
        return search(position, cur + 1, max);
      }
    }

    bool check(int index) {
      size_t size = indices.size();
      if (size == 0 || index < indices[0]) {
        return true;
      } else if (size == 1 && indices[0] == Range::min) {
        return false;
      } else {
        return !(search(index, 0, indices.size()) & 1u);
      }
    }

    void addRange(const Range &range) {
      if (range.lower >= range.upper) {
        return;
      }
#ifndef NDEBUG
      ranges.push_back(range);
#endif
      size_t initialSize = indices.size();

      if (range.isLowerUnbounded()) {
        if (range.isUpperUnbounded()) {
          // We are now all, clear indices.
          indices.clear();
        } else {
          size_t upper = search(range.upper, 0, initialSize);
          if (upper & 1u) {
            // Our upper is above a lower, erase the lower and below.
            indices.erase(
              indices.begin(),
              indices.begin() + upper + 1
            );
          } else {
            // Our upper is above an upper, shift it up and delete everything below it.
            indices[upper] = range.upper;
            indices.erase(
              indices.begin(),
              indices.begin() + upper
            );
          }
        }
      } else if (range.isUpperUnbounded()) {
        size_t lower = search(range.lower, 0, initialSize);
        if (lower & 1u) {
          // Our lower is above an upper.
          if (lower == initialSize - 1) {
            // We are at the end, push lower.
            indices.push_back(range.lower);
          } else {
            // Shift back next lower and erase everything above it.
            indices[lower + 1] = range.lower;
            indices.erase(
              indices.begin() + lower + 2,
              indices.end()
            );
          }
        } else {
          // Our lower is above a lower, erase everything above it.
          indices.erase(
            indices.begin() + lower + 1,
            indices.end()
          );
        }
      } else {
        size_t lower = search(range.lower, 0, initialSize);
        if (lower & 1u) {
          // Range starts at a lower.
          size_t upper = search(range.upper, lower + 1, initialSize);
          if (upper & 1u) {
            // Our upper is above an upper, erase below the upper and shift it up
            indices[upper] = range.upper;
            indices.erase(
              indices.begin() + lower + 1,
              indices.begin() + upper
            );
          } else {
            // Our upper is above a lower, erase the lower and below.
            indices.erase(
              indices.begin() + lower + 1,
              indices.begin() + upper + 1
            );
          }
        } else {
          // Range starts at an upper.
          if (lower == initialSize - 1) {
            // We are at the end of the list, push range.
            indices.push_back(range.lower);
            indices.push_back(range.upper);
          } else {
            // We precede a lower.
            RangePos nextLower = indices[lower + 1];
            if (nextLower > range.upper) {
              // Our upper is less than the next lower, insert our range.
              indices.insert(indices.begin() + lower + 1, {
                range.lower,
                range.upper
              });
            } else {
              // Our upper is greater or equal to the next lower, move it back to our lower.
              size_t upper = search(range.upper, lower + 1, initialSize);
              indices[lower + 1] = range.lower;
              if (upper & 1u) {
                // Our upper is above an upper, erase below the upper and shift it up
                indices[upper] = range.upper;
                indices.erase(
                  indices.begin() + lower + 2,
                  indices.begin() + upper
                );
              } else {
                // Our upper is above a lower, erase the lower and below.
                indices.erase(
                  indices.begin() + lower + 2,
                  indices.begin() + upper + 1
                );
              }
            }
          }
        }
      }
    }

    void validate() {
#ifndef NDEBUG
      for (size_t i = 1; i < indices.size(); i++) {
        assert(indices[i] > indices[i + 1]);
      }

      for (const Range &range : ranges) {
        if (range.lower >= range.upper) continue;
        size_t lower = search(range.lower, 0, indices.size());
        assert(lower & 1u);
        size_t upper = search(range.upper - 1, 0, indices.size());
        assert(upper == lower);
      }
#endif
    }

    std::string toString() {
      std::string out;

      size_t size = indices.size();
      if (size == 0) {
        return "none";
      }

      bool pred = indices[0] > Range::min;
      if (pred) {
        out += "x < " + std::to_string(indices[0]);
      }

      for (size_t i = 0; i < ((size - 1) >> 1u); i++) {
        if (pred) {
          out += ", ";
        } else {
          pred = true;
        }
        size_t lower = indices[i + 1];
        size_t upper = indices[i + 2];
        out += std::to_string(lower) + "<= x < " + std::to_string(upper);
      }

      if (!(size & 1u)) {
        if (pred) {
          out += ", ";
        }
        out += std::to_string(indices[size - 1]) + "<= x";
      }

      return out;
    }
  };

  struct MaskPosition {
    RangePos offset;
    RangePos order;
  };

  struct Alias {
    std::map<MaskPosition, Mask> masks;
  };

  typedef uint8_t LoopBalance;
  static const uint8_t LB_BALANCED = 0;
  static const uint8_t LB_LEFT = 1;
  static const uint8_t LB_RIGHT = 2;
  static const uint8_t LB_UNBALANCED = 3;

  struct LoopInfo {
    Alias read;
    Alias write;
    LoopBalance balance;

    LoopInfo() = default;
  };

  struct Seek;
  struct SeekData {
    int offset = 0;
    Seek* loop;
    [[nodiscard]] bool equals(const Seek &other) const;
    [[nodiscard]] LoopBalance getBalance() const;
  };

  struct Seek {
    SeekData* data;
    Seek* next;
  };

  enum InstKind : uint8_t {
    I_ADD,
    I_SUB,
    I_SEEK,
    I_LOOP,
    I_END,
    I_PUTCHAR,
    I_GETCHAR,
  };

  struct Inst {
    InstKind kind;
    union {
      Seek* seek;
    };
  };

  struct Program {
    std::vector<Seek> seeks;
    std::vector<Inst> instructions;
  };

  Program parse(const std::string &str);
  std::string print(const Program &block);
  std::string printSeek(const Seek &seek);
}