#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <ESPmDNS.h> // Agrega esta línea para incluir la librería mDNS

#define PIN 17
int NUMPIXELS = 12; // Variable para almacenar el número de píxeles

CRGB leds[10000]; // Cambiado el tamaño del array para un máximo de 100 píxeles
WebServer server(80);

int currentEffect = 0;
CRGB currentColor = CRGB::Black;
int currentWait = 50;
bool effectChanged = false;

bool shouldSaveConfig = false;

void saveConfigCallback() {
  Serial.println("Configuración guardada");
  shouldSaveConfig = true;
}

void setup() {
    Serial.begin(115200);
    FastLED.addLeds<NEOPIXEL, PIN>(leds, NUMPIXELS);
    FastLED.clear();
    FastLED.show();

    // Inicializar WiFiManager
    WiFiManager wifiManager;

    // Agregar parámetros de configuración personalizados
    wifiManager.setSaveConfigCallback(saveConfigCallback);

    // Intentar conectar usando la configuración guardada
    if (!wifiManager.autoConnect("ESP32-AP")) {
        Serial.println("Error al conectar, reiniciando...");
        delay(3000);
        ESP.restart();
        delay(5000);
    }


    // Iniciar mDNS con el nombre de dominio "esp32"
    if (!MDNS.begin("esp32")) {
        Serial.println("Error iniciando mDNS");
    }
    Serial.println("mDNS iniciado");

    // Iniciar el servidor web
    setupServer();
}

void setupServer() {
    server.on("/color", HTTP_POST, []() {
        if (server.hasArg("plain") == false) {
            server.send(400, "text/plain", "Bad Request: Body not found");
            return;
        }

        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, server.arg("plain"));

        if (error) {
            server.send(400, "text/plain", "Bad Request: JSON parsing failed");
            return;
        }

        int r = doc["color"]["r"];
        int g = doc["color"]["g"];
        int b = doc["color"]["b"];
        int effect = doc["effect"];
        int wait = doc["wait"]; // Assuming interval is sent as part of the request

        applyEffect(effect, r, g, b, wait);
        server.send(200, "text/plain", "Color and effect applied");
    });

    // Nuevo endpoint para obtener la IP y el ID del dispositivo
    server.on("/info", HTTP_GET, []() {
        DynamicJsonDocument doc(256);
        doc["ip"] = WiFi.localIP().toString(); // IP como string
        doc["id"] = 2;         // MAC address como identificador único

        String response;
        serializeJson(doc, response);          // Serializa el JSON a string
        server.send(200, "application/json", response); // Envía la respuesta
    });

    server.on("/pixels", HTTP_POST, []() {
        if (server.hasArg("plain") == false) {
            server.send(400, "text/plain", "Bad Request: Body not found");
            return;
        }

        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, server.arg("plain"));

        if (error) {
            server.send(400, "text/plain", "Bad Request: JSON parsing failed");
            return;
        }

        int newNumPixels = doc["numPixels"];
        if (newNumPixels <= 0 || newNumPixels > 100000) { // Límite de 100 píxeles por seguridad
            server.send(400, "text/plain", "Bad Request: Invalid number of pixels");
            return;
        }

        NUMPIXELS = newNumPixels; // Actualiza el número de píxeles
        FastLED.addLeds<NEOPIXEL, PIN>(leds, NUMPIXELS); // Reconfigura los píxeles
        FastLED.clear();
        FastLED.show();

        server.send(200, "text/plain", "Number of pixels updated");
    });

    server.begin();
}

void loop() {
    server.handleClient();
    if (effectChanged || currentEffect != 0) {
        switch (currentEffect) {
            case 1:
                colorWipe(currentColor, currentWait);
                break;
            case 2:
                rainbowCycle(currentWait);
                break;
            case 3:
                fadeEffect();
                break;
            case 4:
                theaterChase(currentColor, currentWait);
                break;
            case 5:
                breathingEffect(currentWait);
                break;
            case 6:
                sparkle(currentColor, currentWait);
                break;
            case 7:
                colorWave(currentColor.r, currentColor.g, currentColor.b, currentWait);
                break;
            case 8:
                strobe(currentColor, currentWait);
                break;
            case 9:
                cycle(currentWait);
                break;
            case 10:
                chaseRainbow(currentWait);
                break;
            case 11:
                runningLights(currentColor, currentWait);
                break;
            case 12:
                waterfall(currentColor, currentWait);
                break;
            case 13:
                comet(currentColor, currentWait);
                break;
            default:
                setAllPixels(currentColor.r, currentColor.g, currentColor.b);
                break;
        }
        effectChanged = false;
    }
}

void applyEffect(int effectId, int r, int g, int b, int wait) {
    currentEffect = effectId;
    currentColor = CRGB(r, g, b);
    currentWait = wait;
    effectChanged = true;
}

void setAllPixels(int r, int g, int b) {
    fill_solid(leds, NUMPIXELS, CRGB(r, g, b));
    FastLED.show();
}

void colorWipe(CRGB color, int wait) {
    static int position = 0;
    leds[position] = color;
    FastLED.show();
    position = (position + 1) % NUMPIXELS;
    delay(wait);
}

