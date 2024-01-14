#include <M5Stack.h>           // Inclusion de la bibliothèque M5Stack pour utiliser les fonctionnalités de l'appareil
#include <WiFi.h>              // Inclusion de la bibliothèque WiFi pour configurer et gérer la connexion Wi-Fi
#include <Adafruit_Sensor.h>   // Inclusion de la bibliothèque Adafruit_Sensor pour utiliser les capteurs Adafruit
#include <Adafruit_BMP280.h>   // Inclusion de la bibliothèque Adafruit_BMP280 pour utiliser le capteur de pression
#include <SHT3X.h>             // Inclusion de la bibliothèque SHT3X pour utiliser le capteur d'humidité et de température

const char *nomAP = "Salon";            // Nom de la pièce où est installé l'émetteur
const char *targetSsid = "Montre";            // Nom du dispositif porté
const char *password = "MotDePasseVraimentDureJazz";    // Mot de passe du réseau Wi-Fi
const char *montreIP = "192.168.1.10";  // Adresse IP statique du dispositif porté
const float maxTemperature = 30.0;      // Température maximale autorisée
const float maxHumindex = 35.0;         // Humindex maximal autorisé
const int cryptingKey = 24;             // Clé de chiffrement, décalage par rapport à Y (code ASCII : 24 + 1 = 25e lettre de l'alphabet)
int messageCounter = 1;                 // Compteur de messages envoyés
String messageToSend;                   // Message à envoyer
String encryptedMessageToSend;          // Message crypté
String signature = "Signature super securise jazz";  // Signature de confirmation




const int maxMessages = 7;      // Nombre maximal de messages à afficher
String messages[maxMessages];   // Tableau pour stocker les messages envoyés

Adafruit_BMP280 bmp;            // Objet pour le capteur de pression
SHT3X sht;                      // Objet pour le capteur d'humidité et de température

void setup() {
  M5.begin(true, false, true, true);  // Initialisation de M5Stack avec écran activé, son désactivé, vibreur activé et charge de la batterie activée
  initSensors();                      // Initialisation les capteurs du module ENVII
  connectToWiFi();                    // Connexion au réseau Wi-Fi
}

void loop() {
  float temperature = readTemperature();  // Lecture de la température
  float humindex = calculateHumindex();  // On calcul et récupère le Humindex

  if (temperature > maxTemperature) {
    messageToSend = createAlertMessageTemperature(temperature);   // Création du message d'alerte de température
  } else if (humindex > maxHumindex) {
    messageToSend = createAlertMessageHumindex(humindex);         // Création du message d'alerte d'humidex
  } else {
    messageToSend = createRegularMessage(temperature, humindex);  // Création du message régulier
  }
  
  encryptedMessageToSend = messageEncryption(messageToSend);      // Chiffre le message à envoyer


  WiFiClient client;                // Création d'un client Wi-Fi
  if (client.connect(montreIP, 80)) {
    sendAndDisplayMessage(client);  // Envoi du message au récepteur et affichage sur l'émetteur
  } else {
    handleConnectionError();         // Gestion de l'erreur de connexion
  }

  delay(5000);                       // Attente de 10 secondes avant la prochaine itération/envoi de message, puisque le récepteur attend 5 secondes avant d'envoyer en wifi a la GateWay Lora
}

void initSensors() {
  bmp.begin(0x76);                   // Initialisation du capteur de pression avec l'adresse I2C
  sht.init();                        // Initialisation du capteur d'humidité et de température
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);  // Configuration des couleurs de l'écran
  M5.Lcd.setTextDatum(MC_DATUM);              // Configuration du point de référence du texte
  Serial.begin(115200);                       // Initialisation de la communication série
}

void connectToWiFi() {
  WiFi.begin(targetSsid, password);                   // Démarre la connexion au réseau Wi-Fi

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    M5.Lcd.print(".");                          // Affiche des points pendant la tentative de connexion
  }

  char message[50];
  // Utilisation de sprintf pour formater la chaîne avec le nom du réseau WiFi
  sprintf(message, "\nConnecté au réseau WiFi de : %s", targetSsid);
  // message : tableau de caractères où la chaîne formatée sera stockée
  // "\nConnecté au réseau WiFi de : %s" : Chaîne de format avec un spécificateur %s pour insérer targetSsid
  // targetSsid : La variable qui sera insérée à la place du spécificateur %s dans la chaîne de format
  // Le résultat final est stocké dans le tableau de caractères "message"
  M5.Lcd.println(message);  // Affiche un message une fois la connexion établie
}

float readTemperature() {
  sht.get();                          // Obtenir les données du capteur d'humidité et de température
  // Remplacer par sht.cTemp pour une lecture réelle du capteur
  // return maxTemperature + 1; // Pour des tests de dépassement de seuil
  return sht.cTemp;
}

float calculateHumindex() {
  float T = sht.cTemp;             // Température en degrés Celsius
  float HR = sht.humidity;         // Humidité relative en pourcentage

  float es = 6.112 * exp((17.67 * T) / (T + 243.5));    // Pression de vapeur saturante
  float e = (HR / 100.0) * es;                          // Pression de vapeur actuelle
  float humidex = T + (5.0 / 9.0) * (e - 10.0);         // Calcul de l'indice humidex

  return humidex;  // Retourne la valeur de l'indice humidex
}

String createAlertMessageTemperature(float temperature) {
  return signature + "Alerte ! " + String(nomAP) + " Température trop élevée : " + String(temperature);
}

String createAlertMessageHumindex(float humindex) {
  return signature + "Alerte ! " + String(nomAP) + " Humidex trop élevé : " + String(humindex);
}

String createRegularMessage(float temperature, float humindex) {
  return signature + "Température : " + String(temperature) + "°C & Humidex : " + String(humindex) + "% Message " + String(messageCounter++);
}

// Envoie le message au serveur et met à jour l'affichage local
void sendAndDisplayMessage(WiFiClient &client) {
  encryptedMessageToSend = encryptedMessageToSend;  // Ajout de la signature au message

  client.println(encryptedMessageToSend);  // Envoie le message au serveur via le client Wi-Fi
  client.stop();  // Ferme la connexion avec le client après l'envoi du message
  addMessage(encryptedMessageToSend);  // Ajoute le nouveau message au tableau local
  displayMessages();  // Met à jour l'affichage local avec les messages
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
  WiFi.begin(targetSsid, password);  // Nouvelle tentative de connexion au réseau Wi-Fi
  int attempts = 0;    // Initialisation du compteur de tentatives

  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(250);      // Attente de 1 seconde entre chaque tentative
    attempts++;       // Incrémentation du compteur de tentatives
  }

  if (WiFi.status() == WL_CONNECTED) {
    M5.Lcd.println("Reconnexion reussie!");  // Affichage du message en cas de reconnexion réussie
    messageCounter = 1;  // Réinitialisation du compteur de messages
  } else {
    M5.Lcd.println("Echec de la reconnexion. Réessayer plus tard.");  // Affichage du message en cas d'échec de reconnexion
  }
}

// Fonction de chiffrement du message
// Prend en entrée le message à chiffrer (messageToEncrypt)
// Retourne le message chiffré en décalant chaque caractère par la clé de chiffrement (cryptingKey)
String messageEncryption(String messageToEncrypt) {
  String cryptedMessage = "";
  for (char currentChar : messageToEncrypt) {
    cryptedMessage += char(int(currentChar) + cryptingKey);
  }
  return cryptedMessage;
}
