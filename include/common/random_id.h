#pragma once

#include <cstdint>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

namespace chatroom::common {

inline std::string GenerateRandomHexId() {
    thread_local std::random_device randomDevice;
    thread_local std::mt19937_64 generator(randomDevice());
    std::uniform_int_distribution<std::uint64_t> distribution;

    std::ostringstream output;
    output << std::hex << std::setfill('0')
           << std::setw(16) << distribution(generator)
           << std::setw(16) << distribution(generator);
    return output.str();
}

}  // namespace chatroom::common
