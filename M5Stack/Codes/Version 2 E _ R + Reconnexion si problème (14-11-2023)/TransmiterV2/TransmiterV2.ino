#include <M5Stack.h>                // Inclusion de la bibliothèque M5Stack pour utiliser les fonctionnalités de l'appareil
#include <WiFi.h>                   // Inclusion de la bibliothèque WiFi pour configurer et gérer la connexion Wi-Fi
#include <Adafruit_Sensor.h>        // Inclusion de la bibliothèque Adafruit Sensor pour les capteurs
#include <Adafruit_BMP280.h>        // Inclusion de la bibliothèque Adafruit BMP280 pour le capteur de pression atmosphérique
#include <SHT3X.h>                  // Inclusion de la bibliothèque SHT3X pour le capteur d'humidité et de température

const char *nomAP = "Salon";         // Nom du point d'accès
const char *ssid = "Montre";         // Nom du réseau Wi-Fi
const char *password = "MotDePasse"; // Mot de passe du réseau Wi-Fi
const char *serverIP = "192.168.1.10";// Adresse IP du serveur
int messageCounter = 1;               // Compteur de messages
String messageToSend;                 // Message à envoyer
const float maxTemperature = 30.0;    // Température maximale autorisée

const int maxMessages = 7;            // Nombre maximal de messages à afficher
String messages[maxMessages];          // Tableau pour stocker les messages reçus

Adafruit_BMP280 bmp;                  // Instance du capteur BMP280
SHT3X sht;                            // Instance du capteur SHT3X

void setup() {
  M5.begin(true, false, true, true);  // Initialisation de M5Stack
  bmp.begin(0x76);                    // Initialisation du capteur BMP280 avec l'adresse I2C
  sht.init();                         // Initialisation du capteur SHT3X
  WiFi.begin(ssid, password);         // Connexion au réseau Wi-Fi
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    M5.Lcd.print(".");
  }
  
  M5.Lcd.println("\nConnecte au reseau WiFi");  // Affichage de la connexion réussie
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);    // Configuration des couleurs de l'écran
  M5.Lcd.setTextDatum(MC_DATUM);                 // Configuration du format de texte pour le centre
  Serial.begin(115200);                          // Initialisation de la communication série
}

void loop() {
  sht.get();  // Acquisition des données du capteur SHT3X
  // float temperature = sht.cTemp;
  float temperature = 32;  // Test de dépassement du seuil de température
  
  if (temperature > maxTemperature) {
    // Température supérieure au seuil maximal, envoi d'une alerte
    messageToSend = "Alerte! " + String(nomAP) + " Temperature trop elevee: " + String(temperature);
  } else {
    // Construction du message avec l'incrémentation et la température
    messageToSend = "Temperature: " + String(temperature) + " Message " + String(messageCounter++);
  }

  WiFiClient client;
  if (client.connect(serverIP, 80)) {
    client.println(messageToSend);
    client.stop();
    addMessage(messageToSend);
    displayMessages();
  } else {
    M5.Lcd.println("Erreur de connexion, tentative de reconnexion...");
    resetConnection();
  }

  delay(5000);
}

void addMessage(String newMessage) {
  for (int i = maxMessages - 1; i > 0; i--) {
    messages[i] = messages[i - 1];  // Décalage des messages vers le bas dans le tableau
  }
  messages[0] = newMessage;          // Ajout du nouveau message en haut du tableau
}

void displayMessages() {
  // Effacement de l'écran avant l'affichage des messages
  M5.Lcd.fillRect(0, 0, 320, 240, TFT_BLACK);

  M5.Lcd.setCursor(0, 0);
  for (int i = 0; i < maxMessages; i++) {
    M5.Lcd.printf("Message : %s\n", messages[i].c_str());  // Affichage de chaque message sur l'écran
  }
}

void resetConnection() {
  WiFi.disconnect();
  delay(1000);
  WiFi.begin(ssid, password);
  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(1000);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    M5.Lcd.println("Reconnexion reussie!");  // Affichage de la reconnexion réussie
    messageCounter = 1;
  } else {
    M5.Lcd.println("Echec de la reconnexion. Réessayer plus tard.");  // Affichage de l'échec de la reconnexion
  }
}