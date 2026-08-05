#include "natives/native_registry.hpp"

namespace reaper::natives
{
    void native_registry::clear()
    {
        m_handlers.clear();
    }

    void native_registry::register_handler(native_hash hash, native_handler handler)
    {
        if (handler)
            m_handlers[hash] = handler;
    }

    std::optional<native_handler> native_registry::find(native_hash hash) const
    {
        const auto it = m_handlers.find(hash);
        if (it == m_handlers.end())
            return std::nullopt;
        return it->second;
    }

    std::size_t native_registry::size() const noexcept
    {
        return m_handlers.size();
    }
}
