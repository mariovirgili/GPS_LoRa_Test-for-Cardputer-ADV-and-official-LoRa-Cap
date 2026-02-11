/**
 * M5Cardputer "Granitica" Firmware v0.6.0 - CLEAN UI
 * * STATUS: REFINED BASELINE.
 * * * CHANGES:
 * - UI: Footer reduced to "Press 'H' for Commands".
 * - UI: Help menu font size increased (1.5), split into 3 pages.
 * - UI: Help text displays logical keys (ESC, UP, DOWN) instead of physical map.
 * - FIX: Moved SF indicator to prevent clipping on "SF 12".
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
#define FW_VERSION "v0.6.0"

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

TinyGPSPlus gps;
HardwareSerial gpsSerial(1);
SX1262 radio = new Module(LORA_CS_PIN, LORA_IRQ_PIN, LORA_RST_PIN, LORA_BUSY_PIN);

AppMode currentMode = MODE_GPS;
bool fullRedrawNeeded = true;
String lastLoRaMessage = "No Data";
String oldLoRaMessage = ""; 
float lastRssi = 0;
float lastSnr = 0;
int sniffCursorX = 0;
int currentSF = 9;

// Help System State
int helpPage = 0;
const int MAX_HELP_PAGES = 3; // Increased to 3 pages for larger font

// GPS UI State
bool wasFix = false;       
bool firstRunGPS = true;   

// ==========================================
// --- INITIALIZATION FUNCTIONS ---
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
    
    // Quick Feedback banner
    M5.Display.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, BLUE);
    M5.Display.setTextColor(WHITE, BLUE);
    M5.Display.setCursor(5, 5);
    M5.Display.printf("RADIO: SET SF %d", currentSF);
    delay(500);
    fullRedrawNeeded = true;
}

// ==========================================
// --- LOGIC FUNCTIONS ---
// ==========================================

void logGPSToSerial() {
    if (gps.location.isValid()) {
        Serial.printf("[GPS] FIX: YES | Lat: %.6f | Lon: %.6f | Alt: %.0fm | Sats: %d\r\n", 
                      gps.location.lat(), gps.location.lng(), gps.altitude.meters(), gps.satellites.value());
    } else {
        Serial.printf("[GPS] FIX: NO  | Sats Visible: %d\r\n", gps.satellites.value());
    }
}

void sendPacket(String payload) {
    M5.Display.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, MAGENTA);
    M5.Display.setTextColor(WHITE, MAGENTA);
    M5.Display.setCursor(5, 5);
    M5.Display.setTextSize(1.5);
    M5.Display.print("TX: SENDING...");
    
    Serial.printf("[TX] SF:%d | Payload: %s\r\n", currentSF, payload.c_str());
    radio.transmit(payload);
    radio.startReceive();
    delay(300); 
    fullRedrawNeeded = true; 
}

void sendGeoBeacon() {
    String msg;
    if (gps.location.isValid()) 
        msg = "GEO:" + String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6);
    else 
        msg = "BEACON: No GPS Fix";
    sendPacket(msg);
}

void sendPing() {
    sendPacket("PING from Cardputer (SF" + String(currentSF) + ")");
}

// ==========================================
// --- DRAWING FUNCTIONS ---
// ==========================================

void drawStaticHeader(String title, uint16_t color) {
    M5.Display.fillScreen(BLACK);
    M5.Display.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, color);
    M5.Display.setTextColor(BLACK, color); 
    M5.Display.setTextSize(1.5);
    
    M5.Display.setCursor(5, 5);
    M5.Display.print(title);
    
    // Only show SF if not in Help Mode
    if (currentMode != MODE_HELP) {
        // Moved from 180 to 165 to fit "SF 12" without clipping
        M5.Display.setCursor(165, 5);
        M5.Display.printf("[SF %d]", currentSF);
    }
    
    // Simplified Footer
    M5.Display.drawFastHLine(0, SCREEN_HEIGHT - 18, SCREEN_WIDTH, DARKGREY);
    M5.Display.setTextColor(LIGHTGREY, BLACK);
    M5.Display.setTextSize(1.5); // Slightly larger footer text
    
    // Center the footer text roughly
    M5.Display.setCursor(40, SCREEN_HEIGHT - 14);
    
    if (currentMode == MODE_HELP) {
        M5.Display.print("ARROWS: Page | ESC: Exit");
    } else {
        M5.Display.print("Press 'H' for Commands");
    }
}

void updateHelpMode() {
    if (fullRedrawNeeded) {
        drawStaticHeader("MANUAL / HELP", DARKGREY);
        M5.Display.setTextColor(WHITE, BLACK);
        M5.Display.setTextSize(1.5); // Increased size for readability
        M5.Display.setCursor(5, 35);

        if (helpPage == 0) {
            // PAGE 1: NAVIGATION
            M5.Display.println("NAVIGATION:");
            M5.Display.println("");
            M5.Display.println(" [ESC]   Exit / Back");
            M5.Display.println(" [ARROWS] Scroll Pages");
            M5.Display.println("");
            M5.Display.println("Use '.' and '/' keys");
            M5.Display.println("as Arrows.");
        } 
        else if (helpPage == 1) {
            // PAGE 2: APP MODES
            M5.Display.println("APP MODES:");
            M5.Display.println("");
            M5.Display.println(" [G] GPS Monitor");
            M5.Display.println(" [L] LoRa Terminal");
            M5.Display.println(" [S] RSSI Sniffer");
            M5.Display.println("");
            M5.Display.println("Switch anytime.");
        }
        else if (helpPage == 2) {
            // PAGE 3: ACTIONS
            M5.Display.println("ACTIONS:");
            M5.Display.println("");
            M5.Display.println(" [TAB] Switch SF (7-12)");
            M5.Display.println(" [ENT] TX Geo-Beacon");
            M5.Display.println(" [SPC] TX Ping");
        }
        
        // Page Indicator
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
            M5.Display.setCursor(80, 90); M5.Display.print("SPD");
            M5.Display.setCursor(160, 90); M5.Display.print("SATS");
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
        
        M5.Display.fillRect(80, 102, 60, 16, BLACK);
        M5.Display.setCursor(80, 102); M5.Display.printf("%.0f", gps.speed.kmph());
        
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
        M5.Display.drawRect(0, 40, SCREEN_WIDTH, 50, WHITE);
        M5.Display.setTextColor(LIGHTGREY, BLACK);
        M5.Display.setTextSize(1.5);
        M5.Display.setCursor(5, 30); M5.Display.print("Last Packet:");
        fullRedrawNeeded = false;
        oldLoRaMessage = ""; 
    }

    if (lastLoRaMessage != oldLoRaMessage) {
        M5.Display.fillRect(2, 42, SCREEN_WIDTH-4, 46, BLACK);
        M5.Display.setTextColor(GREEN, BLACK);
        M5.Display.setTextSize(1.5);
        M5.Display.setCursor(5, 50);
        M5.Display.print(lastLoRaMessage);
        oldLoRaMessage = lastLoRaMessage; 
        
        M5.Display.setTextColor(WHITE, BLACK);
        M5.Display.setCursor(5, 95); 
        M5.Display.printf("RSSI: %.0f dBm   SNR: %.2f   ", lastRssi, lastSnr);
    }
}

void updateSnifferMode() {
    if (fullRedrawNeeded) {
        drawStaticHeader("LORA SPECTRUM", RED);
        sniffCursorX = 0;
        fullRedrawNeeded = false;
    }

    float rssi = radio.getRSSI();
    if (rssi < -130) rssi = -130;
    if (rssi > -40) rssi = -40;

    int graphBottom = SCREEN_HEIGHT - 20;
    int maxH = graphBottom - HEADER_HEIGHT;
    int h = map((int)rssi, -130, -40, 0, maxH);

    M5.Display.drawFastVLine(sniffCursorX, HEADER_HEIGHT, maxH, BLACK); 
    uint16_t color = BLUE;
    if (rssi > -95) color = GREEN;
    if (rssi > -75) color = YELLOW;
    if (rssi > -50) color = RED;
    
    M5.Display.drawFastVLine(sniffCursorX, graphBottom - h, h, color);
    M5.Display.drawFastVLine((sniffCursorX + 1) % SCREEN_WIDTH, HEADER_HEIGHT, maxH, WHITE);

    if (sniffCursorX % 20 == 0) {
        M5.Display.fillRect(170, 5, 60, 15, RED); 
        M5.Display.setTextColor(WHITE, RED);
        M5.Display.setCursor(175, 5);
        M5.Display.printf("%.0f", rssi);
    }

    sniffCursorX++;
    if (sniffCursorX >= SCREEN_WIDTH) sniffCursorX = 0;
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
    Serial.printf("\r\n\r\n>> GRANITICA %s - BOOTING <<\r\n", FW_VERSION);
    
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

    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        
        // EXIT / HOME (ESC or Backtick `)
        if (M5Cardputer.Keyboard.isKeyPressed(KEY_ESC) || M5Cardputer.Keyboard.isKeyPressed('`')) {
            currentMode = MODE_GPS;
            fullRedrawNeeded = true;
        }

        // HELP (H)
        if (M5Cardputer.Keyboard.isKeyPressed('h')) {
            if (currentMode != MODE_HELP) {
                currentMode = MODE_HELP;
                helpPage = 0;
                fullRedrawNeeded = true;
            }
        }

        // NAVIGATION (Used in Help)
        // Uses mapped keys [;] [.] [,] [/] for UP/DOWN/LEFT/RIGHT
        if (currentMode == MODE_HELP) {
             if (M5Cardputer.Keyboard.isKeyPressed('.') || M5Cardputer.Keyboard.isKeyPressed('/')) {
                 helpPage++;
                 if (helpPage >= MAX_HELP_PAGES) helpPage = 0;
                 fullRedrawNeeded = true;
             }
             if (M5Cardputer.Keyboard.isKeyPressed(';') || M5Cardputer.Keyboard.isKeyPressed(',')) {
                 helpPage--;
                 if (helpPage < 0) helpPage = MAX_HELP_PAGES - 1;
                 fullRedrawNeeded = true;
             }
        }

        // APP SWITCHING (Only if not in Help)
        if (currentMode != MODE_HELP) {
            if (M5Cardputer.Keyboard.isKeyPressed('g')) { currentMode = MODE_GPS; fullRedrawNeeded = true; }
            if (M5Cardputer.Keyboard.isKeyPressed('l')) { currentMode = MODE_LORA_TERM; fullRedrawNeeded = true; }
            if (M5Cardputer.Keyboard.isKeyPressed('s')) { currentMode = MODE_LORA_SNIFFER; fullRedrawNeeded = true; }
            
            if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) sendGeoBeacon();
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