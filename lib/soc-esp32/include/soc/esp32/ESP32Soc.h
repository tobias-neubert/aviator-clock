#pragma once

#include <memory>
#include <vector>
#include <soc/api/ISoc.h>
#include <soc/api/ISocComponent.h>

namespace soc
{
    namespace esp32
    {
        class ESP32Soc : public soc::api::ISoc
        {
        public:
            ESP32Soc();

            virtual ~ESP32Soc() override;

            void processInput() override;

            void advanceState(unsigned long currentTimeMs) override;

            void render() override;

            void addComponent(std::shared_ptr<soc::api::ISocComponent> component) override;

        private:
            std::vector<std::shared_ptr<soc::api::ISocComponent>> components;
        };
    } // namespace esp32
} // namespace soc