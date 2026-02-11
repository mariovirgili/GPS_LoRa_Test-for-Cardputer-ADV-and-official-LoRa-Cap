/**
 * M5Cardputer "Granitica" Firmware v0.9 - FEEDBACK FIX
 * * STATUS: PERFECTED LOGIC.
 * * * CHANGES:
 * - VERSION: Reverted to v0.9.
 * - LOGIC: Separated UI feedback from Radio transmission.
 * - UI: Specific feedback header for Chat ("TX: SENDING..."), 
 * Ping ("SENDING PING...") and GeoBeacon ("SENDING GEO...").
 * * * HARDWARE:
 * - GPS: UART1 (RX=15, TX=13, Baud=115200)
 * - LoRa: SX1262 (CS=5, RST=3, IRQ=4, BUSY=6, TCXO=1.6V)
 */

#include <M5Cardputer.h>
#include <M5Unified.h>
#include <RadioLib.h>
#include <TinyGPS++.h>
#include <SPI.h>

// --- VERSION DEFINITION ---
#define FW_VERSION "v0.9"

// --- PIN DEFINITIONS ---
#define GPS_RX_PIN     15
#define GPS_TX_PIN     13
#define GPS_BAUD_RATE  115200

#define LORA_CS_PIN    5
#define LORA_RST_PIN   3
#define LORA_IRQ_PIN   4
#define LORA_BUSY_PIN  6
#define LORA_SCK_PIN   40
#define LORA_MISO_PIN  39
#define LORA_MOSI_PIN  14
#define LORA_TCXO_VOLT 1.6 

// --- KEY DEFINITIONS ---
#define KEY_ESC        27  

// --- UI CONSTANTS ---
#define HEADER_HEIGHT 25
#define FOOTER_Y      120
#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 135

enum AppMode { MODE_GPS, MODE_LORA_TERM, MODE_LORA_SNIFFER, MODE_HELP };
enum ChatState { CHAT_TYPING, CHAT_COMMANDS };

TinyGPSPlus gps;
HardwareSerial gpsSerial(1);
SX1262 radio = new Module(LORA_CS_PIN, LORA_IRQ_PIN, LORA_RST_PIN, LORA_BUSY_PIN);

AppMode currentMode = MODE_GPS;
ChatState chatState = CHAT_TYPING;
bool fullRedrawNeeded = true;
String lastLoRaMessage = "No Data";
String oldLoRaMessage = ""; 
float lastRssi = 0;
float lastSnr = 0;
int sniffCursorX = 0;
int currentSF = 9;

// Chat / Input Variables
String inputBuffer = "";
bool inputChanged = false;

// Help System State
int helpPage = 0;
const int MAX_HELP_PAGES = 5; 

// GPS UI State
bool wasFix = false;       
bool firstRunGPS = true;   

// ==========================================
// --- INITIALIZATION ---
// ==========================================

void initLoRa() {
    SPI.begin(LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_CS_PIN);
    Serial.printf("[RADIO] Setting SF to %d\r\n", currentSF);
    int state = radio.begin(868.0, 125.0, currentSF, 7, 0x12, 10, 8, LORA_TCXO_VOLT, false);
    if (state == RADIOLIB_ERR_NONE) radio.startReceive();
    else Serial.printf("[RADIO] Init Failed: %d\r\n", state);
}

void initGPS() {
    gpsSerial.begin(GPS_BAUD_RATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
}

void changeSF() {
    if (currentSF == 7) currentSF = 9;
    else if (currentSF == 9) currentSF = 12;
    else currentSF = 7;
    initLoRa();
    
    M5.Display.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, BLUE);
    M5.Display.setTextColor(WHITE, BLUE);
    M5.Display.setCursor(5, 5);
    M5.Display.printf("RADIO: SET SF %d", currentSF);
    delay(500);
    fullRedrawNeeded = true;
}

// ==========================================
// --- LOGIC ---
// ==========================================

void logGPSToSerial() {
    if (gps.location.isValid()) {
        Serial.printf("[GPS] FIX: YES | Lat: %.6f | Lon: %.6f | Alt: %.0fm | Sats: %d\r\n", 
                      gps.location.lat(), gps.location.lng(), gps.altitude.meters(), gps.satellites.value());
    } else {
        Serial.printf("[GPS] FIX: NO  | Sats Visible: %d\r\n", gps.satellites.value());
    }
}

