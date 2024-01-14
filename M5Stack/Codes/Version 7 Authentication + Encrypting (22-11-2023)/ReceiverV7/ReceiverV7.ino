#include <M5Stack.h>
#include <WiFi.h>

const char *ssid = "Montre";
const char *password = "MotDePasseVraimentDureJazz";
const int cryptingKey = 24;
String expectedSignature = "Signature super securise jazz";
IPAddress staticIP(192, 168, 1, 10);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

WiFiServer server(80);
WiFiClient client;
bool wifiEnabled = true;

const int maxMessages = 7;
String messages[maxMessages];

void setup() {
  M5.begin();
  initDisplay();          
  Serial.begin(115200);   // Initialise la communication série
  wifiSetup();
}

void loop() {
  handleWiFi();
  handleClient();
  handleM5Updates();
  handleButtons();
}

// Initialisation de l'affichage sur l'écran LCD M5
void initDisplay() {
  // Configure la couleur du texte en blanc sur fond noir
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);

  // Définit le point de référence pour le texte au centre de l'écran
  M5.Lcd.setTextDatum(MC_DATUM);
}

// Configuration du Wi-Fi
void wifiSetup() {
  // Configure le point d'accès (AP) Wi-Fi avec le SSID et le mot de passe fournis
  WiFi.softAP(ssid, password);

  // Configure le point d'accès (AP) avec une adresse IP statique, une passerelle et un masque de sous-réseau
  WiFi.softAPConfig(staticIP, gateway, subnet);

  // Affiche sur l'écran LCD M5 que le point d'accès Wi-Fi est créé, avec l'adresse IP attribuée
  M5.Lcd.println("\nPoint d'accès Wi-Fi créé\nIP Address: " + WiFi.softAPIP());

  // Démarre le serveur Wi-Fi pour accepter les connexions
  server.begin();
}

// Fonction de gestion du Wi-Fi
void handleWiFi() {
  // Vérifie si le Wi-Fi n'est pas déjà activé
  if (!wifiEnabled) {
    // Crée une adresse IP nulle pour la configuration du point d'accès (AP)
    IPAddress nullIP(0, 0, 0, 0);

    // Configure le point d'accès (AP) avec des adresses IP nulles
    WiFi.softAPConfig(nullIP, nullIP, nullIP);

    // Arrête le serveur Wi-Fi
    server.stop();

    // Efface l'écran LCD M5 en le remplissant de noir
    M5.Lcd.fillScreen(TFT_BLACK);
  }
}

// Fonction de gestion d'une alerte
void handleAlert(String alertMessage, IPAddress clientIP) {
  // Trouve la position du premier espace dans le message d'alerte
  int start = alertMessage.indexOf(" ") + 1;

  // Trouve la position du deuxième espace dans le message d'alerte en commençant à partir de la position du premier espace
  int end = alertMessage.indexOf(" ", start);

  // Extrait le nom de l'AP à partir du message d'alerte
  String apName = alertMessage.substring(start, end);

  // Ajoute le message d'alerte au stockage
  addMessage("Seuil dépassé dans la pièce : " + apName +
             "\n De : " + clientIP.toString() + "\n\n");

  // Affiche le nom de l'AP en couleur rouge sur l'écran LCD M5
  M5.Lcd.fillScreen(TFT_RED);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_RED);
  M5.Lcd.drawString(apName, 160, 120);

  // Attend 1 seconde pour l'affichage
  delay(1000);
}

// Fonction d'ajout d'un nouveau message
void addMessage(String newMessage) {
  // Déplace chaque message vers le bas dans le tableau
  for (int i = maxMessages - 1; i > 0; i--) {
    messages[i] = messages[i - 1];
  }

  // Place le nouveau message à la première position du tableau
  messages[0] = newMessage;
}

