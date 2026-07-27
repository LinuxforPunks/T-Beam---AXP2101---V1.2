#include <RadioLib.h>
#include <Wire.h>

// 1. PHYSICAL PIN MAPPINGS FOR T-BEAM V1.2 (SX1276)
#define LORA_MOSI   27
#define LORA_MISO   19
#define LORA_SCK    5
#define LORA_NSS    18
#define LORA_RST    23
#define LORA_DIO0   26
#define LORA_DIO1   33

// Instantiate the radio module directly using explicit pins
SX1276 radio = new Module(LORA_NSS, LORA_DIO0, LORA_RST, LORA_DIO1);

// 2. MANDATORY AXP2101 POWER REGISTER INITIALIZATION
// This bypasses LilyGo's missing files and directly forces the power channels open.
void wakeupAXP2101() {
    Wire.begin(21, 22); // Start I2C on the T-Beam V1.2 PMU pins (SDA=21, SCL=22)
    
    Wire.beginTransmission(0x34); // The standard I2C address for the AXP2101 chip
    Wire.write(0x92);             // ALDO2 Voltage control register
    Wire.write(0x1C);             // Set to 3.3V (Powers the SX1276 radio VCC line)
    Wire.endTransmission();

    Wire.beginTransmission(0x34);
    Wire.write(0x93);             // ALDO3 Voltage control register
    Wire.write(0x1C);             // Set to 3.3V (Powers the Onboard GPS/OLED rails)
    Wire.endTransmission();
    
    Wire.beginTransmission(0x34);
    Wire.write(0x10);             // Power output management register
    Wire.write(0x0C);             // Explicitly enable ALDO2 and ALDO3 channels
    Wire.endTransmission();
}

void setup() {
    Serial.begin(115200); // Connects directly to your Reticulum host daemon
    
    // Wake up the power rails first or radio.begin() will throw error -2 (No Hardware Found)
    wakeupAXP2101(); 
    delay(1500);

    // 3. EBYTE E90-DTU COMPATIBLE AIR DATA CONFIGURATION
    // Freq: 868.0 MHz, BW: 125.0 kHz, SF: 7, CR: 5 (4/5), Sync: 0x12, Pwr: 17 dBm, Preamble: 8
    int state = radio.begin(868.0, 125.0, 7, 5, 0x12, 17, 8);
    
    if (state != RADIOLIB_ERR_NONE) {
        while (true); // Halt execution if the chip fails to respond
    }
}

void loop() {
    // ---- PIPELINE 1: USB INPUT (Host App -> T-Beam USB -> Antenna Airwaves) ----
    if (Serial.available() > 0) {
        String outgoingPacket = Serial.readString();  
        radio.transmit(outgoingPacket); 
    }

    // ---- PIPELINE 2: AIR INPUT (Ebyte Antenna -> T-Beam Radio -> Host App USB) ----
    String incomingPacket; 
    int state = radio.receive(incomingPacket);   
    
    if (state == RADIOLIB_ERR_NONE) {
        // Pipes raw frames directly out of USB with zero structural header changes
        Serial.print(incomingPacket); 
    }
}
