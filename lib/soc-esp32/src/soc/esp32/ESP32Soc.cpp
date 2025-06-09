#include <Arduino.h>
#include <soc/esp32/ESP32Soc.h>

namespace soc
{
    namespace esp32
    {
        ESP32Soc::ESP32Soc()
        {
        }

        ESP32Soc::~ESP32Soc()
        {
            for (auto it = components.rbegin(); it != components.rend(); ++it)
            {
                if (*it)
                {
                    (*it)->teardown();
                }
            }
            components.clear();
        }

        void ESP32Soc::processInput()
        {
        }

        void ESP32Soc::advanceState(unsigned long currentTimeMs)
        {
            for (auto &component : components)
            {
                if (component)
                {
                    component->advanceState(currentTimeMs);
                }
            }
        }

        void ESP32Soc::render()
        {
            for (auto &component : components)
            {
                if (component)
                {
                    component->render();
                }
            }
        }

        void ESP32Soc::addComponent(std::shared_ptr<soc::api::ISocComponent> component)
        {
            if (component)
            {
                component->setup();
                components.push_back(component);
            }
            else
            {
                //Serial.println("ESP32Soc: ERROR - Attempted to add a null component pointer!");
            }
        }

    } // namespace esp32
} // namespace soc