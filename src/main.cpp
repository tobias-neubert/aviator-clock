#include <Arduino.h>
#include <memory>
#include "app/CompositionRoot.h"

// Retained globally to survive setup() stack frame lifecycle
std::unique_ptr<aviator::app::CompositionRoot> globalCompositionRoot;

void setup() {
    Serial.begin(115200);

    Serial.println("=================================================");
    Serial.println(" ESP32 Aviator Clock (Hexagonal Architecture)");
    Serial.printf(" System Booted: %s, %s\n", __DATE__, __TIME__);
    Serial.println("=================================================");

    globalCompositionRoot = std::make_unique<aviator::app::CompositionRoot>();
    globalCompositionRoot->setup();
    globalCompositionRoot->start();
}

void loop() {
    // Delete the Arduino loop task to release its stack memory;
    // all application logic executes within dedicated FreeRTOS tasks.
    vTaskDelete(NULL);
}
