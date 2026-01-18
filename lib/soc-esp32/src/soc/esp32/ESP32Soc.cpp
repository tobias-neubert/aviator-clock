#include <soc/esp32/ESP32Soc.h>
#include <soc/esp32/ESP32Task.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <memory>
#include "esp_log.h"

static const char* TAG = "ESP32Soc";

namespace soc
{
  namespace esp32
  {
    namespace
    {
      /**
       * @brief A static C-style function that serves as the entry point for FreeRTOS
       * tasks.
       *
       * This wrapper function allows using `std::function` (and thus lambdas with
       * captures) with the C-style FreeRTOS API.
       *
       * @param pvParameters A pointer to a heap-allocated `soc::api::TaskFunction_t`
       * object.
       */
      void taskRunner(void *pvParameters)
      {
        // Cast the void pointer back to the std::function object.
        auto *taskFunction = static_cast<soc::api::TaskFunction_t *>(pvParameters);

        if (taskFunction)
        {
          // Execute the actual task function (the lambda).
          (*taskFunction)();

          // The task function has completed, so we can clean up the heap-allocated
          // function object.
          delete taskFunction;
        }

        // Let the task delete itself from the FreeRTOS scheduler.
        vTaskDelete(NULL);
      }
    } // namespace

    ESP32Soc::ESP32Soc()
    {
      // Create the service instances in the constructor.
      // Initialization that can fail or takes time is done in init().
      m_timeProvider = std::make_unique<NtpTime>();
      m_sleeper = std::make_unique<ESP32Sleeper>();
    }

    void ESP32Soc::init(const char *wifiSsid, const char *wifiPassword)
    {
      // Initialize the time provider, which connects to WiFi and starts NTP.
      // This is a blocking operation and should be called from setup().
      if (m_timeProvider)
      {
        m_timeProvider->init(wifiSsid, wifiPassword);
      }
    }

    soc::api::ITime &ESP32Soc::getTimeProvider()
    {
      // The time provider is guaranteed to exist by the constructor.
      return *m_timeProvider;
    }

    soc::api::ISleeper &ESP32Soc::getSleeper()
    {
      // The sleeper is guaranteed to exist by the constructor.
      return *m_sleeper;
    }

    std::unique_ptr<soc::api::IDigitalInput> ESP32Soc::createDigitalInput(
        int pin, bool pullup)
    {
      // The factory method creates, initializes, and returns a new instance.
      auto input = std::make_unique<ESP32DigitalInput>(pin, pullup);
      input->begin(); // Initialize the pin mode.
      return input;
    }

    std::unique_ptr<soc::api::ITask> ESP32Soc::createTask(
        soc::api::TaskFunction_t taskFunction, const std::string &name,
        unsigned int stackSize, unsigned int priority)
    {
      // The C-style FreeRTOS API requires a raw pointer. To pass our std::function
      // (which might have captures), we must allocate it on the heap.
      // The `taskRunner` function is responsible for deleting this object later.
      auto *functionPtr = new soc::api::TaskFunction_t(std::move(taskFunction));

      TaskHandle_t handle = nullptr;
      BaseType_t result =
          xTaskCreate(taskRunner,   // The C-style entry point function.
                      name.c_str(), // A descriptive name for the task.
                      stackSize,    // The allocated stack size.
                      functionPtr,  // The parameter to pass to the task (our lambda).
                      priority,     // The task priority.
                      &handle);     // The handle to the created task.

      if (result != pdPASS)
      {
        ESP_LOGE(TAG, "Failed to create task '%s'", name.c_str());
        delete functionPtr; // Clean up the function object if task creation fails.
        return nullptr;
      }

      ESP_LOGI(TAG, "Task '%s' created successfully.", name.c_str());

      // Return an RAII wrapper that will manage the lifetime of the task.
      return std::make_unique<ESP32Task>(handle);
    }
  } // namespace esp32
} // namespace soc