// Helper to show Magenta Header Feedback
void flashHeader(String text) {
    M5.Display.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, MAGENTA);
    M5.Display.setTextColor(WHITE, MAGENTA);
    M5.Display.setCursor(5, 5);
    M5.Display.setTextSize(1.5);
    M5.Display.print(text);
}

// Low-level Packet Sender (No UI side effects except delay/redraw trigger)
void sendPacket(String payload) {
    Serial.printf("[TX] SF:%d | Payload: %s\r\n", currentSF, payload.c_str());
    radio.transmit(payload);
    radio.startReceive();
    
    delay(200); 
    fullRedrawNeeded = true; 
}

void sendGeoBeacon() {
    flashHeader("SENDING GEO...");
    
    String msg;
    if (gps.location.isValid()) 
        msg = "GEO:" + String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6);
    else 
        msg = "BEACON: No GPS Fix";
    
    sendPacket(msg);
}

void sendPing() {
    flashHeader("SENDING PING...");
    sendPacket("PING from Cardputer (SF" + String(currentSF) + ")");
}

void sendChatMessage() {
    if (inputBuffer.length() > 0) {
        flashHeader("TX: SENDING...");
        sendPacket(inputBuffer);
        inputBuffer = ""; 
        inputChanged = true;
    }
}

// ==========================================
// --- DRAWING ---
// ==========================================

void drawStaticHeader(String title, uint16_t color) {
    M5.Display.fillScreen(BLACK);
    M5.Display.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, color);
    M5.Display.setTextColor(BLACK, color); 
    M5.Display.setTextSize(1.5);
    M5.Display.setCursor(5, 5);
    M5.Display.print(title);
    
    if (currentMode != MODE_HELP) {
        M5.Display.setCursor(170, 5);
        M5.Display.printf("[SF %d]", currentSF);
    }
    
    // Footer Logic
    M5.Display.drawFastHLine(0, SCREEN_HEIGHT - 18, SCREEN_WIDTH, DARKGREY);
    M5.Display.setTextColor(LIGHTGREY, BLACK);
    M5.Display.setTextSize(1);
    
    if (currentMode == MODE_HELP) {
        M5.Display.setCursor(5, SCREEN_HEIGHT - 12);
        M5.Display.print("ARROWS: Page | ESC: Exit");
    } 
    else if (currentMode == MODE_LORA_TERM) {
        M5.Display.setCursor(5, SCREEN_HEIGHT - 12);
        if (chatState == CHAT_TYPING) {
            M5.Display.setTextColor(CYAN, BLACK);
            M5.Display.print("TYPE Message > ENTER or ESC");
        } else {
            M5.Display.setTextColor(RED, BLACK);
            M5.Display.print("SPACE for PING | ENTER for GeoBeacon");
        }
    }
    else {
        String footerText = "Press 'H' for Commands";
        int textW = M5.Display.textWidth(footerText);
        int centerX = (SCREEN_WIDTH - textW) / 2;
        M5.Display.setCursor(centerX, SCREEN_HEIGHT - 12);
        M5.Display.print(footerText);
    }
}

void updateHelpMode() {
    if (fullRedrawNeeded) {
        drawStaticHeader("MANUAL / HELP", DARKGREY);
        M5.Display.setTextColor(WHITE, BLACK);
        M5.Display.setTextSize(1.5); 
        M5.Display.setCursor(5, 35);

        if (helpPage == 0) {
            // PAGE 1: NAVIGATION (CLEANED)
            M5.Display.println("NAVIGATION:");
            M5.Display.println("");
            M5.Display.println(" [ESC] Back/Exit");
            M5.Display.println("");
            M5.Display.println(" [ARROWS] Scroll Pages");
        } 
        else if (helpPage == 1) {
            M5.Display.println("APP MODES:");
            M5.Display.println("");
            M5.Display.println(" [G] GPS Monitor");
            M5.Display.println(" [L] LoRa Chat/Term");
            M5.Display.println(" [S] RSSI Sniffer");
            M5.Display.println(" Switch with G, L, S keys");
        }
        else if (helpPage == 2) {
            M5.Display.println("LORA CHAT (1/2):");
            M5.Display.println("");
            M5.Display.println(" 1. TYPE to Chat.");
            M5.Display.println(" 2. ENTER to Send.");
            M5.Display.println(" 3. Press ESC once for");
            M5.Display.println("    COMMAND MODE.");
        }
        else if (helpPage == 3) {
            M5.Display.println("LORA CHAT (2/2):");
            M5.Display.println("");
            M5.Display.println(" 4. In Command Mode:");
            M5.Display.println("    [SPACE] = Ping");
            M5.Display.println("    [ENTER] = GeoBeacon");
            M5.Display.println(" 5. Press ESC again to");
            M5.Display.println("    Exit App.");
        }
        else if (helpPage == 4) {
            M5.Display.println("RADIO THEORY (SF):");
            M5.Display.setTextSize(1);
            M5.Display.println("");
            M5.Display.println("SF 7: FAST / LOW RANGE");
            M5.Display.println("      Low battery usage.");
            M5.Display.println("");
            M5.Display.println("SF 9: BALANCED (Default)");
            M5.Display.println("");
            M5.Display.println("SF 12: SLOW / MAX RANGE");
            M5.Display.println("       Obstacle penetration.");
        }
        
        M5.Display.setTextSize(1.5);
        M5.Display.setTextColor(YELLOW, BLACK);
        M5.Display.setCursor(200, 35);
        M5.Display.printf("%d/%d", helpPage + 1, MAX_HELP_PAGES);
        
        fullRedrawNeeded = false;
    }
}

