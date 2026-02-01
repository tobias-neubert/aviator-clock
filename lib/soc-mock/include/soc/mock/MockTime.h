#pragma once

#include <soc/api/ITime.h>
#include <chrono>

namespace soc
{
  namespace mock
  {
    /**
     * @brief Mock time provider that simulates time passing for host tests.
     * It uses the host's steady clock to simulate uptime.
     */
    class MockTime : public soc::api::ITime
    {
    public:
      MockTime() : m_startTime(std::chrono::steady_clock::now()) {}

      unsigned long now() override
      {
        // Return milliseconds since epoch for 'now'. For a mock, we can
        // simulate it by adding our uptime to a fixed start time.
        auto epoch_start = std::chrono::system_clock::from_time_t(1577836800); // 2020-01-01
        auto uptime = std::chrono::steady_clock::now() - m_startTime;
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   epoch_start.time_since_epoch() + uptime)
            .count();
      }

      bool asTimeComponents(TimeComponents &time) override
      {
        // Return a fixed time for predictability in tests.
        // Let's say 10:20:30 for example.
        time.hours = 10;
        time.minutes = 20;
        time.seconds = 30;
        return true;
      }

      unsigned long millis() const override
      {
        auto uptime = std::chrono::steady_clock::now() - m_startTime;
        return std::chrono::duration_cast<std::chrono::milliseconds>(uptime).count();
      }

    private:
      std::chrono::steady_clock::time_point m_startTime;
    };
  } // namespace mock
} // namespace soc
