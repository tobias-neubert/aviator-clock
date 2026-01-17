#pragma once

#include <soc/api/ITask.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace soc
{
  namespace esp32
  {
    /**
     * @brief An RAII wrapper for a FreeRTOS task.
     *
     * This class implements the ITask interface for the ESP32. It holds the
     * FreeRTOS task handle. When an instance of this class is destroyed,
     * it automatically deletes the underlying FreeRTOS task.
     */
    class ESP32Task : public soc::api::ITask
    {
    public:
      /**
       * @brief Construct a new ESP32Task object.
       * @param handle The FreeRTOS handle of the task to manage.
       */
      explicit ESP32Task(TaskHandle_t handle) : m_handle(handle) {}

      /**
       * @brief Destroys the ESP32Task object and the underlying FreeRTOS task.
       */
      ~ESP32Task() override
      {
        if (m_handle != nullptr)
        {
          vTaskDelete(m_handle);
        }
      }

      // Prevent copying and moving to ensure unique ownership of the task handle.
      ESP32Task(const ESP32Task &) = delete;
      ESP32Task &operator=(const ESP32Task &) = delete;
      ESP32Task(ESP32Task &&) = delete;
      ESP32Task &operator=(ESP32Task &&) = delete;

    private:
      TaskHandle_t m_handle;
    };
  } // namespace esp32
} // namespace soc
