#include <M5Stack.h>  // Inclusion de la bibliothèque M5Stack pour utiliser les fonctionnalités de l'appareil
#include <WiFi.h>     // Inclusion de la bibliothèque WiFi pour configurer et gérer la connexion Wi-Fi

const char *ssid = "Montre";                // Nom du réseau Wi-Fi
const char *password = "MotDePasse";        // Mot de passe du réseau Wi-Fi
IPAddress staticIP(192, 168, 1, 10);        // Adresse IP statique du récepteur
IPAddress gateway(192, 168, 1, 1);          // Adresse IP de la passerelle
IPAddress subnet(255, 255, 255, 0);         // Masque de sous-réseau

WiFiServer server(80);                      // Serveur Wi-Fi écoutant sur le port 80
WiFiClient client;                          // Client Wi-Fi pour gérer les connexions

const int maxMessages = 7;                  // Nombre maximal de messages à afficher
String messages[maxMessages];               // Tableau pour stocker les messages reçus

bool wifiEnabled = true;                    // Indicateur pour activer ou désactiver la connexion Wi-Fi

void setup() {
  M5.begin();                               // Initialisation de M5Stack
  initDisplay();                            // Initialisation de l'affichage
  Serial.begin(115200);                     // Initialisation de la communication série
  wifiSetup();                              // Configuration initiale de la connexion Wi-Fi
}

void loop() {
  handleWiFi();                             // Gérer l'état de la connexion Wi-Fi
  handleClient();                           // Gérer les connexions client
  handleM5Updates();                        // Mettre à jour les entrées/sorties de M5Stack
  handleButtons();                          // Gérer les interactions avec les boutons
}

void initDisplay() {
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);  // Configuration des couleurs de l'écran
  M5.Lcd.setTextDatum(MC_DATUM);              // Configuration du point de référence du texte
}

void wifiSetup() {
  WiFi.softAP(ssid, password);               // Configurer le point d'accès Wi-Fi avec le nom et le mot de passe
  WiFi.softAPConfig(staticIP, gateway, subnet);  // Configurer l'adresse IP statique
  M5.Lcd.println("\nPoint d'acces WiFi cree\nIP Address: " + WiFi.softAPIP());  // Afficher l'adresse IP du point d'accès
  server.begin();                            // Démarrer le serveur Wi-Fi
}

void handleWiFi() {
  if (!wifiEnabled) {
    IPAddress nullIP(0, 0, 0, 0);
    WiFi.softAPConfig(nullIP, nullIP, nullIP);  // Désactiver la connexion Wi-Fi
    server.stop();                           // Arrêter le serveur
    M5.Lcd.fillScreen(TFT_BLACK);            // Effacer l'écran
  }
}

void handleClient() {
  if (wifiEnabled) {
    client = server.available();            // Vérifier la disponibilité d'un client Wi-Fi
    if (client) {
      while (client.connected() && !client.available()) {
        delay(1);
      }

      String message = client.readStringUntil('\n');  // Lire le message envoyé par le client
      IPAddress clientIP = client.remoteIP();         // Obtenir l'adresse IP du client

      if (message.startsWith("Alerte!")) {
        handleAlert(message, clientIP);       // Gérer un message d'alerte
      } else {
        addMessage(message + "\n De : " + clientIP.toString() + "\n\n");  // Ajouter un message régulier
      }

      displayMessages();                      // Afficher les messages sur l'écran
      client.stop();                          // Fermer la connexion avec le client
    }
  }
}

void handleAlert(String alertMessage, IPAddress clientIP) {
  int start = alertMessage.indexOf(" ") + 1;
  int end = alertMessage.indexOf(" ", start);
  String apName = alertMessage.substring(start, end);

  addMessage("Seuil depasse dans la piece : " + apName +
             "\n De : " + clientIP.toString() + "\n\n");

  M5.Lcd.fillScreen(TFT_RED);                // Remplir l'écran en rouge
  M5.Lcd.setTextSize(2);                     // Configuration de la taille du texte
  M5.Lcd.setTextColor(TFT_WHITE, TFT_RED);   // Configuration des couleurs du texte
  M5.Lcd.drawString(apName, 160, 120);       // Afficher le nom de la pièce au centre
  delay(1000);                               // Maintenir l'écran rouge pendant 1 seconde
}

void handleM5Updates() {
  M5.update();                               // Mettre à jour les entrées/sorties de M5Stack
}

void handleButtons() {
  if (M5.BtnB.wasPressed()) {
    wifiEnabled = false;                      // Désactiver la connexion Wi-Fi
  }

  if (M5.BtnC.wasPressed()) {
    wifiEnabled = true;                       // Activer la connexion Wi-Fi
    wifiSetup();                              // Configurer le Wi-Fi
  }
}

void addMessage(String newMessage) {
  for (int i = maxMessages - 1; i > 0; i--) {
    messages[i] = messages[i - 1];           // Décaler les messages vers le bas dans le tableau
  }
  messages[0] = newMessage;                   // Ajouter le nouveau message en haut du tableau
}

void displayMessages() {
  M5.Lcd.fillRect(0, 0, 320, 240, TFT_BLACK);  // Effacer l'écran avec une couleur noire

  M5.Lcd.setTextSize(1);                        // Configuration de la taille du texte
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);    // Configuration des couleurs du texte
  M5.Lcd.setCursor(0, 0);                       // Position du curseur en haut à gauche

  for (int i = 0; i < maxMessages; i++) {
    M5.Lcd.printf("Message : %s\n", messages[i].c_str());  // Afficher chaque message sur l'écran
  }
}