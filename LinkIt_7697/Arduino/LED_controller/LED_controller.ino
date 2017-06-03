#include <LWiFi.h>
#include <HttpClient.h>

char ssid[] = "mcs";     //  your network SSID (name)
char pass[] = "mcs123";  // your network password
char deviceId[] = "*****"; // your test device ID
char deviceKey[] = "*****"; // your test device key
char server[] = "192.168.1.100"; // The IP address of your computer which the MCS Lite is running on
int ws_port = 8000;  // The Web Socket port whic the MCS Lite is using
int rs_port = 3000;  // The Restful API port whic the MCS Lite is using

String switch_on = "{\"datachannelId\":\"switch_controller\",\"values\":{\"value\":1}}";
String switch_off = "{\"datachannelId\":\"switch_controller\",\"values\":{\"value\":0}}";
String content;
int status = WL_IDLE_STATUS;     // the Wifi radio's status
WiFiClient ws_client;  // use WiFiClient to connect to a web socket server
WiFiClient rs_client;  // use WiFiClient to connect to a restful server
String request;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("Booting....");
 
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  Serial.print("WiFi firmware version: ");
  Serial.println(fv); 
  if (fv != "1.1.0") {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    //delay(10000);
  }

  // you're connected now, so print out the data:
  Serial.print("You're connected to the network");
  printCurrentNet();
  printWifiData();
  request += "GET /deviceId/";
  request += String(deviceId);
  request += "/deviceKey/";
  request += String(deviceKey);
  request += "/viewer HTTP/1.1\r\n";
  request += "Upgrade: websocket\r\n";
  request += "Connection: Upgrade\r\n";
  request += "Sec-WebSocket-Version: 13\r\n";
  request += "Sec-WebSocket-Key: L159VM0TWUzyDxwJEIEzjw==\r\n";
  request += "Host: ";
  request += String(server);
  request += "\r\nOrigin: null\r\n\r\n";
  connectWs();
}

int lastTime = 0;

void connectWs () {
    if (ws_client.connect(server, ws_port)) {
        Serial.println("connected to ws_server");
        // Make a Websocket request:
        ws_client.print(request);
        delay(10);
    }
}

void loop() {
  String wscmd="";
  int checkpoint = 0;
  while (ws_client.available()) {
     if(lastTime == 0) {
        lastTime = millis();
      } else {
        int thisTime = millis();
        if(thisTime - lastTime > 10 * 1000) {
          unsigned char frame;
          frame = 0x01; // FIN
          frame = (0x01 << 4);
          ws_client.print(frame);
          lastTime = millis();
        }
      }
      int v = ws_client.read();
     
      if (v != -1) {
        wscmd += (char)v;  
        if (wscmd.substring(2).equals(switch_on)){
          digitalWrite(7, HIGH);
          String data = "string_display,,LED is ON";
          upload_datapoint(data);
          wscmd = "";
        } else if (wscmd.substring(2).equals(switch_off)){
          digitalWrite(7, LOW);
          String data = "string_display,,LED is OFF";
          upload_datapoint(data);
          wscmd = "";
        }
      }
  }
  while (!ws_client.connected()) {
     Serial.println();
     Serial.println("disconnecting from server.");
     ws_client.stop();
     delay(2000);
     connectWs();
  }
}

void printWifiData() {
  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.println(ip);

  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  Serial.print(mac[5], HEX);
  Serial.print(":");
  Serial.print(mac[4], HEX);
  Serial.print(":");
  Serial.print(mac[3], HEX);
  Serial.print(":");
  Serial.print(mac[2], HEX);
  Serial.print(":");
  Serial.print(mac[1], HEX);
  Serial.print(":");
  Serial.println(mac[0], HEX);

}

void printCurrentNet() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the MAC address of the router you're attached to:
  byte bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("BSSID: ");
  Serial.print(bssid[5], HEX);
  Serial.print(":");
  Serial.print(bssid[4], HEX);
  Serial.print(":");
  Serial.print(bssid[3], HEX);
  Serial.print(":");
  Serial.print(bssid[2], HEX);
  Serial.print(":");
  Serial.print(bssid[1], HEX);
  Serial.print(":");
  Serial.println(bssid[0], HEX);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);

  // print the encryption type:
  byte encryption = WiFi.encryptionType();
  Serial.print("Encryption Type:");
  Serial.println(encryption, HEX);
  Serial.println();
}

void upload_datapoint(String content) {
  
  
  if (rs_client.connect(server, rs_port)) {
    Serial.println("rs_client is built.");
    int thislength = content.length();
    rs_client.print("POST /api/devices/");
    rs_client.print(deviceId);
    rs_client.println("/datapoints.csv HTTP/1.1");
    rs_client.print("Host: ");
    rs_client.print(server);
    rs_client.print(":");
    rs_client.println(3000);
    rs_client.println("Content-Type: text/csv");
    rs_client.print("deviceKey: ");
    rs_client.println(deviceKey);
    rs_client.print("Content-Length: ");
    rs_client.println(thislength);
    rs_client.println();
    rs_client.println(content);
    rs_client.println(); 
    while(!rs_client.available())
    {
      delay(10);
    }
  }
  Serial.println("rs_client is closed.");
  rs_client.stop();
}
