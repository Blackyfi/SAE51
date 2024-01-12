#include <M5Stack.h>                   // Inclure la bibliothèque M5Stack
#include <WiFi.h>                      // Inclure la bibliothèque WiFi

const char *ssid = "NomDuPointDAcces"; // Nom du point d'accès WiFi
const char *password = "MotDePasse";    // Mot de passe du point d'accès WiFi
IPAddress staticIP(192, 168, 1, 10);    // Adresse IP statique pour le point d'accès
IPAddress gateway(192, 168, 1, 1);      // Adresse IP de la passerelle
IPAddress subnet(255, 255, 255, 0);     // Masque de sous-réseau

WiFiServer server(80);                  // Serveur WiFi sur le port 80

const int maxMessages = 7;              // Nombre maximal de messages à conserver dans l'historique
String messages[maxMessages];           // Tableau pour stocker les messages

void setup() {
  M5.begin();                            // Initialiser M5Stack

  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK); // Paramètres de couleur pour l'affichage sur M5Stack

  M5.Lcd.setTextDatum(MC_DATUM);        // Définir l'alignement du texte au centre
  Serial.begin(115200);                 // Initialiser la communication série pour le débogage
  
  // Configuration du M5 Stack en tant que point d'accès WiFi avec une IP statique
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(staticIP, gateway, subnet);

  M5.Lcd.println("\nPoint d'acces WiFi cree\nIP Address: " + WiFi.softAPIP()); // Afficher l'adresse IP du point d'accès
  M5.Lcd.println("\nPoint d'acces WiFi cree\nIP Address: " + WiFi.softAPIP()); // Afficher l'adresse IP du point d'accès
  
  server.begin();                        // Démarrer le serveur
}

void loop() {
  WiFiClient client = server.available(); // Attendre qu'un client se connecte
  if (client) {
    // Attendre que le client envoie des données
    while (client.connected() && !client.available()) {
      delay(1);
    }
    
    // Lire les données envoyées par le client
    String message = client.readStringUntil('\n');
    IPAddress clientIP = client.remoteIP();
    
    // Ajouter le nouveau message à l'historique avec l'adresse IP du client
    addMessage(message + "\n De : " + String(clientIP[0]) + "." + String(clientIP[1]) + "." + String(clientIP[2]) + "." + String(clientIP[3]) + "\n\n");

    // Afficher les informations sur l'écran
    displayMessages();

    // Fermer la connexion avec le client
    client.stop();
  }
}

void addMessage(String newMessage) {
  // Ajouter le nouveau message à l'historique
  for (int i = maxMessages - 1; i > 0; i--) {
    messages[i] = messages[i - 1];
  }
  messages[0] = newMessage;
}

void displayMessages() {
  // Effacer uniquement la zone d'affichage des messages
  M5.Lcd.fillRect(0, 0, 320, 240, TFT_BLACK);

  // Afficher les messages sur l'écran
  M5.Lcd.setCursor(0, 0);
  for (int i = 0; i < maxMessages; i++) {
    M5.Lcd.printf("Message : %s\n", messages[i].c_str());
  }
}
