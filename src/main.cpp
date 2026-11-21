#include <Arduino.h>
#include "app/CompositionRoot.h"

// Statically allocated Composition Root; lives for the entire system lifetime
// with zero dynamic heap allocation.
static aviator::app::CompositionRoot globalCompositionRoot;

void setup() {
    Serial.begin(115200);

    Serial.println("=================================================");
    Serial.println(" ESP32 Aviator Clock (Hexagonal Architecture)");
    Serial.printf(" System Booted: %s, %s\n", __DATE__, __TIME__);
    Serial.println("=================================================");

    globalCompositionRoot.setup();
    globalCompositionRoot.start();
}

void loop() {
    // Delete the Arduino loop task to release its stack memory;
    // all application logic executes within dedicated FreeRTOS tasks.
    vTaskDelete(NULL);
}
