#include <SFE_BMP180.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <DHT.h>
#include <DHT_U.h>
#include <LiquidCrystal_I2C.h> 


int pinD = 13;  
#define DHTTYPE DHT11  
SFE_BMP180 bmp180;
LiquidCrystal_I2C lcd(0x27, 16, 2);  

double PresionNivelMar = 1013.25;  


const char* ssid = "Nombre_Wi-fi";
const char* password = "Contraseña";
const char* mqtt_server = "mqtt.eclipseprojects.io";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (100)
char msg[MSG_BUFFER_SIZE];
int value = 0;


double humedad = 0.0;
double temperatura = 0.0;
double presion = 0.0;
double altura = 0.0;


DHT dht(pinD, DHTTYPE);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("Dirección IP: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("conectado");
      client.publish("st/ejemplo", "hola mundo");
      client.subscribe("inTopic");
    } else {
      Serial.print("falló, rc=");
      Serial.print(client.state());
      Serial.println(" intenta de nuevo en 5 segundos");
      delay(5000);
    }
  }
}


void obtenerHumedad() {
  humedad = dht.readHumidity();
  if (isnan(humedad)) {
    Serial.println("Error al leer la humedad");
    humedad = 0.0;
  } else {
    Serial.print("Humedad leída: ");
    Serial.println(humedad);
  }
}

void obtenerTemperatura() {
  char status;
  status = bmp180.startTemperature();
  if (status == 0) {
    Serial.println("Error al iniciar la lectura de temperatura BMP180");
  } else {
    delay(status);
    status = bmp180.getTemperature(temperatura);
    if (status != 0) {
      Serial.print("Temperatura BMP180: ");
      Serial.print(temperatura);
      Serial.println(" °C");
    }
  }
}

void obtenerPresion() {
  char status;
  double T = temperatura; 
  status = bmp180.startPressure(3);
  if (status != 0) {
    delay(status);
    status = bmp180.getPressure(presion, T);
    if (status != 0) {
      Serial.print("Presión: ");
      Serial.print(presion);
      Serial.println(" mb");
    }
  }
}

void obtenerAltura() {
  altura = bmp180.altitude(presion, PresionNivelMar);
  Serial.print("Altitud: ");
  Serial.print(altura);
  Serial.println(" m");
}


void publicarHumedad() {
  snprintf(msg, MSG_BUFFER_SIZE, "%.2f%%", humedad);
  client.publish("st/grupo8/humedad", msg);
  Serial.println("Publicando Humedad...");
}

void publicarTemperatura() {
  snprintf(msg, MSG_BUFFER_SIZE, "%.2f°C", temperatura);
  client.publish("st/grupo8/temperatura", msg);
  Serial.println("Publicando Temperatura...");
}

void publicarPresion() {
  snprintf(msg, MSG_BUFFER_SIZE, "%.2f mb", presion);
  client.publish("st/grupo8/presion", msg);
  Serial.println("Publicando Presión...");
}

void publicarAltura() {
  snprintf(msg, MSG_BUFFER_SIZE, "%.2f m", altura);
  client.publish("st/grupo8/altura", msg);
  Serial.println("Publicando Altitud...");
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  dht.begin();  

  
  lcd.begin(16, 2);  
  lcd.init();
  lcd.backlight();


  if (bmp180.begin()) {
    Serial.println("BMP180 inicializado correctamente");
  } else {
    Serial.println("Error al inicializar el BMP180");
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  
  obtenerHumedad();
  obtenerTemperatura();
  obtenerPresion();
  obtenerAltura();

  
  lcd.clear();
  lcd.setCursor(0, 0);  
  lcd.print("T:");
  lcd.print((int)temperatura);  
  lcd.print("C H:");
  lcd.print((int)humedad);  
  lcd.print("%");

  lcd.setCursor(0, 1);  
  lcd.print("P:");
  lcd.print((int)presion);  
  lcd.print("mb A:");
  lcd.print((int)altura);  
  lcd.print("m");

  
  unsigned long now = millis();
  if (now - lastMsg > 10000) {  
    lastMsg = now;
    ++value;

    
    publicarHumedad();
    publicarTemperatura();
    publicarPresion();
    publicarAltura();
  }

  delay(2000);
}
