#include <soc/mock/MockSoc.h>
#include <soc/mock/MockTask.h>
#include <soc/api/IDigitalInput.h>
#include <algorithm>
#include <thread>
#include <stdio.h>

namespace soc
{
  namespace mock
  {
    MockSoc::MockSoc()
    {
      m_timeProvider = std::make_unique<MockTime>();
      m_sleeper = std::make_unique<MockSleeper>();
      m_persistentStorage = std::make_unique<MockPersistentStorage>();
    }

    // The destructor can now be defaulted in the header, but it's also fine here.
    MockSoc::~MockSoc() = default;

    soc::api::Logger &MockSoc::getLogger() { return m_logger; }
    soc::api::ITime &MockSoc::getTimeProvider() { return *m_timeProvider; }
    soc::api::ISleeper &MockSoc::getSleeper() { return *m_sleeper; }
    soc::api::IPersistentStorage &MockSoc::getPersistentStorage() { return *m_persistentStorage; }

    std::unique_ptr<soc::api::IDigitalInput> MockSoc::createDigitalInput(
        int pin, bool pullup)
    {
      return std::make_unique<MockDigitalInput>(pin, pullup);
    }
    
    std::unique_ptr<soc::api::IRotaryEncoder> MockSoc::createRotaryEncoder(
        int pinA, int pinB, int stepsPerNotch)
    {
      return std::make_unique<MockRotaryEncoder>();
    }

    std::unique_ptr<soc::api::ITask> MockSoc::createTask(
        soc::api::TaskFunction_t taskFunction, const std::string &name,
        unsigned int stackSize, unsigned int priority)
    {
      printf("Mock 'creating' task: %s\n", name.c_str());

      // Create a new thread that executes the provided function.
      // Wrap it in a MockTask to manage the thread's lifecycle (RAII).
      // Return the unique_ptr, transferring ownership to the caller.
      return std::make_unique<MockTask>(std::thread(std::move(taskFunction)));
    }
  } // namespace mock
} // namespace soc
