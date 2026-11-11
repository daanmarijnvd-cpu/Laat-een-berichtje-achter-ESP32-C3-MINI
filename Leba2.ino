#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <LittleFS.h>

const char* apSSID = "Laat een berichtje achter!";
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

DNSServer dnsServer;
WebServer server(80);

const char* messagesFile = "/messages.txt";
const String adminPassword = "admin";

// Lijst met verboden scheldwoorden
const char* badWords[] = {
  "kut", "tering", "godverde", "kolere", "shit", "fuck", "bitch", "hufter", "kanker", "neger", "niga"
};
const int numBadWords = 11;

String getUptime() {
  unsigned long ms = millis();
  int secs = (ms / 1000) % 60;
  int mins = (ms / (1000 * 60)) % 60;
  int hrs = (ms / (1000 * 60 * 60)) % 24;
  int days = ms / (1000 * 60 * 60 * 24);
  
  String res = "";
  if (days > 0) res += String(days) + "d ";
  res += String(hrs) + "u " + String(mins) + "m " + String(secs) + "s";
  return res;
}

String filterBadWords(String text) {
  String lowerText = text;
  lowerText.toLowerCase();

  for (int i = 0; i < numBadWords; i++) {
    String word = badWords[i];
    int pos = lowerText.indexOf(word);
    while (pos != -1) {
      String stars = "";
      for (unsigned int j = 0; j < word.length(); j++) stars += "*";
      
      text = text.substring(0, pos) + stars + text.substring(pos + word.length());
      lowerText = text;
      lowerText.toLowerCase();
      pos = lowerText.indexOf(word, pos + stars.length());
    }
  }
  return text;
}

String renderMessages(bool isAdmin) {
  if (!LittleFS.exists(messagesFile)) {
    return "<p style='color:#777; text-align:center;'><i>Nog geen berichten geplaatst. Wees de eerste!</i></p>";
  }

  File f = LittleFS.open(messagesFile, "r");
  if (!f) return "<p>Fout bij het laden van berichten.</p>";

  String htmlList = "";
  int index = 0;
  
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    int separator1 = line.indexOf('|');
    int separator2 = line.indexOf('|', separator1 + 1);

    if (separator1 != -1 && separator2 != -1) {
      String name = line.substring(0, separator1);
      String time = line.substring(separator1 + 1, separator2);
      String msg = line.substring(separator2 + 1);

      htmlList += "<div class='post'>";
      htmlList += "<div class='meta'><span class='author'>" + name + "</span> • " + time + "</div>";
      htmlList += "<div>" + msg + "</div>";
      
      if (isAdmin) {
        htmlList += "<div style='margin-top:6px; text-align:right;'>";
        htmlList += "<a href='/delete?id=" + String(index) + "&admin=" + adminPassword + "' style='color:#d9534f; font-size:12px; font-weight:bold; text-decoration:none;'>[Verwijder bericht]</a>";
        htmlList += "</div>";
      }
      
      htmlList += "</div>";
      index++;
    }
  }
  f.close();

  if (htmlList == "") {
    return "<p style='color:#777; text-align:center;'><i>Nog geen berichten geplaatst.</i></p>";
  }
  return htmlList;
}

