   #include "soc/esp32/ESP32RotaryEncoder.h"

   namespace soc {
   namespace esp32 {

   ESP32RotaryEncoder::ESP32RotaryEncoder(int pinA, int pinB, int stepsPerNotch)
       : _stepsPerNotch(stepsPerNotch)
   {
       ESP32Encoder::useInternalWeakPullResistors = UP;
       _encoder.attachHalfQuad(pinA, pinB);
       _encoder.setCount(0);
   }

   ESP32RotaryEncoder::~ESP32RotaryEncoder() {
       // ESP32Encoder doesn't have a detach/destructor cleanup we can call easily,
       // but typically lives for app lifetime.
   }

   void ESP32RotaryEncoder::update() {
       // ESP32Encoder is interrupt driven
   }

   int32_t ESP32RotaryEncoder::getDelta() {
       int64_t currentPos = _encoder.getCount();
       int64_t deltaRaw = currentPos - _lastPosition;
       
       // Optimization: only process if there is a change
       if (deltaRaw == 0) return 0;

       // If stepsPerNotch is > 1, we might want to return 0 until a full notch is reached
       // But often better to return raw/steps in getDelta and let logic handle it.
       // However, typical encoders do 4 pulses per detent.
       // Let's just return the raw change divided by stepsPerNotch if requested?
       // No, getDelta returns "steps". If stepsPerNotch is 4, we divide by 4.
       
       if (_stepsPerNotch > 1) {
            int32_t notches = deltaRaw / _stepsPerNotch;
            if (notches != 0) {
                 _lastPosition += (notches * _stepsPerNotch);
                 return notches;
            }
            return 0; // Not enough movement for a full notch
       } else {
           _lastPosition = currentPos;
           return (int32_t)deltaRaw;
       }
   }

   void ESP32RotaryEncoder::reset() {
       _encoder.setCount(0);
       _lastPosition = 0;
   }

   } // namespace esp32
   } // namespace soc
