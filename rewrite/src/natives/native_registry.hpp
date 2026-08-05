#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <unordered_map>

namespace reaper::natives
{
    using native_hash = std::uint64_t;
    using native_handler = void(*)(void* context);

    constexpr std::uint32_t joaat(std::string_view value) noexcept
    {
        std::uint32_t hash{};
        for (char ch : value)
        {
            const auto c = static_cast<unsigned char>(ch >= 'A' && ch <= 'Z' ? ch + ('a' - 'A') : ch);
            hash += c;
            hash += hash << 10;
            hash ^= hash >> 6;
        }
        hash += hash << 3;
        hash ^= hash >> 11;
        hash += hash << 15;
        return hash;
    }

    class native_registry final
    {
    public:
        void clear();
        void register_handler(native_hash hash, native_handler handler);
        [[nodiscard]] std::optional<native_handler> find(native_hash hash) const;
        [[nodiscard]] std::size_t size() const noexcept;

    private:
        std::unordered_map<native_hash, native_handler> m_handlers;
    };
}
