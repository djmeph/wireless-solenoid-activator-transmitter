#include "painlessMesh.h"

#define   MESH_PREFIX     "network"
#define   MESH_PASSWORD   "password"
#define   MESH_PORT       5555

Scheduler     userScheduler;
DynamicJsonDocument poof(1024);
DynamicJsonDocument ping(1024);
StaticJsonDocument<1024> pongJSON;
String poofStr;
String pingStr;
painlessMesh  mesh;

int INPUT_PIN = 0;
int OUTPUT_PIN = 13;

uint8_t button_state;
int button_state_last = -1;
int debounce = 0;
const int debounce_time = 10;

void sendMessage() {
  poof["delay"] = 250;
  serializeJson(poof, poofStr);
  mesh.sendBroadcast(poofStr);
  Serial.printf("Sending message: %s\n", poofStr.c_str());
  poofStr = "";
}

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
  DeserializationError error = deserializeJson(pongJSON, msg);
  if (error) {
    return;
  }
  String cmd = pongJSON["msg"];
  if (cmd == "Pong!") {
    digitalWrite(OUTPUT_PIN, HIGH);
  }
}

void newConnectionCallback(uint32_t nodeId) {
  digitalWrite(OUTPUT_PIN, LOW);
  mesh.sendBroadcast(pingStr);
  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  digitalWrite(OUTPUT_PIN, LOW);
  mesh.sendBroadcast(pingStr);
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

void setup() {
  Serial.begin(115200);
  pinMode(INPUT_PIN, INPUT_PULLUP);
  pinMode(OUTPUT_PIN, OUTPUT);
  mesh.setDebugMsgTypes( ERROR | STARTUP );
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.setContainsRoot(true);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  poof["msg"] = "Poof!";
  ping["msg"] = "Ping!";
  serializeJson(ping, pingStr);
}

void loop() {
  userScheduler.execute();
  mesh.update();
  button();
}

void button() {
  button_state = digitalRead(INPUT_PIN);
  if (button_state != button_state_last && millis() - debounce > debounce_time) {
    button_state_last =  button_state;
    debounce = millis();
    if (button_state == LOW) {
      sendMessage();
    }
  }
}