void updateGPSMode() {
    bool isFix = gps.location.isValid();
    
    if (fullRedrawNeeded || (isFix != wasFix) || firstRunGPS) {
        String headerTitle = "GPS MONITOR " + String(FW_VERSION);
        drawStaticHeader(headerTitle, GREEN);
        
        M5.Display.fillRect(0, HEADER_HEIGHT, SCREEN_WIDTH, FOOTER_Y - HEADER_HEIGHT, BLACK);

        if (isFix) {
            M5.Display.setTextColor(LIGHTGREY, BLACK);
            M5.Display.setTextSize(1);
            
            M5.Display.setCursor(5, 30); M5.Display.print("LATITUDE");
            M5.Display.setCursor(5, 60); M5.Display.print("LONGITUDE");
            M5.Display.setCursor(5, 90);  M5.Display.print("ALT");
            M5.Display.setCursor(80, 90); M5.Display.print("SPD (kmh)"); 
            M5.Display.setCursor(160, 90); M5.Display.print("SATS");
            
            // Time Label Centered
            M5.Display.setCursor(150, 30); M5.Display.print("TIME (UTC)");
        }
        wasFix = isFix;
        firstRunGPS = false;
        fullRedrawNeeded = false;
    }

    static long lastScreenRefresh = 0;
    if (millis() - lastScreenRefresh < 500) return; 
    lastScreenRefresh = millis();

    if (isFix) {
        M5.Display.setTextColor(WHITE, BLACK);
        M5.Display.setTextSize(1.5);

        M5.Display.fillRect(5, 42, 160, 16, BLACK);
        M5.Display.setCursor(5, 42); M5.Display.printf("%.6f", gps.location.lat());
        
        M5.Display.fillRect(5, 72, 160, 16, BLACK);
        M5.Display.setCursor(5, 72); M5.Display.printf("%.6f", gps.location.lng());
        
        M5.Display.fillRect(5, 102, 60, 16, BLACK);
        M5.Display.setCursor(5, 102); M5.Display.printf("%.0fm", gps.altitude.meters());
        
        // Speed with 2 Decimals
        M5.Display.fillRect(80, 102, 60, 16, BLACK);
        M5.Display.setCursor(80, 102); M5.Display.printf("%.2f", gps.speed.kmph());
        
        M5.Display.fillRect(160, 102, 40, 16, BLACK);
        M5.Display.setTextColor(CYAN, BLACK);
        M5.Display.setCursor(160, 102); M5.Display.printf("%d", gps.satellites.value());

        M5.Display.setTextColor(YELLOW, BLACK);
        M5.Display.setCursor(160, 42); M5.Display.printf("%02d:%02d", gps.time.hour(), gps.time.minute());

    } else {
        M5.Display.setTextColor(RED, BLACK);
        M5.Display.setTextSize(2);
        M5.Display.setCursor(55, 55); M5.Display.print("NO GPS FIX");
        
        M5.Display.setTextSize(1.5);
        M5.Display.setTextColor(WHITE, BLACK);
        M5.Display.fillRect(0, 90, SCREEN_WIDTH, 20, BLACK);
        M5.Display.setCursor(65, 90); 
        M5.Display.printf("Sats Visible: %d", gps.satellites.value());
    }
}

