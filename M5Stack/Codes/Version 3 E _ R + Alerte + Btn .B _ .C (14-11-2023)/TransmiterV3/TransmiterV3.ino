#include <M5Stack.h>                // Inclusion de la bibliothèque M5Stack pour utiliser les fonctionnalités de l'appareil
#include <WiFi.h>                   // Inclusion de la bibliothèque WiFi pour configurer et gérer la connexion Wi-Fi
#include <Adafruit_Sensor.h>        // Inclusion de la bibliothèque Adafruit_Sensor pour utiliser les capteurs Adafruit
#include <Adafruit_BMP280.h>        // Inclusion de la bibliothèque Adafruit_BMP280 pour utiliser le capteur de pression
#include <SHT3X.h>                  // Inclusion de la bibliothèque SHT3X pour utiliser le capteur d'humidité et de température

const char *nomAP = "Salon";         // Nom de la pièce où est installé l'émetteur
const char *ssid = "Montre";         // Nom du réseau Wi-Fi
const char *password = "MotDePasse"; // Mot de passe du réseau Wi-Fi
const char *serverIP = "192.168.1.10";// Adresse IP statique du récepteur
int messageCounter = 1;              // Compteur de messages envoyés
String messageToSend;                // Message à envoyer
const float maxTemperature = 30.0;   // Température maximale autorisée

const int maxMessages = 7;           // Nombre maximal de messages à afficher
String messages[maxMessages];         // Tableau pour stocker les messages envoyés

Adafruit_BMP280 bmp;                 // Objet pour le capteur de pression
SHT3X sht;                           // Objet pour le capteur d'humidité et de température

void setup() {
  M5.begin(true, false, true, true);  // Initialisation de M5Stack avec écran activé, son désactivé, vibreur activé et charge de la batterie activée
  bmp.begin(0x76);                    // Initialisation du capteur de pression avec l'adresse I2C
  sht.init();                         // Initialisation du capteur d'humidité et de température
  WiFi.begin(ssid, password);         // Connexion au réseau Wi-Fi
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    M5.Lcd.print(".");
  }
  
  M5.Lcd.println("\nConnecte au reseau WiFi");
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);  // Configuration des couleurs de l'écran
  M5.Lcd.setTextDatum(MC_DATUM);              // Configuration du point de référence du texte
  Serial.begin(115200);                       // Initialisation de la communication série
}

void loop() {
  sht.get();                          // Obtenir les données du capteur d'humidité et de température
  // float temperature = 32;             // Test de dépassement du seuil de température (remplacer par sht.cTemp pour utiliser le capteur réel)
  float temperature = sht.cTemp;

  if (temperature > maxTemperature) {
    // La température dépasse le seuil maximal, envoi d'une alerte
    messageToSend = "Alerte! " + String(nomAP) + " Temperature trop elevee: " + String(temperature);
  } else {
    // Construction du message avec l'incrément et la température
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
    messages[i] = messages[i - 1];  // Décaler les messages vers le bas dans le tableau
  }
  messages[0] = newMessage;          // Ajouter le nouveau message en haut du tableau
}

void displayMessages() {
  M5.Lcd.fillRect(0, 0, 320, 240, TFT_BLACK);  // Effacer l'écran avec une couleur noire

  M5.Lcd.setCursor(0, 0);
  
  for (int i = 0; i < maxMessages; i++) {
    M5.Lcd.printf("Message : %s\n", messages[i].c_str());  // Afficher chaque message sur l'écran
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
    M5.Lcd.println("Reconnexion reussie!");
    messageCounter = 1;
  } else {
    M5.Lcd.println("Echec de la reconnexion. Réessayer plus tard.");
  }
}