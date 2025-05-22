#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
const char* ssid = "Rod";
const char* password = "12345678";
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);  // IP address for AP mode
DNSServer dnsServer;
WebServer server(80);
const int IN1 = 14;
const int IN2 = 27;
const int IN3 = 26;
const int IN4 = 25;
const int ENA = 33;
const int ENB = 32;

int motorSpeed = 200;  // Default speed
bool powerOn = true;   // Power state
bool autoMode = false;

const int batteryPin = 36;  // ADC pin for battery monitoring
const int IR_FRONT_PIN = 35;
const int IR_BACK_PIN = 18;
const int TRIG_PIN = 13;
const int ECHO_PIN = 12;    // Ultrasonic echo pin
WebSocketsServer webSocket = WebSocketsServer(81);

// Add these constants near the top with other global variables
const int NORMAL_SPEED = 150;    // Slower speed for manual mode
const int BUMP_SPEED = 255;      // Maximum speed for bumping
const int BUMP_DISTANCE = 5;      // Distance threshold for bump behavior (cm) - reduced from 15cm to 5cm

// Modify these constants for aggressive pushing behavior
const int DETECT_DISTANCE = 10;   // Distance to detect opponent (cm) - increased from 15cm to 40cm
const int ATTACK_DISTANCE = 10;    // Distance for full-power push (cm) - reduced from 15cm to 5cm
const int SEARCH_SPEED = 70;      // Lower speed while searching
const int ATTACK_SPEED = 255;     // Maximum speed for pushing

// Add these constants for IR sensor logic
const bool BLACK = 1;  // Change to 1 if sensor reads 1 for black
const bool WHITE = 0;  // Change to 0 if sensor reads 0 for white

void handleBattery();  // Function prototype

// Modify the togglePower function
void togglePower() {
  powerOn = !powerOn;
  stopMotors();  // Ensure motors are stopped
  autoMode = false;  // Reset to manual mode when power is toggled
  
  if (powerOn) {
    Serial.println("Power ON - Motors stopped");
  } else {
    Serial.println("Power OFF");
  }
}

// Update the webSocketEvent handler
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      stopMotors();
      break;

    case WStype_CONNECTED:
      Serial.printf("[%u] Connected!\n", num);
      break;

    case WStype_TEXT: {
      String command = String((char*)payload);
      Serial.print("Received command: ");
      Serial.println(command);

      // Only process commands if power is on and in manual mode
      if (!powerOn) {
        Serial.println("Power is OFF - ignoring command");
        return;
      }

      if (autoMode) {
        Serial.println("In AUTO mode - ignoring manual command");
        return;
      }

      // Process commands
      if (command == "forward") {
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, LOW);
        analogWrite(ENA, motorSpeed);
        analogWrite(ENB, motorSpeed);
        Serial.println("Moving Forward");
      }
      else if (command == "backward") {
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, HIGH);
        analogWrite(ENA, motorSpeed);
        analogWrite(ENB, motorSpeed);
        Serial.println("Moving Backward");
      }
      else if (command == "left") {
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);
        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, LOW);
        analogWrite(ENA, motorSpeed);
        analogWrite(ENB, motorSpeed);
        Serial.println("Turning Left");
      }
      else if (command == "right") {
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, HIGH);
        analogWrite(ENA, motorSpeed);
        analogWrite(ENB, motorSpeed);
        Serial.println("Turning Right");
      }
      else if (command == "stop") {
        stopMotors();
        Serial.println("Stopping");
      }
      break;
    }
  }
}

// Add speed control handler
void handleSpeed() {
  if (server.hasArg("value")) {
    motorSpeed = server.arg("value").toInt();
    motorSpeed = constrain(motorSpeed, 0, 255);
    server.send(200, "text/plain", "Speed set to: " + String(motorSpeed));
    Serial.println("Speed set to: " + String(motorSpeed));
  } else {
    server.send(400, "text/plain", "Missing value parameter");
  }
}

