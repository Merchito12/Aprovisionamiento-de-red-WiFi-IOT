#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

// ---- Access Point ----
const char* apSSID = "ESP32_Config";
const char* apPassword = "12345678";

// ---- Variables WiFi Station ----
String staSSID = "";
String staPassword = "";

// Preferencias (NVS) para guardar credenciales
Preferences preferences;

// Servidor web
WebServer server(80);

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Configurar WiFi</title></head><body>";
  html += "<h2>Estado del ESP32</h2>";

  if (WiFi.status() == WL_CONNECTED) {
    html += "<p>Conectado a la red: <b>" + staSSID + "</b></p>";
    html += "<p>IP asignada: " + WiFi.localIP().toString() + "</p>";
  } else {
    html += "<p> No conectado a ninguna red WiFi</p>";
  }

  html += "<hr><h3>Configurar nueva red</h3>";
  html += "<form action='/save' method='POST'>";
  html += "SSID: <input type='text' name='ssid'><br><br>";
  html += "Password: <input type='password' name='pass'><br><br>";
  html += "<input type='submit' value='Guardar'>";
  html += "</form>";

  html += "<hr><h3>Administrar</h3>";
  html += "<form action='/delete' method='POST'>";
  html += "<input type='submit' value='Borrar datos guardados'>";
  html += "</form>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleSave() {
  if (server.hasArg("ssid") && server.hasArg("pass")) {
    staSSID = server.arg("ssid");
    staPassword = server.arg("pass");

    // Guardar en NVS
    preferences.begin("wifi", false);
    preferences.putString("ssid", staSSID);
    preferences.putString("pass", staPassword);
    preferences.end();

    server.send(200, "text/html", "<h2> Datos guardados</h2><p>Reinicia el ESP32 para aplicar cambios.</p>");
    Serial.println("Credenciales guardadas: " + staSSID + " / " + staPassword);
  } else {
    server.send(400, "text/plain", "Faltan datos");
  }
}

void handleDelete() {
  preferences.begin("wifi", false);
  preferences.clear();  // Borra todos los datos guardados en "wifi"
  preferences.end();

  staSSID = "";
  staPassword = "";

  server.send(200, "text/html", "<h2> Datos borrados</h2><p>Reinicia el ESP32. Ahora solo funcionará como Access Point.</p>");
  Serial.println(" Credenciales borradas de la memoria.");
}

void setup() {
  Serial.begin(115200);

  // Leer credenciales guardadas
  preferences.begin("wifi", true);
  staSSID = preferences.getString("ssid", "");
  staPassword = preferences.getString("pass", "");
  preferences.end();

  // Modo dual: AP + STA
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apSSID, apPassword);
  Serial.print("AP iniciado. IP: ");
  Serial.println(WiFi.softAPIP());

  // Si hay credenciales guardadas, intentar conectarse
  if (staSSID != "") {
    WiFi.begin(staSSID.c_str(), staPassword.c_str());
    Serial.print("Conectando a ");
    Serial.println(staSSID);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n Conectado a Internet");
      Serial.print("IP asignada: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("\n No se pudo conectar a la red guardada.");
    }
  } else {
    Serial.println("ℹ No hay credenciales guardadas, solo AP activo.");
  }

  // Configurar servidor
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/delete", HTTP_POST, handleDelete);
  server.begin();
}

void loop() {
  server.handleClient();
}