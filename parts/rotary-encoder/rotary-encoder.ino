#include <WiFi.h>
#include <esp_now.h>
#include <Encoder.h>

// ===================== CONFIG =====================
constexpr int encoderPinA = 0;
constexpr int encoderPinB = 1;
constexpr int buttonPin   = 2;
constexpr int ledPin      = LED_BUILTIN;

constexpr unsigned long idleTimeout = 1500;
constexpr unsigned long debounceDelay = 30;
constexpr unsigned long confirmBlinkDuration = 100;

constexpr uint8_t broadcastAddress[] = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

// ===================== FLAGS =====================
#define FLAG_ROTATION_DONE 0x01

// ===================== MESSAGE STRUCT =====================
typedef struct __attribute__((packed)) {
  uint16_t duration_ms;
  uint16_t sender_id;
  uint8_t flags;
} struct_message;

struct_message outgoingMsg;

// ===================== STATE =====================
Encoder myEnc(encoderPinA, encoderPinB);

// Encoder tracking
long lastEncoderPos = 0;
unsigned long movementStartTime = 0;
unsigned long lastMovementTime = 0;
bool interactionActive = false;

// LED control
bool ledOn = false;
unsigned long ledEndTime = 0;

// Confirmation blink
bool cancelBlinkActive = false;
unsigned long cancelBlinkEndTime = 0;

// Button debounce
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

  // Set LED ON for exact duration
  digitalWrite(ledPin, LOW);  // LED ON (active-low)
  ledOn = true;
  ledEndTime = millis() + incoming.duration_ms;

  if (incoming.flags & FLAG_ROTATION_DONE) {
    Serial.println("Flag: Rotation complete");
  }
}

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH); // LED OFF (active-low)

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

  // LED off after normal feedback
  if (!cancelBlinkActive && ledOn && now >= ledEndTime) {
    digitalWrite(ledPin, HIGH); // LED OFF
    ledOn = false;
  }

  // End of confirmation blink
  if (cancelBlinkActive && now >= cancelBlinkEndTime) {
    digitalWrite(ledPin, HIGH); // LED OFF
    cancelBlinkActive = false;
  }

  // ----- NON-BLOCKING BUTTON DEBOUNCE -----
  bool rawButtonState = digitalRead(buttonPin);

  if (rawButtonState != buttonPrevState) {
    lastDebounceTime = now;
  }
  buttonPrevState = rawButtonState;

  if ((now - lastDebounceTime) > debounceDelay) {
    if (rawButtonState != buttonStableState) {
      buttonStableState = rawButtonState;

      if (buttonStableState == LOW) {
        digitalWrite(ledPin, LOW); // Start blink (LED ON)
        cancelBlinkActive = true;
        cancelBlinkEndTime = now + confirmBlinkDuration;

        ledOn = false; // Cancel ongoing feedback
        Serial.println("🔴 Button pressed: Feedback canceled, blinking...");
      }
    }
  }

  // ----- ENCODER MOVEMENT DETECTION -----
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

  // ----- INACTIVITY DETECTED → SEND MESSAGE -----
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
