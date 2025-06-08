#include <WiFi.h>
#include <esp_now.h>

// LED pin (active-low)
const int ledPin = LED_BUILTIN;

// Broadcast MAC address (multicast)
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Message structure
typedef struct struct_message {
  char message[64];
} struct_message;

struct_message outgoingMsg;

// Blink control
bool shouldBlink = false;
unsigned long blinkStartTime = 0;
const unsigned long blinkDuration = 1000; // ms

// Helper: Print MAC address cleanly
void printMac(const uint8_t *mac) {
  for (int i = 0; i < 6; ++i) {
    Serial.printf("%02X", mac[i]);
    if (i < 5) Serial.print(":");
  }
}

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// Callback when data is received
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  struct_message incomingMsg;
  memcpy(&incomingMsg, incomingData, min((size_t)len, sizeof(incomingMsg)));

  Serial.print("Received message from ");
  printMac(recv_info->src_addr);
  Serial.print(": ");
  Serial.println(incomingMsg.message);

  // Start LED blink (active-low)
  digitalWrite(ledPin, LOW);
  shouldBlink = true;
  blinkStartTime = millis();
}

void setup() {
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH); // Start with LED OFF

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  // Turn LED off after blink duration
  if (shouldBlink && (millis() - blinkStartTime >= blinkDuration)) {
    digitalWrite(ledPin, HIGH); // LED OFF
    shouldBlink = false;
  }

  // Send message every 5 seconds
  static unsigned long lastSend = 0;
  if (millis() - lastSend >= 5000) {
    lastSend = millis();

    uint64_t mac = ESP.getEfuseMac();
    snprintf(outgoingMsg.message, sizeof(outgoingMsg.message),
             "Hello from device %02X:%02X:%02X:%02X:%02X:%02X",
             (uint8_t)(mac >> 40),
             (uint8_t)(mac >> 32),
             (uint8_t)(mac >> 24),
             (uint8_t)(mac >> 16),
             (uint8_t)(mac >> 8),
             (uint8_t)(mac));

    esp_err_t result = esp_now_send(broadcastAddress, (const uint8_t *)&outgoingMsg, sizeof(outgoingMsg));
    if (result != ESP_OK) {
      Serial.println("Error sending data");
    }
  }
}