void rainbowCycle(int wait) {
    static uint16_t firstColor = 0;
    for (int i = 0; i < NUMPIXELS; i++) {
        leds[i] = Wheel(((i * 256 / NUMPIXELS) + firstColor) & 255);
    }
    FastLED.show();
    firstColor++;
    delay(wait);
}

CRGB Wheel(byte WheelPos) {
    WheelPos = 255 - WheelPos;
    if (WheelPos < 85) {
        return CRGB(255 - WheelPos * 3, 0, WheelPos * 3);
    } else if (WheelPos < 170) {
        WheelPos -= 85;
        return CRGB(0, WheelPos * 3, 255 - WheelPos * 3);
    } else {
        WheelPos -= 170;
        return CRGB(WheelPos * 3, 255 - WheelPos * 3, 0);
    }
}

void fadeEffect() {
    static int brightness = 0;
    static int fadeAmount = 5;
    brightness = brightness + fadeAmount;
    if (brightness <= 0 || brightness >= 255) {
        fadeAmount = -fadeAmount;
    }
    FastLED.setBrightness(brightness);
    FastLED.show();
    delay(50); // Adjust as needed
}

void theaterChase(CRGB color, int wait) {
    static int position = 0;
    for (int a = 0; a < 10; a++) {
        for (int b = 0; b < 3; b++) {
            fill_solid(leds, NUMPIXELS, CRGB::Black);
            for (int c = b; c < NUMPIXELS; c += 3) {
                leds[(position + c) % NUMPIXELS] = color;
            }
            FastLED.show();
            delay(wait);
        }
    }
    position = (position + 1) % NUMPIXELS;
}

void breathingEffect(int wait) {
    static int brightness = 0;
    static int fadeAmount = 5;
    brightness = brightness + fadeAmount;
    if (brightness <= 0 || brightness >= 255) {
        fadeAmount = -fadeAmount;
    }
    FastLED.setBrightness(brightness);
    FastLED.show();
    delay(wait); // Adjust as needed
}

void sparkle(CRGB color, int wait) {
    static int lastPixel = -1;
    int pixel = random(NUMPIXELS);
    if (lastPixel >= 0) {
        leds[lastPixel] = CRGB::Black;
    }
    leds[pixel] = color;
    FastLED.show();
    lastPixel = pixel;
    delay(wait); // Adjust as needed
}

void colorWave(int r, int g, int b, int wait) {
    static int wavePosition = 0;
    wavePosition = (wavePosition + 1) % NUMPIXELS;
    for (int i = 0; i < NUMPIXELS; i++) {
        leds[i] = CRGB((r * (NUMPIXELS - abs(i - wavePosition))) / NUMPIXELS, (g * (NUMPIXELS - abs(i - wavePosition))) / NUMPIXELS, (b * (NUMPIXELS - abs(i - wavePosition))) / NUMPIXELS);
    }
    FastLED.show();
    delay(wait); // Adjust as needed
}




void strobe(CRGB color, int wait) {
    fill_solid(leds, NUMPIXELS, color);
    FastLED.show();
    delay(wait); // Adjust as needed
    fill_solid(leds, NUMPIXELS, CRGB::Black);
    FastLED.show();
    delay(wait); // Adjust as needed
}

void cycle(int wait) {
    static int colorIndex = 0;
    fill_solid(leds, NUMPIXELS, (colorIndex == 0) ? CRGB(255, 0, 0) : (colorIndex == 1) ? CRGB(0, 255, 0) : CRGB(0, 0, 255));
    FastLED.show();
    colorIndex = (colorIndex + 1) % 3;
    delay(wait); // Adjust as needed
}

void chaseRainbow(int wait) {
    static uint16_t firstColor = 0;
    for (int i = 0; i < NUMPIXELS; i++) {
        leds[i] = Wheel((i + firstColor) & 255);
    }
    FastLED.show();
    firstColor++;
    delay(wait); // Adjust as needed
}

void runningLights(CRGB color, int wait) {
    static int position = 0;
    fill_solid(leds, NUMPIXELS, CRGB::Black);
    leds[position] = color;
    FastLED.show();
    position = (position + 1) % NUMPIXELS;
    delay(wait); // Adjust as needed
}



void waterfall(CRGB color, int wait) {
    for (int i = NUMPIXELS - 1; i >= 0; i--) {
        leds[i] = color;
        FastLED.show();
        delay(wait); // Adjust as needed
        if (i < NUMPIXELS - 1) {
            leds[i + 1] = CRGB::Black;
            FastLED.show();
        }
    }
}

void comet(CRGB color, int wait) {
    for (int i = 0; i < NUMPIXELS + 10; i++) {
        fill_solid(leds, NUMPIXELS, CRGB::Black);
        for (int j = 0; j < 10; j++) {
            if (i - j < NUMPIXELS && i - j >= 0) {
                leds[i - j] = dimColor(color, j * 25);
            }
        }
        FastLED.show();
        delay(wait); // Adjust as needed
    }
}

CRGB dimColor(CRGB color, int dimBy) {
    return CRGB(dimBy * color.r / 255, dimBy * color.g / 255, dimBy * color.b / 255);
}
