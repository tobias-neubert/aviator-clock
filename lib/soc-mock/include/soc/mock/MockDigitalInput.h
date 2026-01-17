#pragma once

#include <soc/api/IDigitalInput.h>
#include <atomic>

namespace soc
{
  namespace mock
  {
    /**
     * @brief Mock digital input whose state can be controlled for testing.
     */
    class MockDigitalInput : public soc::api::IDigitalInput
    {
    public:
      // We assume the mock is for an active-low switch with pull-up.
      // So, not active (1) by default, active (0) when set.
      MockDigitalInput(int pin, bool pullup) : m_pin(pin), m_pullup(pullup), m_active(false) {}

      void begin() const override
      {
        // In mock, do nothing.
      }

      bool isActive() const override { return m_active; }

      /**
       * @brief Manually sets the active state of the mock input for testing.
       * @param active The state to set.
       */
      void setActive(bool active) { m_active = active; }

    private:
      int m_pin;
      bool m_pullup;
      std::atomic<bool> m_active;
    };
  } // namespace mock
} // namespace soc