void updateLoRaTermMode() {
    if (fullRedrawNeeded) {
        drawStaticHeader("LORA TERMINAL", ORANGE);
        M5.Display.setTextColor(LIGHTGREY, BLACK);
        M5.Display.setTextSize(1.5);
        M5.Display.setCursor(5, 30); M5.Display.print("Last Received:");
        
        M5.Display.drawRect(0, 42, SCREEN_WIDTH, 40, WHITE);
        
        uint16_t boxColor = (chatState == CHAT_TYPING) ? LIGHTGREY : RED;
        if (chatState == CHAT_TYPING) M5.Display.drawFastHLine(0, 95, SCREEN_WIDTH, boxColor);
        else M5.Display.drawRect(0, 95, SCREEN_WIDTH, 22, boxColor);
        
        M5.Display.setCursor(5, 100); 
        M5.Display.setTextColor(CYAN, BLACK);
        M5.Display.print("> ");
        
        fullRedrawNeeded = false;
        oldLoRaMessage = ""; 
        inputChanged = true; 
    }

    if (lastLoRaMessage != oldLoRaMessage) {
        M5.Display.fillRect(2, 44, SCREEN_WIDTH-4, 36, BLACK);
        M5.Display.setTextColor(GREEN, BLACK);
        M5.Display.setTextSize(1.5);
        M5.Display.setCursor(5, 50);
        M5.Display.print(lastLoRaMessage);
        oldLoRaMessage = lastLoRaMessage; 
        
        M5.Display.setTextColor(WHITE, BLACK);
        M5.Display.setCursor(140, 30); 
        M5.Display.printf("RSSI:%.0f", lastRssi);
    }
    
    if (inputChanged) {
        M5.Display.fillRect(20, 100, 220, 16, BLACK);
        M5.Display.setCursor(20, 100);
        M5.Display.setTextColor(CYAN, BLACK);
        M5.Display.print(inputBuffer);
        if (chatState == CHAT_TYPING) M5.Display.print("_");
        inputChanged = false;
    }
}

void updateSnifferMode() {
    if (fullRedrawNeeded) {
        drawStaticHeader("LORA SPECTRUM", RED);
        sniffCursorX = 0;
        fullRedrawNeeded = false;
    }
    float rssi = radio.getRSSI();
    if (rssi < -130) rssi = -130; if (rssi > -40) rssi = -40;
    int h = map((int)rssi, -130, -40, 0, SCREEN_HEIGHT - 20 - HEADER_HEIGHT);
    M5.Display.drawFastVLine(sniffCursorX, HEADER_HEIGHT, SCREEN_HEIGHT - 20 - HEADER_HEIGHT, BLACK); 
    M5.Display.drawFastVLine(sniffCursorX, SCREEN_HEIGHT - 20 - h, h, (rssi > -95) ? GREEN : BLUE);
    M5.Display.drawFastVLine((sniffCursorX + 1) % SCREEN_WIDTH, HEADER_HEIGHT, SCREEN_HEIGHT - 20 - HEADER_HEIGHT, WHITE);
    if (sniffCursorX % 20 == 0) {
        M5.Display.fillRect(170, 5, 60, 15, RED); 
        M5.Display.setTextColor(WHITE, RED);
        M5.Display.setCursor(175, 5); M5.Display.printf("%.0f", rssi);
    }
    sniffCursorX++; if (sniffCursorX >= SCREEN_WIDTH) sniffCursorX = 0;
    delay(5);
}

// ==========================================
// --- SETUP & LOOP ---
// ==========================================

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5.Display.setRotation(1);
    
    Serial.begin(115200);
    Serial.printf("\r\n\r\n>> GRANITICA %s - FINAL <<\r\n", FW_VERSION);
    
    initGPS();
    initLoRa();
    
    drawStaticHeader("SYSTEM READY", BLUE);
    delay(1000);
    fullRedrawNeeded = true;
}

