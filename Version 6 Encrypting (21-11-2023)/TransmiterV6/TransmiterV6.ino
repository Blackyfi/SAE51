#include <M5Stack.h>           // Inclusion de la bibliothèque M5Stack pour utiliser les fonctionnalités de l'appareil
#include <WiFi.h>              // Inclusion de la bibliothèque WiFi pour configurer et gérer la connexion Wi-Fi
#include <Adafruit_Sensor.h>   // Inclusion de la bibliothèque Adafruit_Sensor pour utiliser les capteurs Adafruit
#include <Adafruit_BMP280.h>   // Inclusion de la bibliothèque Adafruit_BMP280 pour utiliser le capteur de pression
#include <SHT3X.h>             // Inclusion de la bibliothèque SHT3X pour utiliser le capteur d'humidité et de température

const char *nomAP = "Salon";   // Nom de la pièce où est installé l'émetteur
const char *ssid = "Montre";   // Nom du réseau Wi-Fi
const char *password = "MotDePasse";  // Mot de passe du réseau Wi-Fi
const char *serverIP = "192.168.1.10";  // Adresse IP statique du récepteur
int messageCounter = 1;        // Compteur de messages envoyés
String messageToSend;          // Message à envoyer
String encryptedMessageToSend;    // Message crypté
const float maxTemperature = 30.0;  // Température maximale autorisée
const float maxHumindex = 35.0;  // Humindex maximal autorisé
const int cryptingKey = 24; // Clé de chiffrement, décalage par rapport à Y (code ASCII : 24 + 1 = 25e lettre de l'alphabet)

const int maxMessages = 7;      // Nombre maximal de messages à afficher
String messages[maxMessages];   // Tableau pour stocker les messages envoyés

Adafruit_BMP280 bmp;            // Objet pour le capteur de pression
SHT3X sht;                      // Objet pour le capteur d'humidité et de température

void setup() {
  M5.begin(true, false, true, true);  // Initialisation de M5Stack avec écran activé, son désactivé, vibreur activé et charge de la batterie activée
  initSensors();                      // Initialisation des capteurs
  connectToWiFi();                    // Connexion au réseau Wi-Fi
}

void loop() {
  float temperature = readTemperature();  // Lecture de la température
  float humindex = calculateHumindex();  // On calcul et récupère le Humindex

  if (temperature > maxTemperature) {
    messageToSend = createAlertMessageTemperature(temperature);  // Création d'un message d'alerte
  } else if (humindex > maxHumindex) {
    messageToSend = createAlertMessageHumindex(humindex);  // Création d'un message d'alerte
  } else {
    messageToSend = createRegularMessage(temperature, humindex);  // Création d'un message régulier
  }
  
  encryptedMessageToSend = messageEncryption(messageToSend);
  WiFiClient client;                // Création d'un client Wi-Fi

  if (client.connect(serverIP, 80)) {
    sendAndDisplayMessage(client);  // Envoi du message au récepteur et affichage sur l'émetteur
  } else {
    handleConnectionError();         // Gestion de l'erreur de connexion
  }

  
  delay(5000);                       // Attente de 5 secondes avant la prochaine itération/envoi de message
}

void initSensors() {
  bmp.begin(0x76);                   // Initialisation du capteur de pression avec l'adresse I2C
  //sht.init();                        // Initialisation du capteur d'humidité et de température
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);  // Configuration des couleurs de l'écran
  M5.Lcd.setTextDatum(MC_DATUM);              // Configuration du point de référence du texte
  Serial.begin(115200);                       // Initialisation de la communication série
}

void connectToWiFi() {
  WiFi.begin(ssid, password);          // Connexion au réseau Wi-Fi

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    M5.Lcd.print(".");
  }

  M5.Lcd.println("\nConnecte au reseau WiFi");
}

float readTemperature() {
  sht.get();                          // Obtenir les données du capteur d'humidité et de température
  // Remplacer par sht.cTemp pour une lecture réelle du capteur
  // return 32; // Pour des tests de dépassement de seuil
  return sht.cTemp;
}

float calculateHumindex() {
  float T = sht.cTemp;
  float HR = sht.humidity;

  float es = 6.112 * exp((17.67 * T) / (T + 243.5));
  float e = (HR / 100.0) * es;
  float humidex = T + (5.0 / 9.0) * (e - 10.0);

  return humidex;
}

String createAlertMessageTemperature(float temperature) {
  return "Alerte! " + String(nomAP) + " Temperature trop elevee: " + String(temperature);
}

String createAlertMessageHumindex(float humindex) {
  return "Alerte! " + String(nomAP) + " Humindex trop elevee: " + String(humindex);
}

String createRegularMessage(float temperature, float humindex) {
  return "Temperature: " + String(temperature) + "°C & Humindex: " + String(humindex) + "% Message " + String(messageCounter++);
}

// Envoie le message au serveur et met à jour l'affichage local
void sendAndDisplayMessage(WiFiClient &client) {
  client.println(encryptedMessageToSend);  // Envoie le message au serveur via le client Wi-Fi
  client.stop();                   // Ferme la connexion avec le client après l'envoi du message
  addMessage(encryptedMessageToSend);       // Ajoute le nouveau message au tableau local
  displayMessages();               // Met à jour l'affichage local avec les messages
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

// Gestion des erreurs de connexion Wi-Fi
void handleConnectionError() {
  M5.Lcd.println("Erreur de connexion, tentative de reconnexion...");  // Affichage du message d'erreur
  resetConnection();  // Appel de la fonction pour réinitialiser la connexion
}

// Réinitialisation de la connexion Wi-Fi
void resetConnection() {
  WiFi.disconnect();  // Déconnexion du réseau Wi-Fi actuel
  delay(1000);        // Attente de 1 seconde
  WiFi.begin(ssid, password);  // Nouvelle tentative de connexion au réseau Wi-Fi
  int attempts = 0;    // Initialisation du compteur de tentatives

  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(1000);      // Attente de 1 seconde entre chaque tentative
    attempts++;       // Incrémentation du compteur de tentatives
  }

  if (WiFi.status() == WL_CONNECTED) {
    M5.Lcd.println("Reconnexion reussie!");  // Affichage du message en cas de reconnexion réussie
    messageCounter = 1;  // Réinitialisation du compteur de messages
  } else {
    M5.Lcd.println("Echec de la reconnexion. Réessayer plus tard.");  // Affichage du message en cas d'échec de reconnexion
  }
}

String messageEncryption(String messageToEncrypt) {
  char *upedMessage = new char[messageToEncrypt.length() + 1];  // Réservation d'une mémoire de taille `messageToEncrypt.length() + 1` pour le tableau `upedMessage`.
  messageToEncrypt.toCharArray(upedMessage, messageToEncrypt.length() + 1); // La valeur `+1` permet de réserver de la place pour le caractère nul (`'\0'`), qui marque la fin de la chaîne.

  int lengthMessage = strlen(upedMessage);
  char *cryptedMessage = NULL;

  for (int i = 0; i < lengthMessage; i++) {
    if (upedMessage[i] == '\0') {
      cryptedMessage[i] = '\0';
    } else {
      int asc = (int) upedMessage[i] + cryptingKey;
      cryptedMessage[i] = (char) (asc + 26 * ((asc < 65) - (asc > 90)));
    }
  }

  delete[] upedMessage;
  return cryptedMessage;
}