// Update setupSensors function
void setupSensors() {
  pinMode(IR_FRONT_PIN, INPUT);
  pinMode(IR_BACK_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Remove pullup resistors as they might interfere with the sensor
  digitalWrite(IR_FRONT_PIN, LOW);
  digitalWrite(IR_BACK_PIN, LOW);
}

// Add a debug function for IR sensors
void debugIRSensors() {
  int irFront = digitalRead(IR_FRONT_PIN);
  int irBack = digitalRead(IR_BACK_PIN);
  
  Serial.println("----------------------------------------");
  Serial.print("Front IR (Pin ");
  Serial.print(IR_FRONT_PIN);
  Serial.print("): ");
  Serial.print(irFront);
  Serial.println(irFront == 1 ? " BLACK" : " WHITE");
  
  Serial.print("Back IR (Pin ");
  Serial.print(IR_BACK_PIN);
  Serial.print("): ");
  Serial.print(irBack);
  Serial.println(irBack == 1 ? " BLACK" : " WHITE");
  Serial.println("----------------------------------------");
}

// Add a debug function specifically for the back IR sensor
void debugBackIRSensor() {
  int irBack = digitalRead(IR_BACK_PIN);
  
  Serial.println("----------------------------------------");
  Serial.print("Back IR (Pin ");
  Serial.print(IR_BACK_PIN);
  Serial.print("): Raw Value = ");
  Serial.print(irBack);
  Serial.println(" (Test over black surface)");
  delay(1000);
  Serial.println("----------------------------------------");
}

// Update the testIRSensors function
void testIRSensors() {
  int samples = 10;  // Take multiple readings for stability
  
  for(int i = 0; i < samples; i++) {
    int irFront = digitalRead(IR_FRONT_PIN);
    int irBack = digitalRead(IR_BACK_PIN);
    
    Serial.println("\n----- IR Sensor Test #" + String(i+1) + " -----");
    Serial.print("Front IR (Pin ");
    Serial.print("IR_FRONT_PIN30");
    Serial.print("): Raw Value = ");
    Serial.print(irFront);
    Serial.println(irFront == BLACK ? " (BLACK)" : " (WHITE)");
    
    Serial.print("Back IR (Pin ");
    Serial.print(IR_BACK_PIN);
    Serial.print("): Raw Value = ");
    Serial.print(irBack);
    Serial.println(irBack == BLACK ? " (BLACK)" : " (WHITE)");
    
    if (irFront == 0 && irBack == 0) {
      Serial.println("STOPPED");
      stopMotors();  // Changed from stop() to stopMotors()
    } else if (irFront == 0 && irBack == 1) {
      Serial.println("BACKWARD");
      moveBackward();
    } else if (irFront == 1 && irBack == 0) {
      Serial.println("FORWARD");
      moveForward();
    } else if (irFront == 1 && irBack == 1) {
      Serial.println("STOPPED");
      stopMotors();  // Changed from stop() to stopMotors()
    }
    delay(100);  // Short delay between readings
  }
}

void setupMotors() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
}

// Movement functions
void moveForward() {
  if (!powerOn) return;
  
  // Check if both IR sensors detect black
  int irFront = digitalRead(IR_FRONT_PIN);
  int irBack = digitalRead(IR_BACK_PIN);
  if (irFront != BLACK || irBack != BLACK) {
    Serial.println("Safety check: Cannot move forward - IR sensor(s) detect WHITE");
    stopMotors();
    return;
  }
  
  // Read distance and adjust speed
  long distance = readUltrasonic();
  int currentSpeed = NORMAL_SPEED;  // Default to slow speed
  
  // If object detected within bump distance, increase speed
  if (distance > 0 && distance < BUMP_DISTANCE) {
    currentSpeed = BUMP_SPEED;
    Serial.println("Object detected - Charging at high speed!");
  }

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, currentSpeed);
  analogWrite(ENB, currentSpeed);
}

void moveBackward() {
  if (!powerOn) return;  // Only check power
  
  // Check if both IR sensors detect black
  int irFront = digitalRead(IR_FRONT_PIN);
  int irBack = digitalRead(IR_BACK_PIN);
  if (irFront != BLACK || irBack != BLACK) {
    Serial.println("Safety check: Cannot move backward - IR sensor(s) detect WHITE");
    stopMotors();
    return;
  }
  
  digitalWrite(IN1, LOW);   // Left motor backward
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);   // Right motor backward
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);
}

void turnLeft() {
  if (!powerOn) return;  // Only check power
  
  // Check if both IR sensors detect black
  int irFront = digitalRead(IR_FRONT_PIN);
  int irBack = digitalRead(IR_BACK_PIN);
  if (irFront != BLACK || irBack != BLACK) {
    Serial.println("Safety check: Cannot turn left - IR sensor(s) detect WHITE");
    stopMotors();
    return;
  }
  
  digitalWrite(IN1, LOW);   // Left motor backward
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);  // Right motor forward
  digitalWrite(IN4, LOW);
  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);
}

