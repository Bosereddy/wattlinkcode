#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

char auth[] = "YourBlynkAuthToken";
char ssid[] = "YourWiFiSSID";
char pass[] = "YourWiFiPassword";

#define NUM_APPLIANCES 5

int sensorPins[NUM_APPLIANCES] = {A0, A1, A2, A3, A4};

const float VOLTAGE = 230.0;
const float TARIFF = 6.5;
const float CURRENT_THRESHOLD = 0.1;
const float POWER_LIMIT = 10.0;

unsigned long onTime[NUM_APPLIANCES] = {0};
float energy[NUM_APPLIANCES] = {0};
bool lastState[NUM_APPLIANCES] = {false};
unsigned long lastUpdateTime = 0;
bool notificationSent = false;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000);

void setup() {
    Serial.begin(115200);
    Blynk.begin(auth, ssid, pass);
    timeClient.begin();
}

void loop() {
    Blynk.run();
    timeClient.update();

    float totalEnergy = 0;
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - lastUpdateTime;

    for (int i = 0; i < NUM_APPLIANCES; i++) {
        float current = analogRead(sensorPins[i]) * 0.0264;
        bool state = (current > CURRENT_THRESHOLD);

        if (state) {
            onTime[i] += elapsedTime;
            float power = VOLTAGE * current;
            energy[i] += (power * (elapsedTime / 3600000.0)) / 1000;
        }

        lastState[i] = state;
        Blynk.virtualWrite(V1 + i, energy[i]);
    }

    lastUpdateTime = millis();
    float totalCost = totalEnergy * TARIFF;
    Blynk.virtualWrite(V10, totalCost);

    if (totalEnergy > POWER_LIMIT && !notificationSent) {
        String warningMessage = "⚠️ Power Limit Exceeded! Current consumption: " + String(totalEnergy) + " kWh";
        Blynk.notify(warningMessage);
        notificationSent = true;
    }

    if (timeClient.getHours() == 21 && timeClient.getMinutes() == 0 && !notificationSent) {
        String message = "🔔 Daily Power Bill: ₹" + String(totalCost) + "\n";
        for (int i = 0; i < NUM_APPLIANCES; i++) {
            message += "Appliance " + String(i + 1) + " - On-Time: " + String(onTime[i] / 3600000.0) + " hrs\n";
        }
        Blynk.notify(message);
        
        for (int i = 0; i < NUM_APPLIANCES; i++) {
            onTime[i] = 0;
            energy[i] = 0;
        }
        notificationSent = true;
    }

    if (timeClient.getHours() == 0 && timeClient.getMinutes() == 0) {
        notificationSent = false;
    }

    delay(60000);
}