bool authenticateSender(String encryptedMessage) {
  // Déchiffrer le message
  String decryptedMessage = messageDecryption(encryptedMessage);

  // Vérifier la présence de la signature dans le message déchiffré
  if (decryptedMessage.indexOf(expectedSignature) != -1) {
    return true;  // Authentification réussie
  } else {
    // Échec de l'authentification, afficher le message déchiffré
    M5.Lcd.println("Authentification échouée. Message déchiffré : " + decryptedMessage);
    Serial.println(decryptedMessage);
    return false;
  }
}

// Fonction de gestion du client
void handleClient() {
  // Vérifie si le Wi-Fi est activé
  if (wifiEnabled) {
    client = server.available();  // Accepte la connexion du client si disponible

    // Vérifie si un client est connecté
    if (client) {
      // Attend que des données soient disponibles ou que la connexion soit fermée
      while (client.connected() && !client.available()) {
        delay(1);
      }

      // Lit le message reçu jusqu'à rencontrer un saut de ligne
      String encryptedMessage = client.readStringUntil('\n');

      // Authentifie l'expéditeur du message
      if (authenticateSender(encryptedMessage)) {
        // Déchiffre le message reçu
        String decryptedMessage = messageDecryption(encryptedMessage);
        
        // Récupère l'adresse IP du client
        IPAddress clientIP = client.remoteIP();

        // Trouve la position de la signature dans le message déchiffré
        int signaturePos = decryptedMessage.indexOf(expectedSignature);

        // Vérifie si la signature est présente dans le message déchiffré
        if (signaturePos != -1) {
          // Extrait le message à afficher en excluant la signature
          String messageToDisplay = decryptedMessage.substring(signaturePos + expectedSignature.length());

          // Vérifie si le message est une alerte
          if (messageToDisplay.startsWith("Alerte!")) {
            // Gère l'alerte et affiche l'adresse IP du client
            handleAlert(messageToDisplay, clientIP);
          } else {
            // Ajoute le message au stockage et affiche les messages
            addMessage(messageToDisplay + "\n De : " + clientIP.toString() + "\n\n");
            displayMessages();
          }
        } else {
          // Signature non trouvée, ignore le message
          client.stop();
        }
      } else {
        // Échec de l'authentification, ignore le message
        client.stop();
      }
    }
  }
}

void handleM5Updates() {
  M5.update();
}

void handleButtons() {
  if (M5.BtnB.wasPressed()) {
    wifiEnabled = false;
  }

  if (M5.BtnC.wasPressed()) {
    wifiEnabled = true;
    wifiSetup();
  }
}

// Fonction d'affichage des messages sur l'écran LCD M5
// Efface l'écran, configure la taille du texte et la couleur, puis affiche les messages stockés
void displayMessages() {
  
  M5.Lcd.fillRect(0, 0, 320, 240, TFT_BLACK);   // Efface l'écran en remplissant avec la couleur noire

  // Configure la taille du texte et la couleur du texte
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);

  M5.Lcd.setCursor(0, 0);   // Positionne le curseur en haut à gauche de l'écran

  // Boucle à travers les messages stockés et les affiche sur l'écran
  for (int i = 0; i < maxMessages; i++) {
    M5.Lcd.printf("Message : %s\n", messages[i].c_str());
  }
}

// Fonction de déchiffrement du message
// Prend en entrée le message à déchiffrer (messageToDecrypt)
// Retourne le message déchiffré en décalant chaque caractère par la clé de chiffrement (cryptingKey)
String messageDecryption(String messageToDecrypt) {
  String decryptedMessage = "";   // Initialise une chaîne de caractères pour stocker le message déchiffré

  // Boucle à travers chaque caractère du message à déchiffrer
  for (char currentChar : messageToDecrypt) {
    decryptedMessage += char(int(currentChar) - cryptingKey);   // Déchiffre le caractère en décalant sa valeur ASCII par la clé de chiffrement
  }

  return decryptedMessage;    // Retourne le message déchiffré
}
