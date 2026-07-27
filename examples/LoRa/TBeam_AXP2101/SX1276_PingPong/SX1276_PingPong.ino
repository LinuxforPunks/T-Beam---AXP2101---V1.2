/*
  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

#include <RadioLib.h>
#define XPOWERS_CHIP_AXP2101
#include <XPowersLib.h>

// uncomment the following only on one
// of the nodes to initiate the pings
// 注释INITIATING_NODE这行初始化接收,打开这行初始化发送,两个设备，必须一个初始化为接收另一个初始化为发送才能收到
// #define INITIATING_NODE

// Board pin definitions
#define GPS_RX_PIN                  34
#define GPS_TX_PIN                  12
#define BUTTON_PIN                  38
#define I2C_SDA                     21
#define I2C_SCL                     22
#define PMU_IRQ                     35
#define RADIO_SCLK_PIN               5
#define RADIO_MISO_PIN              19
#define RADIO_MOSI_PIN              27
#define RADIO_CS_PIN                18
#define RADIO_DIO0_PIN              26
#define RADIO_RST_PIN               23
#define RADIO_DIO1_PIN              33
#define RADIO_DIO2_PIN              32
#define BOARD_LED                   4
#define LED_ON                      LOW
#define BUTTON_PIN                  38
#define GPS_BAUD_RATE               9600
#define BOARD_VARIANT_NAME          "T-Beam-AXP2101-SX1276"

#define CONFIG_RADIO_FREQ           868.0
#define CONFIG_RADIO_OUTPUT_POWER   17


XPowersPMU pmu; // Power management unit
SX1276 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_DIO1_PIN);

// save transmission states between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate transmission or reception state
bool transmitFlag = false;

// flag to indicate that a packet was sent or received
volatile bool operationDone = false;

void setFlag(void)
{
    // we sent or received a packet, set the flag
    operationDone = true;
}

void setupPeripheralPowerSupplies()
{
    // Initialize AXP2101
    bool success =  pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, I2C_SDA, I2C_SCL);
    if (!success) {
        Serial.println(F("[AXP2101] Initialization failed!"));
        while (true) {
            delay(10);
        }
    }
    // WARNING: DC1 is the core power supply for the ESP32; do not configure it.

    // GNSS RTC PowerVDD 3300mV
    pmu.setButtonBatteryChargeVoltage(3300);
    pmu.enableButtonBatteryCharge();

    // LoRa VDD 3300mV
    pmu.setALDO2Voltage(3300);
    pmu.enableALDO2();

    //GNSS VDD 3300mV
    pmu.setALDO3Voltage(3300);
    pmu.enableALDO3();

    // Set charger constant current
    pmu.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_500MA);
}

void setup() {
    Serial.begin(115200);

    // SX1276 Parameters mapped to match your E90-DTU:
    // Frequency: 868.0 MHz, Bandwidth: 125.0 kHz, Spreading Factor: 7, 
    // Coding Rate: 5 (4/5), Sync Word: 0x12, Output Power: 17, Preamble: 8
    int state = radio.begin(868.0, 125.0, 7, 5, 0x12, 17, 8); 
    
    if (state == RADIOLIB_ERR_NONE) {
        // Success indicator (Optional: flash an onboard LED here if desired)
    } else {
        while (true); // Halt execution if initialization fails
    }
}

void loop() {
    // 1. LISTEN TO LOCAL PC/IPHONE (USB Serial -> LoRa Airwaves)
    if (Serial.available() > 0) {
        String outgoingPacket = Serial.readString();  
        radio.transmit(outgoingPacket); 
    }

    // 2. LISTEN TO AIRWAVES (LoRa Antenna -> USB Serial)
    String incomingPacket; // Define the buffer container locally
    
    // Explicitly command the physical chip to scan the airwaves for packets
    int state = radio.receive(incomingPacket);   
    
    if (state == RADIOLIB_ERR_NONE) {
        // If a valid packet arrives from the Ebyte, dump raw text out of USB
        Serial.print(incomingPacket); 
    }
}
