#ifndef CPR_PAYLOAD_H
#define CPR_PAYLOAD_H

#include <cstdint>
#include <initializer_list>
#include <memory>
#include <string>

#include "cpr/curlholder.h"
#include <utility>

namespace cpr {

struct Pair {
    Pair(std::string&& p_key, std::string&& p_value)
            : key(std::move(p_key)), value(std::move(p_value)) {}
    Pair(std::string&& p_key, const std::int32_t& p_value)
            : key(std::move(p_key)), value{std::to_string(p_value)} {}

    std::string key;
    std::string value;
};

class Payload {
  public:
    template <class It>
    Payload(const It begin, const It end) {
        // Create a temporary CurlHolder for URL encoding:
        CurlHolder holder;
        for (It pair = begin; pair != end; ++pair) {
            AddPair(*pair, holder);
        }
    }
    Payload(const std::initializer_list<Pair>& pairs);

    void AddPair(const Pair& pair, const CurlHolder& holder);
    void AddPair(const Pair& pair);

    std::string content;
};

} // namespace cpr

#endif
