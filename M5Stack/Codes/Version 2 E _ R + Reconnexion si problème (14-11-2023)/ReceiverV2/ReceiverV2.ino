#include <M5Stack.h>                // Inclusion de la bibliothèque M5Stack pour utiliser les fonctionnalités de l'appareil
#include <WiFi.h>                   // Inclusion de la bibliothèque WiFi pour configurer et gérer la connexion Wi-Fi

const char *ssid = "Montre";         // Nom du réseau Wi-Fi
const char *password = "MotDePasse"; // Mot de passe du réseau Wi-Fi
IPAddress staticIP(192, 168, 1, 10);  // Adresse IP statique pour le point d'accès Wi-Fi
IPAddress gateway(192, 168, 1, 1);    // Adresse IP de la passerelle
IPAddress subnet(255, 255, 255, 0);   // Masque de sous-réseau

WiFiServer server(80);               // Création d'une instance du serveur Wi-Fi

const int maxMessages = 7;           // Nombre maximal de messages à afficher
String messages[maxMessages];         // Tableau pour stocker les messages reçus

void setup() {
  M5.begin();                         // Initialisation de M5Stack
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);  // Configuration des couleurs de l'écran
  M5.Lcd.setTextDatum(MC_DATUM);     // Configuration du format de texte pour le centre
  Serial.begin(115200);               // Initialisation de la communication série
  
  WiFi.softAP(ssid, password);        // Configuration du point d'accès Wi-Fi
  WiFi.softAPConfig(staticIP, gateway, subnet);  // Configuration de l'adresse IP statique
  
  M5.Lcd.println("\nPoint d'acces WiFi cree\nIP Address: " + WiFi.softAPIP());  // Affichage de l'adresse IP du point d'accès
  server.begin();                     // Démarrage du serveur Wi-Fi
}

void loop() {
  WiFiClient client = server.available();  // Vérification de la disponibilité d'un client
  if (client) {
    while (client.connected() && !client.available()) {
      delay(1);
    }
    
    String message = client.readStringUntil('\n');  // Lecture du message envoyé par le client
    IPAddress clientIP = client.remoteIP();         // Obtention de l'adresse IP du client
    
    // Vérification si le message commence par "Alerte!"
    if (message.startsWith("Alerte!")) {
      // Extraction du nom du point d'accès
      int start = message.indexOf(" ") + 1;    // Recherche de l'espace après "Alerte!"
      int end = message.indexOf(" ", start);   // Recherche de l'espace suivant après le nom du point d'accès
      String apName = message.substring(start, end);  // Extraction du nom du point d'accès
      
      // Affichage de "Seuil depasse" avec le nom du point d'accès
      addMessage("Seuil depasse dans la piece : " + apName + "\n De : " + String(clientIP[0]) + "." + String(clientIP[1]) + "." + String(clientIP[2]) + "." + String(clientIP[3]) + "\n\n");

      // Changement de la couleur de l'écran en rouge et affichage du nom de la pièce
      M5.Lcd.fillScreen(TFT_RED);
      M5.Lcd.setTextSize(2);
      M5.Lcd.setTextColor(TFT_WHITE, TFT_RED);
      M5.Lcd.drawString(apName, 160, 120);  // Affichage du nom de la pièce au centre
      delay(1000);  // Maintien de l'écran rouge pendant 1 seconde
    } else {
      // Ajout du message normal à l'historique avec l'adresse IP du client
      addMessage(message + "\n De : " + String(clientIP[0]) + "." + String(clientIP[1]) + "." + String(clientIP[2]) + "." + String(clientIP[3]) + "\n\n");
    }

    // Effacement de l'écran après l'affichage des messages
    displayMessages();
    client.stop();  // Déconnexion du client
  }
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

  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.setCursor(0, 0);
  
  for (int i = 0; i < maxMessages; i++) {
    M5.Lcd.printf("Message : %s\n", messages[i].c_str());  // Affichage de chaque message sur l'écran
  }
}