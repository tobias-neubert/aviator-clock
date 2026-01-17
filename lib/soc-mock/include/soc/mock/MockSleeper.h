#pragma once

#include <soc/api/ISleeper.h>
#include <thread>
#include <chrono>

namespace soc
{
  namespace mock
  {
    /**
     * @brief Mock sleeper implementation that uses std::this_thread::sleep_for.
     */
    class MockSleeper : public soc::api::ISleeper
    {
    public:
      void sleep(uint32_t millis) override
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(millis));
      }
    };
  } // namespace mock
} // namespace soc
