#include <M5Stack.h>                   // Inclure la bibliothèque M5Stack
#include <WiFi.h>                      // Inclure la bibliothèque WiFi
#include <Adafruit_Sensor.h>           // Inclure la bibliothèque Adafruit Sensor
#include <Adafruit_BMP280.h>           // Inclure la bibliothèque Adafruit BMP280
#include <SHT3X.h>                     // Inclure la bibliothèque SHT3X

const char *ssid = "NomDuPointDAcces"; // Nom du point d'accès WiFi
const char *password = "MotDePasse";    // Mot de passe du point d'accès WiFi
const char *serverIP = "192.168.1.10";  // Adresse IP du serveur
int messageCounter = 1;                 // Compteur pour l'incrémentation des messages
String messageToSend;                   // Message à envoyer

const int maxMessages = 7;              // Nombre maximal de messages à conserver dans l'historique
String messages[maxMessages];           // Tableau pour stocker les messages

Adafruit_BMP280 bmp;                    // Déclarer le capteur BMP280 (Le capteur de pression atmosphérique)
SHT3X sht;                              // Déclarer le capteur SHT30 (Le capteur de température et humidité)

void setup() {
  M5.begin(true, false, true, true);    // Initialiser M5Stack et ajouter le bus I2C

  bmp.begin(0x76);                       // Initialiser le capteur BMP280 avec l'adresse I2C 0x76 (Adresse par défaut et bus I2C sur le Port A)
  sht.init();                            // Initialiser le capteur SHT30

  WiFi.begin(ssid, password);            // Se connecter au point d'accès WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    M5.Lcd.print(".");
  }
  M5.Lcd.println("\nConnecte au reseau WiFi");

  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK); // Paramètres de couleur pour l'affichage sur M5Stack

  M5.Lcd.setTextDatum(MC_DATUM);        // Définir l'alignement du texte au centre
  Serial.begin(115200);                 // Initialiser la communication série pour le débogage
}

void loop() {
  sht.get();                            // Mesurer la température et l'humidité avec le capteur SHT30
  float temperature = sht.cTemp;        // Récupérer la température
  
  // Construire le message avec l'incrémentation et la température
  messageToSend = "Temperature: " + String(temperature) + " Message " + String(messageCounter++);

  WiFiClient client;
  if (client.connect(serverIP, 80)) {  // Se connecter au serveur (M5 AP)
    client.println(messageToSend);     // Envoyer le message
    client.stop();

    addMessage(messageToSend);          // Ajouter le nouveau message à l'historique
    displayMessages();                  // Afficher les informations sur l'écran
  }

  delay(5000);                          // Attendre 5 secondes avant d'envoyer une nouvelle fois
}
 
void addMessage(String newMessage) {
  for (int i = maxMessages - 1; i > 0; i--) {
    messages[i] = messages[i - 1];
  }
  messages[0] = newMessage;
}

void displayMessages() {
  M5.Lcd.fillRect(0, 0, 320, 240, TFT_BLACK); // Effacer uniquement la zone d'affichage des messages

  M5.Lcd.setCursor(0, 0);
  for (int i = 0; i < maxMessages; i++) {
    M5.Lcd.printf("Message : %s\n", messages[i].c_str());
  }
}
