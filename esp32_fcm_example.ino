#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP_Signer.h>

// Konfigurasi WiFi dan credential OAuth
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define PROJECT_ID "YOUR_PROJECT_ID"
#define CLIENT_EMAIL "YOUR_CLIENT_EMAIL"
const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY----------END CERTIFICATE-----\n";

SignerConfig config;

void tokenStatusCallback(TokenInfo info);

void setup()
{
    Serial.begin(115200);
    Serial.println();
    Serial.println();

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    config.service_account.data.client_email = CLIENT_EMAIL;
    config.service_account.data.project_id = PROJECT_ID;
    config.service_account.data.private_key = PRIVATE_KEY;

    config.signer.expiredSeconds = 3600;
    config.signer.preRefreshSeconds = 60;

    config.signer.tokens.scope = "https://www.googleapis.com/auth/firebase.messaging";

    config.token_status_callback = tokenStatusCallback;

    Signer.begin(&config);
}

void loop()
{
    bool ready = Signer.tokenReady();
    if (ready)
    {
        String accessToken = Signer.accessToken();

        // Your FCM notification sending logic here
        sendNotificationToSelectedDriver(accessToken);

        // Delay or other logic as needed
        delay(5000);
    }
}

void sendNotificationToSelectedDriver(const String &accessToken)
{
    WiFiClientSecure client;
    HTTPClient http;

    client.setInsecure(); // Skip certificate validation for testing, not recommended for production

    http.begin(client, "https://fcm.googleapis.com/v1/projects/dev-smart-garden/messages:send");
    
    http.addHeader("Authorization", "Bearer " + accessToken);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("User-Agent", "PostmanRuntime/7.40.0");
    http.addHeader("Accept", "*/*");
    http.addHeader("Cache-Control", "no-cache");
    http.addHeader("Postman-Token", "f8c537d9-8db0-4839-81c4-0740d00535a4");
    http.addHeader("Host", "fcm.googleapis.com");
    http.addHeader("Accept-Encoding", "gzip, deflate, br");
    http.addHeader("Connection", "keep-alive");
    http.addHeader("Content-Length", "290"); // Sesuaikan dengan panjang konten yang sesungguhnya

    StaticJsonDocument<256> message;
    message["message"]["topic"] = "smart_plant";
    message["message"]["notification"]["title"] = "Smart Garden";
    message["message"]["notification"]["body"] = "test notif";
    message["message"]["android"]["notification"]["icon"] = "notification_icon";
    message["message"]["data"]["tripId"] = "your_trip_id_here"; // Isi dengan data trip ID yang sesuai

    String messageString;
    serializeJson(message, messageString);

    Serial.println("Payload JSON:");
    Serial.println(messageString);

    int httpResponseCode = http.POST(messageString);

    if (httpResponseCode == 200)
    {
        Serial.println("FCM Notification sent successfully");
    }
    else
    {
        Serial.print("Failed to send notification, ");
        Serial.println(httpResponseCode);
    }

    http.end();
}

void tokenStatusCallback(TokenInfo info)
{
    if (info.status == esp_signer_token_status_error)
    {
        Signer.printf("Token info: type = %s, status = %s\n", Signer.getTokenType(info).c_str(), Signer.getTokenStatus(info).c_str());
        Signer.printf("Token error: %s\n", Signer.getTokenError(info).c_str());
    }
    else
    {
        Signer.printf("Token info: type = %s, status = %s\n", Signer.getTokenType(info).c_str(), Signer.getTokenStatus(info).c_str());
        if (info.status == esp_signer_token_status_ready)
            Signer.printf("Token: %s\n", Signer.accessToken().c_str());
    }
}