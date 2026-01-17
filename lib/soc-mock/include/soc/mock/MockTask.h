#pragma once

#include <soc/api/ITask.h>
#include <thread>

namespace soc
{
  namespace mock
  {
    /**
     * @brief Mock ITask implementation that wraps a std::thread.
     *
     * This RAII-style class ensures that the underlying thread is properly
     * joined upon destruction.
     */
    class MockTask : public soc::api::ITask
    {
    public:
      explicit MockTask(std::thread &&thread) : m_thread(std::move(thread)) {}

      ~MockTask() override
      {
        if (m_thread.joinable())
        {
          m_thread.join();
        }
      }

      // Prevent copying, allow moving.
      MockTask(const MockTask &) = delete;
      MockTask &operator=(const MockTask &) = delete;
      MockTask(MockTask &&) = default;
      MockTask &operator=(MockTask &&) = default;

    private:
      std::thread m_thread;
    };
  } // namespace mock
} // namespace soc
