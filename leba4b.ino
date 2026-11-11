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
const char* tmpFile = "/messages.tmp";
const String adminPassword = "3D7dgh";

// Ingebouwd ledje van de ESP32-C3 mini.
const int LED_PIN = 8;
const int LED_ON = LOW;
const int LED_OFF = HIGH;
const unsigned long LED_BLINK_MS = 200;

bool ledActive = false;
unsigned long ledOffAt = 0;

// Lijst met verboden scheldwoorden
const char* badWords[] = {
"kut","tering","godverde","kolere","shit","fuck","bitch","hufter","kanker","neger","niga","zwarte","lul","godverdome","hitler","jeffrey","epstein","diddy","kanker","k@nker","k4nker", "kk","tering","t3ring",
"tyfus","pokken","godverdomme","godver","godsamme","verdomme","godskolere","klootzak","klootviool","kloot","eikel","lul","lulletje","sukkel","idioot","debiel","achterlijk","mongool","hoerenzoon","hoer","hoerig","kankerlijer",
"teringlijer","tyfuslijer","kut","kutwijf","kuthoer","kankerhoer","teringzooi","wijf","trut","muts","flikker","nicht","homo","pisnicht","sodemieter","sodemieteren","opkankeren","optyfen","oprotten","rot op","sodemieter op",
"pleur op","zak","zakkenwasser","lulhannes","hufter","etter","smeerlap","vuile hond","fuck","fucking","fucker","motherfucker","shit","bullshit","asshole","dickhead","bastard","bitch","son of a bitch","whore","slut",
"cunt","twat","prick","wanker","piss off","damn","dammit","dumbass","jackass","dick","faggot","nigger","nigga","retard","dipshit","f*ck","f@ck","f4ck","sh1t","sh!t","b!tch","a$$hole","d1ck","b@stard",
"k4nker","k@nker","g0dverdomme","g0dver"
};
const int numBadWords = sizeof(badWords) / sizeof(badWords[0]);

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

void startLedBlink() {
  digitalWrite(LED_PIN, LED_ON);
  ledOffAt = millis() + LED_BLINK_MS;
  ledActive = true;
}

void updateLed() {
  if (ledActive && (long)(millis() - ledOffAt) >= 0) {
    digitalWrite(LED_PIN, LED_OFF);
    ledActive = false;
  }
}

void streamMessages(bool isAdmin) {
  if (!LittleFS.exists(messagesFile)) {
    server.sendContent(F("<p style='color:#777; text-align:center;'><i>Nog geen berichten geplaatst. Wees de eerste!</i></p>"));
    return;
  }

  File f = LittleFS.open(messagesFile, "r");
  if (!f) {
    server.sendContent(F("<p>Fout bij het laden van berichten.</p>"));
    return;
  }

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

      String post = "<div class='post'>";
      post += "<div class='meta'><span class='author'>" + name + "</span> • " + time + "</div>";
      post += "<div>" + msg + "</div>";

      if (isAdmin) {
        post += "<div style='margin-top:6px; text-align:right;'>";
        post += "<a href='/delete?id=" + String(index) + "&admin=" + adminPassword + "' style='color:#d9534f; font-size:12px; font-weight:bold; text-decoration:none;'>[Verwijder bericht]</a>";
        post += "</div>";
      }
      post += "</div>";

      server.sendContent(post);
      index++;
    }
  }
  f.close();

  if (index == 0) {
    server.sendContent(F("<p style='color:#777; text-align:center;'><i>Nog geen berichten geplaatst.</i></p>"));
  }
}

