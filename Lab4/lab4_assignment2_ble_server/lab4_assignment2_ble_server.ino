/*
 * Lab 4 - Assignment 2: BLE Server
 *
 * Description:
 *   This code implements a BLE Server that reads a button state from GPIO 2
 *   and notifies the button state to connected BLE clients.
 *   When the button is pressed, it toggles a state value and sends it to the client.
 *
 * Hardware:
 *   - ESP32 (Kepler ESP-A) board
 *   - I/O CUBE Push button connected to GPIO 2
 *
 * BLE Communication:
 *   - Service UUID: [INSERT YOUR SERVICE UUID HERE]
 *   - Characteristic UUID: [INSERT YOUR CHARACTERISTIC UUID HERE]
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ===== BLE Configuration =====
// TODO: Replace these UUIDs with your own generated UUIDs
#define SERVICE_UUID        "YOUR-SERVICE-UUID-HERE"
#define CHARACTERISTIC_UUID "YOUR-CHARACTERISTIC-UUID-HERE"
#define BLE_SERVER_NAME     "ESP32_BLE_Server"

// ===== Hardware Configuration =====
#define BUTTON_PIN 2  // GPIO 2 for I/O CUBE button

// ===== Global Variables =====
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// Button state variables
int buttonState = HIGH;           // Current button state
int lastButtonState = HIGH;       // Previous button state
uint8_t ledToggleState = 0;       // Toggle state to send to client (0 or 1)

// ===== BLE Server Callbacks =====
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Client connected");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Client disconnected");
    }
};

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Server...");

  // Initialize button pin
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Create the BLE Device
  BLEDevice::init(BLE_SERVER_NAME);

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService* pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic with NOTIFY property
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  // Add BLE2902 descriptor for notifications
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("BLE Server is now advertising");
  Serial.println("Waiting for a client connection...");
}

// ===== Main Loop =====
void loop() {
  // Read button state
  buttonState = digitalRead(BUTTON_PIN);

  // Detect button press (falling edge: HIGH -> LOW)
  if (buttonState == LOW && lastButtonState == HIGH) {
    // Button pressed - toggle the state
    ledToggleState = !ledToggleState;

    Serial.print("Button pressed! Toggle state: ");
    Serial.println(ledToggleState);

    // If device is connected, notify the client
    if (deviceConnected) {
      pCharacteristic->setValue(&ledToggleState, 1);
      pCharacteristic->notify();
      Serial.println("Notification sent to client");
    }

    delay(50);  // Simple debounce delay
  }

  // Update last button state
  lastButtonState = buttonState;

  // Handle disconnection - restart advertising
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);  // Give the bluetooth stack time to prepare
    pServer->startAdvertising();
    Serial.println("Restarting advertising...");
    oldDeviceConnected = deviceConnected;
  }

  // Handle new connection
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }

  delay(10);  // Small delay for stability
}