void turnRight() {
  if (!powerOn) return;  // Only check power
  
  // Check if both IR sensors detect black
  int irFront = digitalRead(IR_FRONT_PIN);
  int irBack = digitalRead(IR_BACK_PIN);
  if (irFront != BLACK || irBack != BLACK) {
    Serial.println("Safety check: Cannot turn right - IR sensor(s) detect WHITE");
    stopMotors();
    return;
  }
  
  digitalWrite(IN1, HIGH);  // Left motor forward
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);   // Right motor backward
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

// Add these new functions after the existing movement functions
void moveForwardWithSpeed(int speed) {
  if (!powerOn) return;
  
  // Check if both IR sensors detect black
  int irFront = digitalRead(IR_FRONT_PIN);
  int irBack = digitalRead(IR_BACK_PIN);
  if (irFront != BLACK || irBack != BLACK) {
    Serial.println("Safety check: Cannot move forward - IR sensor(s) detect WHITE");
    stopMotors();
    return;
  }
  
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}

void turnRightWithSpeed(int speed) {
  if (!powerOn) return;
  
  // Check if both IR sensors detect black
  int irFront = digitalRead(IR_FRONT_PIN);
  int irBack = digitalRead(IR_BACK_PIN);
  if (irFront != BLACK || irBack != BLACK) {
    Serial.println("Safety check: Cannot turn - IR sensor(s) detect WHITE");
    stopMotors();
    return;
  }
  
  digitalWrite(IN1, HIGH);  // Left motor forward
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);   // Right motor backward
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}

// Modify the runAutoMode function to handle different sensor logic
void runAutoMode() {
  if (!powerOn || !autoMode) {
    stopMotors();
    return;
  }

  int irFront = digitalRead(IR_FRONT_PIN);
  int irBack = digitalRead(IR_BACK_PIN);
  long distance = readUltrasonic();

  // Debug output
  Serial.println("\n--- Auto Mode Status ---");
  Serial.print("Front IR Raw: ");
  Serial.println(irFront);
  Serial.print("Back IR Raw: ");
  Serial.println(irBack);
  Serial.print("Distance: ");
  Serial.println(distance);
  
  // Check if sensors detect white (using BLACK constant)
  bool frontDetectsWhite = (irFront != BLACK);
  bool backDetectsWhite = (irBack != BLACK);
  
  Serial.print("Front detects white: ");
  Serial.println(frontDetectsWhite ? "YES" : "NO");
  Serial.print("Back detects white: ");
  Serial.println(backDetectsWhite ? "YES" : "NO");

  // Both sensors detect white - stop immediately
  if (frontDetectsWhite && backDetectsWhite) {
    Serial.println("DANGER: Both sensors detect WHITE - Stopping");
    stopMotors();
    return;
  }

  // Front sensor detects white - move backward
  if (frontDetectsWhite) {
    Serial.println("Front WHITE detected - Moving backward");
    // Override safety checks for recovery movement
    digitalWrite(IN1, LOW);   // Left motor backward
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);   // Right motor backward
    digitalWrite(IN4, HIGH);
    analogWrite(ENA, motorSpeed);
    analogWrite(ENB, motorSpeed);
    return;
  }

  // Back sensor detects white - move forward
  if (backDetectsWhite) {
    Serial.println("Back WHITE detected - Moving forward");
    // Override safety checks for recovery movement
    digitalWrite(IN1, HIGH);  // Left motor forward
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);  // Right motor forward
    digitalWrite(IN4, LOW);
    analogWrite(ENA, motorSpeed);
    analogWrite(ENB, motorSpeed);
    return;
  }

  // Both sensors on black - normal operation
  if (irFront == BLACK && irBack == BLACK) {
    // Check if we detect an opponent
    if (distance > 0 && distance < DETECT_DISTANCE) {
      // Calculate speed based on distance - closer = faster
      int attackSpeed;
      
      // If very close, use maximum speed for pushing
      if (distance < ATTACK_DISTANCE) {
        Serial.println("Target in ATTACK range - Maximum power!");
        attackSpeed = ATTACK_SPEED;
      } else {
        // Scale speed based on distance - the closer, the faster
        attackSpeed = map(distance, DETECT_DISTANCE, ATTACK_DISTANCE, SEARCH_SPEED + 10, ATTACK_SPEED);
        Serial.print("Target detected - Approaching with speed: ");
        Serial.println(attackSpeed);
      }
      
      // Move forward with the calculated attack speed
      moveForwardWithSpeed(attackSpeed);
    } else {
      // No object detected - use slow search speed
      Serial.println("Searching - Using slow speed");
      
      // Implement a smarter search pattern - alternate between turning and moving forward
      static unsigned long lastSearchChange = 0;
      static bool isMovingForward = false;
      unsigned long currentTime = millis();
      
      // Change search behavior every 1.5 seconds
      if (currentTime - lastSearchChange > 1500) {
        isMovingForward = !isMovingForward;
        lastSearchChange = currentTime;
        
        if (isMovingForward) {
          Serial.println("Search pattern: Moving forward briefly");
        } else {
          Serial.println("Search pattern: Turning to scan");
        }
      }
      
      if (isMovingForward) {
        // Move forward briefly to cover more ground - use SEARCH_SPEED (slow)
        moveForwardWithSpeed(SEARCH_SPEED);
      } else {
        // Turn to scan the area - use SEARCH_SPEED (slow)
        turnRightWithSpeed(SEARCH_SPEED);
      }
    }
  }
}

