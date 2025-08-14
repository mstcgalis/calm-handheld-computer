#include <WiFi.h>
#include <esp_now.h>
#include <Encoder.h>

// ===================== CONFIG =====================
constexpr int encoderPinA = 0;
constexpr int encoderPinB = 1;
constexpr int buttonPin   = 2;
constexpr int ledPin      = LED_BUILTIN;
constexpr int vibrationPin = D8;

constexpr unsigned long idleTimeout = 1500;
constexpr unsigned long debounceDelay = 30;
constexpr unsigned long confirmBlinkDuration = 100;

constexpr uint8_t broadcastAddress[] = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

// ===================== FLAGS =====================
#define FLAG_ROTATION_DONE 0x01

// ===================== ESP-NOW MESSAGE STRUCT =====================
typedef struct __attribute__((packed)) {
  uint16_t duration_ms;
  uint16_t sender_id;
  uint8_t flags;
} struct_message;

struct_message outgoingMsg;

// ===================== STATE =====================
Encoder myEnc(encoderPinA, encoderPinB);

long lastEncoderPos = 0;
unsigned long movementStartTime = 0;
unsigned long lastMovementTime = 0;
bool interactionActive = false;

bool feedbackActive = false;
unsigned long feedbackEndTime = 0;

bool cancelBlinkActive = false;
unsigned long cancelBlinkEndTime = 0;

bool buttonPrevState = HIGH;
bool buttonStableState = HIGH;
unsigned long lastDebounceTime = 0;

// ===================== CALLBACKS =====================
void OnDataSent(const uint8_t *, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Send: Success" : "Send: Fail");
}

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len < sizeof(struct_message)) return;

  struct_message incoming;
  memcpy(&incoming, data, sizeof(incoming));

  Serial.printf("From 0x%04X | Duration: %u ms | Flags: 0x%02X\n",
                incoming.sender_id,
                incoming.duration_ms,
                incoming.flags);

  digitalWrite(ledPin, LOW);
  digitalWrite(vibrationPin, HIGH);
  feedbackActive = true;
  feedbackEndTime = millis() + incoming.duration_ms;
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  pinMode(vibrationPin, OUTPUT);
  digitalWrite(vibrationPin, LOW);

  pinMode(buttonPin, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (true);
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    while (true);
  }

  myEnc.write(0);
}

// ===================== LOOP =====================
void loop() {
  unsigned long now = millis();

  if (!cancelBlinkActive && feedbackActive && now >= feedbackEndTime) {
    digitalWrite(ledPin, HIGH);
    digitalWrite(vibrationPin, LOW);
    feedbackActive = false;
  }

  if (cancelBlinkActive && now >= cancelBlinkEndTime) {
    digitalWrite(ledPin, HIGH);
    digitalWrite(vibrationPin, LOW);
    cancelBlinkActive = false;
  }

  bool rawButtonState = digitalRead(buttonPin);
  if (rawButtonState != buttonPrevState) {
    lastDebounceTime = now;
  }
  buttonPrevState = rawButtonState;

  if ((now - lastDebounceTime) > debounceDelay) {
    if (rawButtonState != buttonStableState) {
      buttonStableState = rawButtonState;

      if (buttonStableState == LOW) {
        digitalWrite(ledPin, LOW);
        digitalWrite(vibrationPin, HIGH);
        cancelBlinkActive = true;
        cancelBlinkEndTime = now + confirmBlinkDuration;
        feedbackActive = false;
        Serial.println("🔴 Button pressed: Cancel feedback + blink");
      }
    }
  }

  long pos = myEnc.read() / 4;
  if (pos != lastEncoderPos) {
    lastEncoderPos = pos;
    if (!interactionActive) {
      movementStartTime = now;
      interactionActive = true;
    }
    lastMovementTime = now;
    Serial.printf("Rotating... (%ld)\n", pos);
  }

  if (interactionActive && (now - lastMovementTime >= idleTimeout)) {
    interactionActive = false;
    uint16_t duration = (uint16_t)(lastMovementTime - movementStartTime);
    uint64_t mac = ESP.getEfuseMac();

    outgoingMsg.duration_ms = duration;
    outgoingMsg.sender_id = (uint16_t)(mac & 0xFFFF);
    outgoingMsg.flags = FLAG_ROTATION_DONE;

    esp_now_send(broadcastAddress, (uint8_t *)&outgoingMsg, sizeof(outgoingMsg));
    Serial.printf("📤 Sent: %u ms from 0x%04X\n", duration, outgoingMsg.sender_id);

    myEnc.write(0);
    lastEncoderPos = 0;
  }
}
