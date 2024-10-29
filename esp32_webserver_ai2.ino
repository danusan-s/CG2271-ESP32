// Load Wi-Fi library
#include <WiFi.h>

#define RXD2 16
#define TXD2 17

// Replace with your network credentials
const char* ssid = "S20_FE";
const char* password = "Danu@8125";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String response, ip_address;

// Auxiliar variables to store the current output state
String output26State = "off";

// Assign output variables to GPIO pins
const int output26 = 26;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;
int wait30 = 30000; // time to reconnect when connection is lost.

int x = 0, y = 0;

// This is your Static IP
IPAddress local_IP(192, 168, 27, 189);
// Gateway IP address
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);
// Use google DNS
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

String generate_page() {
    String html = "<!DOCTYPE html>";
    html += "<html>";
    html += "<head>";
    html += "<title>ESP32 Robot Control</title>";

    // Add style
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; text-align: center; padding: 50px; }";
    html += "h1 { color: #333; }";
    html += ".row {";
    html += "  display: flex;";
    html += "  justify-content: center;";
    html += "  align-items: center;";
    html += "  gap: 20px;";
    html += "  margin-top: 50px;";
    html += "}";

    html += "button {";
    html += "  padding: 20px;";
    html += "  font-size: 18px;";
    html += "  border: none;";
    html += "  background-color: #4CAF50;";
    html += "  color: white;";
    html += "  cursor: pointer;";
    html += "  width: 200px;";
    html += "  height: 50px;";
    html += "  text-align: center;";
    html += "}";
    html += "button:hover { background-color: #45a049; }";
    html += "</style>";

    // Add JavaScript
    html += "<script>";
    html += "let pressed = new Set();";

    // Function to send the command and update the status
    html += "function sendCommand(command) {";
    html += "  fetch(command).then(response => response.text()).then(text => {";
    html += "    document.getElementById('status').innerText = text;"; // Update status
    html += "  });";
    html += "}";

    // Key press handling
    html += "window.addEventListener('keydown', function(e) {";
    html += "  if (pressed.has(e.code)) return;";
    html += "  pressed.add(e.code);";

    html += "  switch(e.key) {";
    html += "    case 'w': sendCommand('/forward'); break;";
    html += "    case 's': sendCommand('/backward'); break;";
    html += "    case 'a': sendCommand('/left'); break;";
    html += "    case 'd': sendCommand('/right'); break;";
    html += "    case 'p': sendCommand('/play'); break;";
    html += "    default: pressed.delete(e.code); break;";
    html += "  }";
    html += "});";

    // Key release handling
    html += "window.addEventListener('keyup', function(e) {";
    html += "  pressed.delete(e.code);";
    html += "  switch(e.key) {";
    html += "    case 'w': sendCommand('/backward'); break;";
    html += "    case 's': sendCommand('/forward'); break;";
    html += "    case 'a': sendCommand('/right'); break;";
    html += "    case 'd': sendCommand('/left'); break;";
    html += "  }";
    html += "  if (pressed.size === 0) sendCommand('/stop');";
    html += "});";
    html += "</script>";

    html += "</head>";
    html += "<body>";
    html += "<h1>ESP32 Robot Control</h1>";

    html += "<div class='row'>";
    html += "<button>Forward : W</button>";
    html += "<button>Left : A</button>";
    html += "<button>Right : D</button>";
    html += "<button>Backward : S</button>";
    html += "<button>Play Speaker : P</button>";
    html += "</div>";

    // Status paragraph
    html += "<p id='status'>WAITING FOR COMMAND.</p>";

    html += "</body>";
    html += "</html>";

    return html;
}

void setup() {
    Serial.begin(115200);
    Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
    // Initialize the output variables as outputs
    pinMode(output26, OUTPUT);
    // Set outputs to LOW
    digitalWrite(output26, LOW);

    // Configure Static IP
    // if(!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
    //{
    //   Serial.println("Static IP failed to configure");
    // }

    // Connect to Wi-Fi network with SSID and password
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    // Print local IP address and start web server
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    ip_address = WiFi.localIP().toString();
    Serial.println(ip_address);
    server.begin();
}

void loop() {
    // If disconnected, try to reconnect every 30 seconds.
    if ((WiFi.status() != WL_CONNECTED) && (millis() > wait30)) {
        WiFi.disconnect();
        WiFi.begin(ssid, password);
        wait30 = millis() + 30000;
    }

    WiFiClient client = server.available();
    if (!client) {
        return;
    }

    String req = client.readStringUntil('\r');
    client.flush(); // Flush to avoid sending extra data

    uint8_t prev_command = 0;

    // Handle request
    if (req.indexOf("GET / ") != -1) {
        String html_response = generate_page();
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println("");
        client.println(html_response); // Serve the HTML once
    } else {
        String command_response = "Inputs cancel out";
        uint8_t command = 0;

        if (req.indexOf("/forward") != -1) {
            y += 1;
        } else if (req.indexOf("/backward") != -1) {
            y -= 1;
        } else if (req.indexOf("/left") != -1) {
            x -= 1;
        } else if (req.indexOf("/right") != -1) {
            x += 1;
        } else if (req.indexOf("/play") != -1) {
            x = 0;
            y = 0; // Quick reset to avoid bugs
            command_response = "PLAYING MUSIC";
            command = 9;
        } else if (req.indexOf("/stop") != -1) {
            x = 0;
            y = 0; // Quick reset to avoid bugs
            command_response = "WAITING FOR COMMAND";
        }

        if (x == 0) {
            if (y > 0) {
                command = 1;
            } else if (y < 0) {
                command = 2;
            }
        } else if (y == 0) {
            if (x > 0) {
                command = 4;
            } else if (x < 0) {
                command = 3;
            }
        } else if (x > 0) {
            if (y > 0) {
                command = 6;
            } else if (y < 0) {
                command = 5;
            }
        } else if (x < 0) {
            if (y > 0) {
                command = 8;
            } else if (y < 0) {
                command = 7;
            }
        }

        if (command!=prev_command) {
            Serial2.write(command);
            prev_command = command;
        }

        if (x || y) {
            command_response = String("X: " + String(x) + " Y: " + String(y));
        }

        // Send back only the response text, not the whole page
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("");
        client.println(command_response); // Only the command response
    }

    client.stop();
}