// Update the mode toggle handler
void toggleMode() {
  autoMode = !autoMode;
  
  if (autoMode) {
    Serial.println("Auto Mode ON - Starting in 5 seconds...");
    
    // Visual countdown in serial monitor
    for (int i = 5; i > 0; i--) {
      Serial.print(i);
      Serial.println(" seconds remaining...");
      delay(1000); // Wait for 1 second
    }
    
    Serial.println("Auto Mode activated!");
  } else {
    Serial.println("Manual Mode ON");
    stopMotors();  // Stop motors when switching to manual
  }
}

// Function to get battery voltage
float readBatteryVoltage() {
  int rawValue = analogRead(batteryPin);  // Read raw value from the battery pin
  float voltage = (rawValue / 4095.0) * 3.3;  // Convert to voltage (3.3V reference)
  return voltage * (3.3 / 1.0);  // Adjust this value based on your voltage divider ratio
}

long readUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Increased timeout for better detection
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);  // 30ms timeout
  
  if (duration == 0) return -1;  // Return -1 if timeout
  
  // Calculate distance
  long distance = duration * 0.034 / 2;
  
  // Filter out distances greater than 50cm or less than 0cm
  if (distance > 50 || distance < 0) return -1;
  
  return distance;
}

void handleRoot() {
  const char* html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>SUBOMOTO ROBOT</title>
  <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Poppins:wght@400;600&display=swap');

    :root {
      --primary-color: #070603;
      --danger-color: #001aff;
      --warning-color: #f59e0b;
      --info-color: #38bdf8;
    }

    body {
      margin: 0;
      font-family: 'Poppins', sans-serif;
      color: white;
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      background: #BF9264;
      background-size: 600% 600%;
      animation: rainbow 18s ease infinite;
      padding: 20px;
    }

    @keyframes rainbow {
      0% {background-position: 0% 50%;}
      50% {background-position: 100% 50%;}
      100% {background-position: 0% 50%;}
    }

    .container {
      max-width: 800px;
      width: 100%;
    }

    h1 {
      font-size: 3rem;
      font-weight: 600;
      margin: 10px 0 5px;
      text-align: center;
      text-shadow: 2px 2px 10px rgba(0,0,0,0.4);
    }

    .status-container {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 20px;
      flex-wrap: wrap;
      gap: 10px;
    }

    #status {
      font-size: 1rem;
      color: #eee;
      padding: 8px 15px;
      border-radius: 20px;
      background: rgba(0, 0, 0, 0.2);
      transition: all 0.3s ease;
    }

    #status.connected {
      background: rgba(255, 247, 0, 0.3);
    }

    #status.disconnected {
      background: rgba(0, 0, 0, 0.3);
    }

    #battery {
      font-size: 1.2rem;
      color: #eee;
      padding: 8px 15px;
      border-radius: 20px;
      background: rgba(0, 0, 0, 0.2);
      transition: all 0.3s ease;
    }

    #battery.low {
      background: rgba(239, 68, 68, 0.3);
    }

    #battery.medium {
      background: rgba(245, 158, 11, 0.3);
    }

    #battery.good {
      background: rgba(74, 222, 128, 0.3);
    }

    .controls-container {
      display: flex;
      flex-direction: column;
      gap: 20px;
      margin-top: 20px;
    }

    .control-panel {
      background: rgba(255, 255, 255, 0.1);
      border-radius: 20px;
      padding: 20px;
      box-shadow: 0 8px 20px rgba(0,0,0,0.2);
      backdrop-filter: blur(10px);
      transition: all 0.3s ease;
    }

    .control-panel:hover {
      transform: translateY(-5px);
      box-shadow: 0 12px 25px rgba(0,0,0,0.25);
    }

    .panel-header {
      display: flex;
      justify-content: center;
      align-items: center;
      margin-bottom: 15px;
      border-bottom: 1px solid rgba(255, 255, 255, 0.2);
      padding-bottom: 10px;
    }

    .panel-title {
      font-size: 1.4rem;
      font-weight: 600;
      margin: 0;
      text-align: center;
      width: 100%;
    }

    #sensors {
      text-align: center;
      font-size: 1.2rem;
      color: #fff;
      transition: all 0.3s ease;
    }

    #sensors p {
      margin: 10px 0;
      font-weight: 500;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }

    #sensors span {
      font-weight: 600;
      padding: 6px 12px;
      border-radius: 8px;
      transition: all 0.3s ease;
      min-width: 80px;
      text-align: center;
    }

    .ir-black {
      background: #222;
      color: white;
    }

    .ir-white {
      background: #fff;
      color: #222;
    }

    .ultra-highlight {
      color: #38bdf8;
      background: rgba(56, 189, 248, 0.1);
    }

    .ultra-warning {
      color: #f59e0b;
      background: rgba(245, 158, 11, 0.1);
    }

    .ultra-danger {
      color: #ef4444;
      background: rgba(239, 68, 68, 0.1);
    }

    .switch-container {
      display: flex;
      justify-content: center;
      align-items: center;
      gap: 15px;
      margin: 10px 0;
    }

    .switch {
      position: relative;
      display: inline-block;
      width: 70px;
      height: 38px;
    }
    
    .switch input { opacity: 0; width: 0; height: 0; }
    
    .slider {
      position: absolute;
      cursor: pointer;
      top: 0; left: 0; right: 0; bottom: 0;
      background-color: #ccc;
      transition: 0.4s;
      border-radius: 34px;
    }
    
    .slider:before {
      position: absolute;
      content: "";
      height: 30px;
      width: 30px;
      left: 4px;
      bottom: 4px;
      background-color: white;
      transition: 0.4s;
      border-radius: 50%;
    }
    
    input:checked + .slider { background-color: var(--primary-color); }
    input:checked + .slider:before { transform: translateX(32px); }

    .controller {
      display: grid;
      grid-template-columns: repeat(3, 90px);
      grid-template-rows: repeat(3, 90px);
      gap: 15px;
      justify-items: center;
      align-items: center;
      margin: 0 auto;
      justify-content: center;
    }

    .btn {
      width: 90px;
      height: 90px;
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 2rem;
      border-radius: 10px;
      background: rgba(255, 255, 255, 0.1);
      border: 2px solid rgb(0, 0, 0);
      cursor: pointer;
      transition: 0.3s;
      user-select: none;
    }

    .btn:hover {
      transform: scale(1.05);
      background: rgba(255, 255, 255, 0.2);
    }

    .btn:active {
      transform: scale(0.95);
      background: rgba(255, 255, 255, 0.3);
    }

    .btn.active {
      background: rgb(0, 0, 0);
      border-color: var(--primary-color);
    }

    .speed-control {
      text-align: center;
      margin-top: 15px;
    }

    .speed-label {
      display: flex;
      justify-content: space-between;
      margin-bottom: 10px;
    }

    input[type="range"] {
      width: 100%;
      height: 10px;
      border-radius: 5px;
      background: rgba(255, 255, 255, 0.2);
      outline: none;
      -webkit-appearance: none;
      appearance: none;
    }

    input[type="range"]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 20px;
      height: 20px;
      border-radius: 50%;
      background: white;
      cursor: pointer;
    }

    .presets {
      display: flex;
      justify-content: space-between;
      margin-top: 15px;
    }

    .preset-btn {
      padding: 8px 15px;
      border-radius: 10px;
      background: rgba(255, 255, 255, 0.1);
      border: 1px solid white;
      color: white;
      cursor: pointer;
      transition: 0.3s;
      font-size: 0.9rem;
    }

    .preset-btn:hover {
      background: rgba(255, 255, 255, 0.2);
    }

    footer {
      margin-top: 30px;
      color: #eee;
      font-size: 0.9rem;
      text-align: center;
    }

    .keyboard-controls {
      margin-top: 15px;
      text-align: center;
      font-size: 0.9rem;
      color: rgba(255, 255, 255, 0.7);
    }

    .keyboard-key {
      display: inline-block;
      padding: 5px 10px;
      background: rgba(255, 255, 255, 0.1);
      border-radius: 5px;
      margin: 0 3px;
    }

    .log-container {
      max-height: 150px;
      overflow-y: auto;
      margin-top: 15px;
      padding: 10px;
      background: rgba(0, 0, 0, 0.2);
      border-radius: 10px;
      font-size: 0.9rem;
    }

    .log-entry {
      margin: 5px 0;
      padding: 5px;
      border-radius: 5px;
    }

    .log-info {
      background: rgba(56, 189, 248, 0.1);
    }

    .log-warning {
      background: rgba(245, 158, 11, 0.1);
    }

    .log-error {
      background: rgba(239, 68, 68, 0.1);
    }

    @media (max-width: 600px) {
      .controller {
        grid-template-columns: repeat(3, 70px);
        grid-template-rows: repeat(3, 70px);
        gap: 10px;
      }
      
      .btn {
        width: 70px;
        height: 70px;
        font-size: 1.5rem;
      }
      
      h1 {
        font-size: 2rem;
      }
      
      .panel-title {
        font-size: 1.2rem;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>SUBOMOTO CONTROLLER</h1>
    
    <div class="status-container">
      <div id="status">Status: Connecting...</div>
      <div id="battery">Battery : ** V</div>
    </div>
    
    <div class="controls-container">
      <div class="control-panel">
        <div class="panel-header">
          <h2 class="panel-title"><i class="fas fa-power-off"></i>SWITCH</h2>
        </div>
        
        <div class="switch-container">
          <span>Power:</span>
          <label class="switch">
            <input type="checkbox" id="powerSwitch" checked onchange="togglePower()">
            <span class="slider"></span>
          </label>
        </div>
        
        <div class="switch-container">
          <span>Mode: <strong id="modeLabel">Manual</strong></span>
          <label class="switch">
            <input type="checkbox" id="modeSwitch" onchange="toggleMode()">
            <span class="slider"></span>
          </label>
        </div>
      </div>
      
      <div class="control-panel">
        <div class="panel-header">
          <h2 class="panel-title"><i class="fas fa-gamepad"></i> Controls</h2>
        </div>
        
        <div class="controller">
          <div></div>
          <div class="btn" id="forwardBtn" ontouchstart="controlStart('forward')" ontouchend="controlStop()" onmousedown="controlStart('forward')" onmouseup="controlStop()" onmouseleave="controlStop()">▲</div>
          <div></div>
          <div class="btn" id="leftBtn" ontouchstart="controlStart('left')" ontouchend="controlStop()" onmousedown="controlStart('left')" onmouseup="controlStop()" onmouseleave="controlStop()">◄</div>
          <div class="btn" id="stopBtn" onclick="controlStart('stop')">↯</div>
          <div class="btn" id="rightBtn" ontouchstart="controlStart('right')" ontouchend="controlStop()" onmousedown="controlStart('right')" onmouseup="controlStop()" onmouseleave="controlStop()">►</div>
          <div></div>
          <div class="btn" id="backwardBtn" ontouchstart="controlStart('backward')" ontouchend="controlStop()" onmousedown="controlStart('backward')" onmouseup="controlStop()" onmouseleave="controlStop()">▼</div>
          <div></div>
        </div>
        
        <div class="keyboard-controls">
          Use keyboard: <span class="keyboard-key">W</span> <span class="keyboard-key">A</span> <span class="keyboard-key">S</span> <span class="keyboard-key">D</span> to control, <span class="keyboard-key">Space</span> to stop
        </div>
      </div>
      
      <div class="control-panel">
        <div class="panel-header">
          <h2 class="panel-title"><i class="fas fa-tachometer-alt"></i> Speed Control</h2>
        </div>
        
        <div class="speed-control">
          <div class="speed-label">
            <span>Speed:</span>
            <span id="speedValue">200</span>
          </div>
          <input type="range" id="speed" min="100" max="255" value="200" 
                 oninput="updateSpeed(this.value)"
                 onchange="setSpeed(this.value)">
          
          <div class="presets">
            <button class="preset-btn" onclick="setSpeedPreset(100)">Slow</button>
            <button class="preset-btn" onclick="setSpeedPreset(175)">Medium</button>
            <button class="preset-btn" onclick="setSpeedPreset(255)">Fast</button>
          </div>
        </div>
      </div>
      
      <div class="control-panel">
        <div class="panel-header">
          <h2 class="panel-title"><i class="fas fa-robot"></i> Sensor Readings</h2>
        </div>
        
        <div id="sensors">
          <p>
            <span>IR Front:</span>
            <span id="irFront" class="ir-white">--</span>
          </p>
          <p>
            <span>IR Back:</span>
            <span id="irBack" class="ir-white">--</span>
          </p>
          <p>
            <span>Distance:</span>
            <span id="ultra" class="ultra-highlight">--</span><span>cm</span>
          </p>
        </div>
      </div>
      
      <div class="control-panel">
        <div class="panel-header">
          <h2 class="panel-title"><i class="fas fa-history"></i> Activity Log</h2>
        </div>
        
        <div id="logContainer" class="log-container">
          <div class="log-entry log-info">System initialized</div>
        </div>
      </div>
    </div>
  </div>
  <script>
    const server = new WebSocket('ws://' + window.location.hostname + ':81');
    let isConnected = false;
    let isPowerOn = true;
    let isModeManual = true;
    let isMoving = false;
    let speed = 200;
    let lastMoveTime = 0;
    let moveInterval = null;
    let activeControl = null;
    let controlInterval = null;
    
    function addLog(message, type = 'info') {
      const logContainer = document.getElementById('logContainer');
      const logEntry = document.createElement('div');
      logEntry.className = 'log-entry log-' + type;
      logEntry.textContent = '[' + new Date().toLocaleTimeString() + '] ' + message;
      logContainer.appendChild(logEntry);
      logContainer.scrollTop = logContainer.scrollHeight;
      
      while (logContainer.children.length > 50) {
        logContainer.removeChild(logContainer.firstChild);
      }
    }
    
    function togglePower() {
      const powerSwitch = document.getElementById('powerSwitch');
      fetch('/togglePower')
        .then(() => {
          addLog('Power turned ' + (powerSwitch.checked ? 'ON' : 'OFF'));
        })
        .catch(error => {
          addLog('Failed to toggle power: ' + error, 'error');
        });
    }
    
    function toggleMode() {
      const modeSwitch = document.getElementById('modeSwitch');
      const modeLabel = document.getElementById('modeLabel');
      fetch('/toggleMode')
        .then(() => {
          const newMode = modeSwitch.checked ? "Auto" : "Manual";
          modeLabel.innerText = newMode;
          addLog('Mode changed to ' + newMode);
        })
        .catch(error => {
          addLog('Failed to toggle mode: ' + error, 'error');
        });
    }
    
    function controlStart(direction) {
      if (controlInterval) {
        clearInterval(controlInterval);
        controlInterval = null;
      }
      
      document.querySelectorAll('.btn').forEach(btn => btn.classList.remove('active'));
      
      const btnId = direction + 'Btn';
      const btn = document.getElementById(btnId);
      if (btn) btn.classList.add('active');
      
      activeControl = direction;
      
      sendCommand(direction);
      
      if (direction !== 'stop') {
        controlInterval = setInterval(() => {
          sendCommand(direction);
        }, 200);
      }
    }
    
    function controlStop() {
      if (controlInterval) {
        clearInterval(controlInterval);
        controlInterval = null;
      }
      
      document.querySelectorAll('.btn').forEach(btn => btn.classList.remove('active'));
      
      if (activeControl && activeControl !== 'stop') {
        sendCommand('stop');
        document.getElementById('stopBtn').classList.add('active');
        setTimeout(() => {
          document.getElementById('stopBtn').classList.remove('active');
        }, 300);
      }
      
      activeControl = null;
    }
    
    function sendCommand(command) {
      if (server.readyState === WebSocket.OPEN) {
        server.send(command);
        if (!controlInterval || command === 'stop') {
          addLog('Command sent: ' + command);
        }
      } else {
        addLog('WebSocket not connected, command failed: ' + command, 'error');
        controlStop();
      }
    }
    
    function initWebSocket() {
      server.onopen = function() {
        isConnected = true;
        updateStatus();
      };
      
      server.onclose = function() {
        isConnected = false;
        updateStatus();
      };
      
      server.onmessage = function(event) {
        const data = JSON.parse(event.data);
        if (data.type === 'sensors') {
          updateSensors(data);
        }
      };
    }
    
    function updateSpeed(value) {
      document.getElementById('speedValue').innerText = value;
    }
    
    function setSpeed(value) {
      fetch('/speed?value=' + value)
        .then(() => {
          addLog('Speed set to ' + value);
        })
        .catch(error => {
          addLog('Failed to set speed: ' + error, 'error');
        });
    }
    
    function setSpeedPreset(value) {
      const speedSlider = document.getElementById('speed');
      speedSlider.value = value;
      updateSpeed(value);
      setSpeed(value);
    }
    
    function updateStatus() {
      const statusElement = document.getElementById('status');
      statusElement.textContent = 'Status: ' + (isConnected ? 'Connected' : 'Disconnected');
      statusElement.className = isConnected ? 'connected' : 'disconnected';
    }
    
    function updateBattery() {
      fetch('/battery')
        .then(response => response.text())
        .then(voltage => {
          const batteryElement = document.getElementById('battery');
          batteryElement.textContent = 'Battery: ' + voltage + 'V';
          
          const v = parseFloat(voltage);
          if (v < 3.3) {
            batteryElement.className = 'low';
          } else if (v < 3.7) {
            batteryElement.className = 'medium';
          } else {
            batteryElement.className = 'good';
          }
        })
        .catch(error => {
          console.error('Failed to update battery:', error);
        });
    }
    
    function updateSensors() {
      fetch('/sensors')
        .then(response => response.json())
        .then(data => {
          const irFront = document.getElementById('irFront');
          const irBack = document.getElementById('irBack');
          const ultra = document.getElementById('ultra');
          
          irFront.textContent = data.irFront ? 'BLACK' : 'WHITE';
          irFront.className = data.irFront ? 'ir-black' : 'ir-white';
          
          irBack.textContent = data.irBack ? 'BLACK' : 'WHITE';
          irBack.className = data.irBack ? 'ir-black' : 'ir-white';
          
          if (data.distance === -1) {
            ultra.textContent = 'OUT OF RANGE';
            ultra.className = 'ultra-warning';
          } else {
            ultra.textContent = data.distance;
            ultra.className = data.distance < 30 ? 'ultra-danger' : 'ultra-highlight';
          }
        })
        .catch(error => {
          console.error('Failed to update sensors:', error);
        });
    }
    
    document.addEventListener('keydown', function(event) {
      if (event.repeat) return;
      
      switch(event.key.toLowerCase()) {
        case 'w':
          controlStart('forward');
          break;
        case 's':
          controlStart('backward');
          break;
        case 'a':
          controlStart('left');
          break;
        case 'd':
          controlStart('right');
          break;
        case ' ':
          controlStart('stop');
          break;
      }
    });
    
    document.addEventListener('keyup', function(event) {
      switch(event.key.toLowerCase()) {
        case 'w':
        case 's':
        case 'a':
        case 'd':
          controlStop();
          break;
      }
    });
    
    window.onload = () => {
      initWebSocket();
      updateStatus();
      updateBattery();
      updateSensors();
      
      setInterval(updateSensors, 1000);  // Update every second
      setInterval(updateStatus, 3000);   // Check connection every 3 seconds
      setInterval(updateBattery, 5000);  // Check battery every 5 seconds
    };
  </script>
</body>
</html>)rawliteral";
  server.send(200, "text/html", html);
}
void handleSensors() {
  // Read sensor values
  int irFront = digitalRead(IR_FRONT_PIN);
  int irBack = digitalRead(IR_BACK_PIN);
  long distance = readUltrasonic();
  
  // Create JSON response
  StaticJsonDocument<200> doc;
  doc["irFront"] = irFront;
  doc["irBack"] = irBack;
  doc["distance"] = distance;
  
  String jsonResponse;
  serializeJson(doc, jsonResponse);
  
  server.send(200, "application/json", jsonResponse);
}