void handleRoot() {
  bool isAdmin = server.hasArg("admin") && server.arg("admin") == adminPassword;

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  server.sendContent(F("<!DOCTYPE html><html><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
    "<title>Laat een berichtje achter!</title><style>"
    "body { font-family: Arial, sans-serif; background: #eef2f5; margin: 0; padding: 15px; }"
    ".card { max-width: 480px; margin: 0 auto; background: white; padding: 20px; border-radius: 12px; box-shadow: 0 4px 12px rgba(0,0,0,0.1); }"
    "h2 { margin-top: 0; margin-bottom: 8px; color: #2c3e50; text-align: center; }"
    ".section-header { display: flex; justify-content: space-between; align-items: center; margin-top: 20px; margin-bottom: 10px; }"
    ".section-header h3 { margin: 0; color: #2c3e50; font-size: 16px; }"
    ".uptime-badge { font-size: 11px; color: #888; background: #f1f3f5; padding: 3px 7px; border-radius: 6px; }"
    "input[type='text'], textarea, input[type='password'] { width: 100%; box-sizing: border-box; padding: 10px; margin-bottom: 12px; border: 1px solid #ccc; border-radius: 6px; font-size: 14px; }"
    "textarea { height: 70px; resize: vertical; }"
    "input[type='submit'] { width: 100%; background: #28a745; color: white; border: none; padding: 12px; border-radius: 6px; cursor: pointer; font-size: 16px; font-weight: bold; }"
    "input[type='submit']:hover { background: #218838; }"
    ".post { background: #f8f9fa; border-left: 4px solid #007bff; padding: 10px 12px; margin-bottom: 10px; border-radius: 4px; word-wrap: break-word; }"
    ".meta { font-size: 11px; color: #888; margin-bottom: 4px; }"
    ".author { font-weight: bold; color: #333; }"
    ".admin-box { margin-top: 25px; padding-top: 15px; border-top: 1px solid #eee; font-size: 13px; text-align: center; }"
    "</style></head><body>"));

  server.sendContent(F("<div class='card'>"
    "<h2>Laat een berichtje achter!</h2>"
    "<form action='/post' method='POST'>"
    "<input type='text' name='name' placeholder='Je naam of alias' maxlength='25' required>"
    "<textarea name='msg' placeholder='Laat een bericht achter...' maxlength='250' required></textarea>"
    "<input type='submit' value='Bericht Plaatsen'>"
    "</form>"
    "<hr style='margin: 20px 0; border: 0; border-top: 1px solid #eee;'>"
    "<div class='section-header'><h3>Recente Berichten:</h3>"));
  
  server.sendContent("<div class='uptime-badge'>Actief: " + getUptime() + "</div>");
  server.sendContent(F("</div>"));

  streamMessages(isAdmin);

  server.sendContent(F("<div class='admin-box'>"));
  if (isAdmin) {
    server.sendContent(F("<p style='color:green; font-weight:bold;'>Beheerder ingelogd</p>"
      "<a href='/' style='color:#007bff; text-decoration:none; display:inline-block; margin-bottom:10px;'>Uitloggen</a><br>"));
    server.sendContent("<a href='/clear?admin=" + adminPassword + "' style='color:red; text-decoration:none;'>[Wis het hele prikbord]</a>");
  } else {
    server.sendContent(F("<form action='/' method='GET'>"
      "<input type='password' name='admin' placeholder='Beheerderswachtwoord' style='width:68%; display:inline-block; margin-bottom:0;'>"
      "<input type='submit' value='Login' style='width:28%; display:inline-block; padding:10px; font-size:14px; background:#6c757d;'>"
      "</form>"));
  }
  server.sendContent(F("</div></div></body></html>"));

  server.sendContent("");
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

    File tmp = LittleFS.open(tmpFile, "w");
    if (tmp) {
      tmp.print(newEntry);

      if (LittleFS.exists(messagesFile)) {
        File old = LittleFS.open(messagesFile, "r");
        if (old) {
          uint8_t buf[256];
          while (old.available()) {
            size_t n = old.read(buf, sizeof(buf));
            if (n == 0) break;
            tmp.write(buf, n);
          }
          old.close();
        }
      }
      tmp.close();

      LittleFS.remove(messagesFile);
      LittleFS.rename(tmpFile, messagesFile);

      startLedBlink();
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
      File tmp = LittleFS.open(tmpFile, "w");

      if (f && tmp) {
        int currentIndex = 0;

        while (f.available()) {
          String line = f.readStringUntil('\n');
          line.trim();
          if (line.length() > 0) {
            if (currentIndex != targetId) {
              tmp.print(line + "\n");
            }
            currentIndex++;
          }
        }
        f.close();
        tmp.close();

        LittleFS.remove(messagesFile);
        LittleFS.rename(tmpFile, messagesFile);
      } else {
        if (f) f.close();
        if (tmp) tmp.close();
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
  delay(1000);

  // Zet de CPU-frequentie op 80 MHz (standaard is 160 MHz)
  setCpuFrequencyMhz(80);

  Serial.print("Huidige CPU Frequentie: ");
  Serial.print(getCpuFrequencyMhz());
  Serial.println(" MHz");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LED_OFF);

  if (!LittleFS.begin(true)) {
    Serial.println("Fout bij starten LittleFS");
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
  updateLed();
}