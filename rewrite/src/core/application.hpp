#pragma once

#include <atomic>
#include <thread>

namespace reaper::core
{
    class application final
    {
    public:
        static application& instance();

        bool start();
        void request_stop();
        void stop();

        application(const application&) = delete;
        application& operator=(const application&) = delete;

    private:
        application() = default;
        ~application();

        void run();

        std::atomic_bool m_running{};
        std::thread m_thread;
    };
}