void handleBattery() {
  float voltage = readBatteryVoltage();
  String response = String(voltage);
  server.send(200, "text/plain", response);
}

// Modify setup() to include testing
void setup() {
  Serial.begin(9600);
  delay(1000);
  
  // WiFi Access Point setup
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  
  // Print WiFi connection details
  Serial.println("\nWiFi Access Point Setup:");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // DNS Server setup
  dnsServer.start(DNS_PORT, "*", apIP);
  
  // Web server setup  
  server.on("/", handleRoot);
  server.on("/sensors", handleSensors);
  server.on("/battery", handleBattery);
  server.on("/toggleMode", HTTP_GET, []() {
    toggleMode();
    server.send(200, "text/plain", autoMode ? "AUTO" : "MANUAL");
  });
  server.on("/speed", HTTP_GET, handleSpeed);
  server.begin();
  
  // WebSocket setup
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // Rest of your existing setup code...
  setupMotors();
  stopMotors();
  setupSensors();
  
  Serial.println("Setup complete - Ready for connections");
}

// Update the main loop
void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  webSocket.loop();
  
  // Add IR sensor debugging
  static unsigned long lastDebugTime = 0;
  if (millis() - lastDebugTime > 500) {  // Print every 500ms
    debugIRSensors();
    lastDebugTime = millis();
  }
  
  if (powerOn) {
    if (autoMode) {
      runAutoMode();  // Continuously run auto mode
    }
    // Manual mode is handled by webSocketEvent
  }
}
