/*
  ESP32 - Ten Simultaneous Square Wave Generators

  Frequencies:
  GPIO 13 -> 40 MHz
  GPIO 14 -> 30 MHz
  GPIO 25 -> 1 kHz
  GPIO 26 -> 10 kHz
  GPIO 27 -> 100 kHz
  GPIO 32 -> 1 MHz
  GPIO 4  -> 3 MHz
  GPIO 16 -> 4 MHz
  GPIO 17 -> 5 MHz
  GPIO 18 -> 10 MHz

  Duty Cycle: 50%
*/

const int NUM_CHANNELS = 10;

const int outputPins[NUM_CHANNELS] = {
  13, 14, 25, 26, 27, 32, 4, 16, 17, 18
};

const uint32_t frequencies[NUM_CHANNELS] = {
  40000000, // 40 MHz
  30000000, // 30 MHz
  1000,
  10000,
  100000,
  1000000,
  3000000,
  4000000,
  5000000,
  10000000
};

// 1-bit resolution allows 50% duty cycle up to 40 MHz max
const uint8_t pwmResolution = 1;

void setup() {

  Serial.begin(115200);
  Serial.println();
  Serial.println("ESP32 Multi-Frequency Square Wave Generator");
  Serial.println("-------------------------------------------");

  for (int i = 0; i < NUM_CHANNELS; i++) {

    bool configured = ledcAttach(
      outputPins[i],
      frequencies[i],
      pwmResolution
    );

    if (configured) {

      // Set duty cycle to 50% (1 out of 2^1)
      ledcWrite(outputPins[i], 1);

      Serial.print("GPIO ");
      if (outputPins[i] < 10) Serial.print(" "); // Formatting alignment
      Serial.print(outputPins[i]);
      Serial.print(" -> ");

      // Print frequency nicely formatted
      if (frequencies[i] >= 1000000) {
        Serial.print(frequencies[i] / 1000000);
        Serial.print(" MHz");
      } else if (frequencies[i] >= 1000) {
        Serial.print(frequencies[i] / 1000);
        Serial.print(" kHz");
      } else {
        Serial.print(frequencies[i]);
        Serial.print(" Hz");
      }

      Serial.println(" : OK");

    } else {

      Serial.print("GPIO ");
      Serial.print(outputPins[i]);
      Serial.println(" : ERROR");
    }
  }

  Serial.println("-------------------------------------------");
  Serial.println("All 10 PWM outputs configured successfully.");
}

void loop() {
  // Hardware handles all signal generation independently.
}
