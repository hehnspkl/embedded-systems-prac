/*
 * Lab 4 - Assignment 2: BLE Client
 *
 * Description:
 *   This code implements a BLE Client that connects to a BLE Server,
 *   receives button state notifications, and controls an LED on GPIO 15
 *   based on the received toggle state.
 *
 * Hardware:
 *   - ESP32 (Kepler ESP-A) board
 *   - Built-in LED on GPIO 15
 *
 * BLE Communication:
 *   - Service UUID: [INSERT YOUR SERVICE UUID HERE]
 *   - Characteristic UUID: [INSERT YOUR CHARACTERISTIC UUID HERE]
 */

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// ===== BLE Configuration =====
// TODO: Replace these UUIDs with your own generated UUIDs (must match server)
#define SERVICE_UUID        "YOUR-SERVICE-UUID-HERE"
#define CHARACTERISTIC_UUID "YOUR-CHARACTERISTIC-UUID-HERE"
#define BLE_SERVER_NAME     "ESP32_BLE_Server"

// ===== Hardware Configuration =====
#define LED_PIN 15  // GPIO 15 for LED

// ===== Global Variables =====
static BLEAdvertisedDevice* myDevice;
BLEClient* pClient = NULL;
BLERemoteCharacteristic* pRemoteCharacteristic = NULL;

bool doConnect = false;
bool connected = false;
bool doScan = false;

// ===== BLE Notify Callback =====
// This function is called when a notification is received from the server
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {

  Serial.print("Notify callback - Received value: ");

  // Get the toggle state value
  uint8_t toggleState = pData[0];
  Serial.println(toggleState);

  // Control LED based on received toggle state
  if (toggleState == 1) {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("LED ON");
  } else {
    digitalWrite(LED_PIN, LOW);
    Serial.println("LED OFF");
  }
}

// ===== BLE Client Callback =====
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("Connected to server");
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("Disconnected from server");
    Serial.println("Will attempt to reconnect...");
  }
};

// ===== Connect to BLE Server =====
bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  // Create client
  if (pClient == NULL) {
    pClient = BLEDevice::createClient();
    Serial.println("Created new BLE client");
    pClient->setClientCallbacks(new MyClientCallback());
  }

  // Connect to the remote BLE Server
  if (!pClient->connect(myDevice)) {
    Serial.println("Failed to connect to server");
    return false;
  }
  Serial.println("Connected to server");

  // Obtain a reference to the service
  BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find service UUID: ");
    Serial.println(SERVICE_UUID);
    pClient->disconnect();
    return false;
  }
  Serial.println("Found service");

  // Obtain a reference to the characteristic
  pRemoteCharacteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find characteristic UUID: ");
    Serial.println(CHARACTERISTIC_UUID);
    pClient->disconnect();
    return false;
  }
  Serial.println("Found characteristic");

  // Register for notifications
  if (pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
    Serial.println("Registered for notifications");
  } else {
    Serial.println("Characteristic does not support notifications");
    pClient->disconnect();
    return false;
  }

  connected = true;
  return true;
}

// ===== BLE Advertised Device Callback =====
// This callback is called during BLE scan when a device is found
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // Check if the device name matches our server
    if (advertisedDevice.getName() == BLE_SERVER_NAME) {
      Serial.println("Found our server!");

      // Stop scanning
      BLEDevice::getScan()->stop();

      // Save the device reference
      myDevice = new BLEAdvertisedDevice(advertisedDevice);

      // Set flag to connect
      doConnect = true;
      doScan = false;
    }
  }
};

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Client...");

  // Initialize LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // Start with LED off

  // Initialize BLE
  BLEDevice::init("");

  // Start BLE scan
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);  // Scan for 5 seconds

  Serial.println("Scanning for BLE Server...");
}

// ===== Main Loop =====
void loop() {
  // If connection flag is set, try to connect
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("Successfully connected to BLE Server");
    } else {
      Serial.println("Failed to connect to server, will retry...");
      delay(1000);
      doScan = true;  // Restart scanning
    }
    doConnect = false;
  }

  // If connected, the notify callback will handle LED control
  if (connected) {
    // Connection is active, notifications are being received
    // LED control happens in notifyCallback
  }
  // If disconnected, restart scanning
  else if (!connected && !doScan) {
    doScan = true;
  }

  // Start new scan if needed
  if (doScan) {
    Serial.println("Restarting BLE scan...");
    BLEDevice::getScan()->start(0);  // Scan continuously until server is found
    doScan = false;
  }

  delay(1000);  // Loop delay
}