void loop() {
    M5Cardputer.update();
    
    while (gpsSerial.available()) gps.encode(gpsSerial.read());

    static long lastGpsLog = 0;
    if (millis() - lastGpsLog > 5000) {
        logGPSToSerial();
        lastGpsLog = millis();
    }

    int irq = radio.readData(lastLoRaMessage);
    if (irq == RADIOLIB_ERR_NONE) {
        if (lastLoRaMessage.length() > 0) {
            lastRssi = radio.getRSSI();
            lastSnr = radio.getSNR();
            Serial.printf("[RX] SF:%d | RSSI:%4.0f | MSG: %s\r\n", currentSF, lastRssi, lastLoRaMessage.c_str());
        }
    }

    // === INPUT HANDLING ===
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

        // 1. HELP NAVIGATION
        if (currentMode == MODE_HELP) {
            if (M5Cardputer.Keyboard.isKeyPressed('.') || M5Cardputer.Keyboard.isKeyPressed('/')) {
                helpPage++; if (helpPage >= MAX_HELP_PAGES) helpPage = 0; fullRedrawNeeded = true;
            }
            if (M5Cardputer.Keyboard.isKeyPressed(';') || M5Cardputer.Keyboard.isKeyPressed(',')) {
                helpPage--; if (helpPage < 0) helpPage = MAX_HELP_PAGES - 1; fullRedrawNeeded = true;
            }
            if (M5Cardputer.Keyboard.isKeyPressed(KEY_ESC) || M5Cardputer.Keyboard.isKeyPressed('`')) {
                currentMode = MODE_GPS; fullRedrawNeeded = true;
            }
            return;
        }

        // 2. LORA CHAT
        if (currentMode == MODE_LORA_TERM) {
            if (M5Cardputer.Keyboard.isKeyPressed(KEY_ESC) || M5Cardputer.Keyboard.isKeyPressed('`')) {
                if (chatState == CHAT_TYPING) {
                    chatState = CHAT_COMMANDS; 
                    fullRedrawNeeded = true;
                } else {
                    currentMode = MODE_GPS;    
                    chatState = CHAT_TYPING;
                    inputBuffer = "";
                    fullRedrawNeeded = true;
                }
                return;
            }

            if (chatState == CHAT_COMMANDS) {
                if (M5Cardputer.Keyboard.isKeyPressed(' ')) { sendPing(); chatState = CHAT_TYPING; fullRedrawNeeded = true; }
                if (status.enter) { sendGeoBeacon(); chatState = CHAT_TYPING; fullRedrawNeeded = true; }
                
                for (auto c : status.word) {
                    chatState = CHAT_TYPING;
                    inputBuffer += c;
                    inputChanged = true;
                    fullRedrawNeeded = true;
                }
                return;
            }

            if (chatState == CHAT_TYPING) {
                if (status.del && inputBuffer.length() > 0) {
                    inputBuffer.remove(inputBuffer.length() - 1);
                    inputChanged = true;
                }
                if (status.enter) {
                    sendChatMessage();
                }
                for (auto c : status.word) {
                    if (inputBuffer.length() < 30) {
                        inputBuffer += c;
                        inputChanged = true;
                    }
                }
            }
        }

        // 3. STANDARD NAVIGATION
        else {
            if (M5Cardputer.Keyboard.isKeyPressed(KEY_ESC) || M5Cardputer.Keyboard.isKeyPressed('`')) {
                currentMode = MODE_GPS; fullRedrawNeeded = true;
            }
            if (M5Cardputer.Keyboard.isKeyPressed('h')) { currentMode = MODE_HELP; helpPage = 0; fullRedrawNeeded = true; }
            if (M5Cardputer.Keyboard.isKeyPressed('g')) { currentMode = MODE_GPS; fullRedrawNeeded = true; }
            if (M5Cardputer.Keyboard.isKeyPressed('l')) { currentMode = MODE_LORA_TERM; chatState = CHAT_TYPING; fullRedrawNeeded = true; }
            if (M5Cardputer.Keyboard.isKeyPressed('s')) { currentMode = MODE_LORA_SNIFFER; fullRedrawNeeded = true; }
            
            if (status.enter) sendGeoBeacon();
            if (M5Cardputer.Keyboard.isKeyPressed(KEY_TAB)) changeSF();
            if (M5Cardputer.Keyboard.isKeyPressed(' ')) sendPing();
        }
    }

    switch (currentMode) {
        case MODE_GPS: updateGPSMode(); break;
        case MODE_LORA_TERM: updateLoRaTermMode(); break;
        case MODE_LORA_SNIFFER: updateSnifferMode(); break;
        case MODE_HELP: updateHelpMode(); break;
    }
}