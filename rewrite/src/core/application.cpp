#include "core/application.hpp"

#include "core/logger.hpp"

#include <chrono>

namespace reaper::core
{
    application& application::instance()
    {
        static application value;
        return value;
    }

    application::~application()
    {
        stop();
    }

    bool application::start()
    {
        bool expected = false;
        if (!m_running.compare_exchange_strong(expected, true))
            return false;

        m_thread = std::thread(&application::run, this);
        return true;
    }

    void application::request_stop()
    {
        m_running.store(false);
    }

    void application::stop()
    {
        request_stop();
        if (m_thread.joinable())
            m_thread.join();
    }

    void application::run()
    {
        auto& log = logger::instance();
        log.initialize();
        log.write("info", "ReaperCore started");

        while (m_running.load())
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

        log.write("info", "ReaperCore stopped");
        log.shutdown();
    }
}