void handleRoot() {
  bool isAdmin = server.hasArg("admin") && server.arg("admin") == adminPassword;

  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Laat een berichtje achter!</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; background: #eef2f5; margin: 0; padding: 15px; }";
  html += ".card { max-width: 480px; margin: 0 auto; background: white; padding: 20px; border-radius: 12px; box-shadow: 0 4px 12px rgba(0,0,0,0.1); }";
  html += "h2 { margin-top: 0; margin-bottom: 8px; color: #2c3e50; text-align: center; }";
  html += ".browser-tip { background: #e8f4fd; border: 1px solid #b6d4fe; padding: 10px; border-radius: 8px; text-align: center; margin-bottom: 16px; font-size: 13px; color: #084298; }";
  html += ".btn-browser { display: inline-block; margin-top: 6px; padding: 7px 14px; background: #0d6efd; color: white; text-decoration: none; border-radius: 5px; font-weight: bold; font-size: 13px; }";
  html += "input[type='text'], textarea, input[type='password'] { width: 100%; box-sizing: border-box; padding: 10px; margin-bottom: 12px; border: 1px solid #ccc; border-radius: 6px; font-size: 14px; }";
  html += "textarea { height: 70px; resize: vertical; }";
  html += "input[type='submit'] { width: 100%; background: #28a745; color: white; border: none; padding: 12px; border-radius: 6px; cursor: pointer; font-size: 16px; font-weight: bold; }";
  html += "input[type='submit']:hover { background: #218838; }";
  html += ".post { background: #f8f9fa; border-left: 4px solid #007bff; padding: 10px 12px; margin-bottom: 10px; border-radius: 4px; word-wrap: break-word; }";
  html += ".meta { font-size: 11px; color: #888; margin-bottom: 4px; }";
  html += ".author { font-weight: bold; color: #333; }";
  html += ".admin-box { margin-top: 25px; padding-top: 15px; border-top: 1px solid #eee; font-size: 13px; text-align: center; }";
  html += "</style></head><body>";
  
  html += "<div class='card'>";
  html += "<h2>📌 ESP32 Prikbord</h2>";
  
  // Aangepaste instructie voor captive portal (ingedrukt houden)
  html += "<div class='browser-tip'>";
  html += "Werkt het plaatsen niet goed in dit pop-up scherm?<br>";
  html += "<b>Houd de knop hieronder ingedrukt</b> en kies <i>'Open in nieuw tabblad'</i> of open handmatig <b>192.168.4.1</b> in je browser.<br>";
  html += "<a href='http://192.168.4.1' target='_blank' class='btn-browser'>🌐 Open in je browser</a>";
  html += "</div>";
  
  html += "<form action='/post' method='POST'>";
  html += "<input type='text' name='name' placeholder='Je naam of alias' maxlength='25' required>";
  html += "<textarea name='msg' placeholder='Laat een bericht achter...' maxlength='250' required></textarea>";
  html += "<input type='submit' value='Bericht Plaatsen'>";
  html += "</form>";
  
  html += "<hr style='margin: 20px 0; border: 0; border-top: 1px solid #eee;'>";
  html += "<h3>Recente Berichten:</h3>";
  
  html += renderMessages(isAdmin);

  html += "<div class='admin-box'>";
  if (isAdmin) {
    html += "<p style='color:green; font-weight:bold;'>Beheerder ingelogd</p>";
    html += "<a href='/' style='color:#007bff; text-decoration:none; display:inline-block; margin-bottom:10px;'>Uitloggen</a><br>";
    html += "<a href='/clear?admin=" + adminPassword + "' style='color:red; text-decoration:none;'>[Wis het hele prikbord]</a>";
  } else {
    html += "<form action='/' method='GET'>";
    html += "<input type='password' name='admin' placeholder='Beheerderswachtwoord' style='width:68%; display:inline-block; margin-bottom:0;'>";
    html += "<input type='submit' value='Login' style='width:28%; display:inline-block; padding:10px; font-size:14px; background:#6c757d;'>";
    html += "</form>";
  }
  html += "</div>";

  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

void handlePost() {
  if (server.hasArg("name") && server.hasArg("msg")) {
    String name = server.arg("name");
    String msg = server.arg("msg");
    
    name.replace("|", "/");
    msg.replace("|", "/");
    name.replace("<", "&lt;"); name.replace(">", "&gt;");
    msg.replace("<", "&lt;"); msg.replace(">", "&gt;");
    name.replace("\r", "");
    name.replace("\n", " ");
    msg.replace("\r", "");
    msg.replace("\n", "<br>");

    name = filterBadWords(name);
    msg = filterBadWords(msg);

    String timeStr = getUptime() + " sinds boot";
    String newEntry = name + "|" + timeStr + "|" + msg + "\n";

    String allContent = "";
    if (LittleFS.exists(messagesFile)) {
      File f = LittleFS.open(messagesFile, "r");
      if (f) {
        allContent = f.readString();
        f.close();
      }
    }

    File f = LittleFS.open(messagesFile, "w");
    if (f) {
      f.print(newEntry + allContent);
      f.close();
    }
  }
  
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void handleDelete() {
  if (server.hasArg("admin") && server.arg("admin") == adminPassword && server.hasArg("id")) {
    int targetId = server.arg("id").toInt();
    
    if (LittleFS.exists(messagesFile)) {
      File f = LittleFS.open(messagesFile, "r");
      String newContent = "";
      int currentIndex = 0;

      while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() > 0) {
          if (currentIndex != targetId) {
            newContent += line + "\n";
          }
          currentIndex++;
        }
      }
      f.close();

      File fWrite = LittleFS.open(messagesFile, "w");
      if (fWrite) {
        fWrite.print(newContent);
        fWrite.close();
      }
    }
    server.sendHeader("Location", "/?admin=" + adminPassword, true);
    server.send(302, "text/plain", "");
    return;
  }
  
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void handleClear() {
  if (server.hasArg("admin") && server.arg("admin") == adminPassword) {
    if (LittleFS.exists(messagesFile)) {
      LittleFS.remove(messagesFile);
    }
    server.sendHeader("Location", "/?admin=" + adminPassword, true);
    server.send(302, "text/plain", "");
    return;
  }
  
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void setup() {
  Serial.begin(115200);

  if (!LittleFS.begin(true)) {
    Serial.println("Fout bij starten LittleFS");
    return;
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(apSSID);

  dnsServer.start(DNS_PORT, "*", apIP);

  server.on("/", handleRoot);
  server.on("/post", HTTP_POST, handlePost);
  server.on("/delete", handleDelete);
  server.on("/clear", handleClear);
  server.onNotFound(handleRoot);

  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}
