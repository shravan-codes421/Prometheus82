const int CONTACT_PIN = 3;  // pin mapped to mute switch; measures whether or not the switch made contact with the controller under test.
const int SOLENOID_PIN = 2;  // pin that controls solenoid movement
unsigned long PULSE_DURATION_US = 40000;
// RESP means response. CMD means command.
const char DELAY_TEST_CMD = 'D';
const char DELAY_TEST_READY_RESP = 'R';

const char VERSION_PREAMBLE = 'V';

const char TRIGGER_SOLENOID_CMD = 'T';
const char SWITCH_MADE_CONTACT_RESP = 'S';

const char SET_PULSE_DURATION_CMD = 'P';
const char SET_PULSE_DURATION_RESP = 'A';

const char QUERY_CONTACT_CMD = 'Q';
const char CONTACT_CLOSED_RESP = 'H';
const char CONTACT_OPEN_RESP = 'U';

// firmware version; has to be chars for Serial.write to work properly.
const char VERSION_MAJOR = '1';
const char VERSION_MINOR = '1';
const char VERSION_PATCH = '1';

volatile bool windowActive = false, contactDetected = false, solenoidActive = false;
volatile unsigned long solenoidStartTime_us = 0;

void handleContact() {
    if (!windowActive) return;
    if (!contactDetected) {
        Serial.write(SWITCH_MADE_CONTACT_RESP);
        contactDetected = true;
        return;
    }
}

void print_firmware_version() {
    /*
    Prints firmware version of Arduino program in the form of RVx.y.z\n
    */
    Serial.write(DELAY_TEST_READY_RESP);
    Serial.write(VERSION_PREAMBLE);
    Serial.write(VERSION_MAJOR);
    Serial.write(".");
    Serial.write(VERSION_MINOR);
    Serial.write(".");
    Serial.write(VERSION_PATCH);
    Serial.write("\n");
}

void setup() {
    Serial.begin(115200);
    pinMode(CONTACT_PIN, INPUT_PULLUP);
    pinMode(SOLENOID_PIN, OUTPUT);

    // when mute switch goes from low to high
    attachInterrupt(digitalPinToInterrupt(CONTACT_PIN), handleContact, FALLING);

    for (int i = 0; i < 3; i++) {
        digitalWrite(SOLENOID_PIN, HIGH);
        delay(PULSE_DURATION_US / 1000);
        digitalWrite(SOLENOID_PIN, LOW);
        delay(200);
    }
    print_firmware_version();
}

void loop() {
    if (Serial.available() > 0) {
        char cmd = Serial.read();
        
        // Fast handling of 'D' command for delay test
        if (cmd == DELAY_TEST_CMD) {
            Serial.write(DELAY_TEST_READY_RESP);
            return; // Exit to avoid further checks
        }
        
        // Other commands are processed as before
        if (cmd == TRIGGER_SOLENOID_CMD) {
            contactDetected = false;
            digitalWrite(SOLENOID_PIN, HIGH);
            solenoidStartTime_us = micros();
            solenoidActive = true;
            windowActive = true;
        }
        else if (cmd == SET_PULSE_DURATION_CMD) {
            // set pulse duration from the python program
            unsigned long startTime = millis();
            while (Serial.available() < 2 && (millis() - startTime) < 50) {
                ;
            }
            if (Serial.available() >= 2) {
                unsigned long duration_ms = (Serial.read() << 8) | Serial.read();
                PULSE_DURATION_US = duration_ms * 1000;
                Serial.write(SET_PULSE_DURATION_RESP);  // setting the pulse duration was successful
            }
            while (Serial.available() > 0) {
                Serial.read();
            }
        }
        else if (cmd == QUERY_CONTACT_CMD) {
            int state = digitalRead(CONTACT_PIN);
            Serial.write(state == LOW ? CONTACT_CLOSED_RESP : CONTACT_OPEN_RESP);
        }
    }

    if (solenoidActive && (micros() - solenoidStartTime_us >= PULSE_DURATION_US)) {
        digitalWrite(SOLENOID_PIN, LOW);
        solenoidActive = false;
    }
}
