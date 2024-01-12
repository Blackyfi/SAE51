#include <M5Stack.h>
#include <WiFi.h>
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

// Ajout des définitions pour LoRa
static const u1_t PROGMEM APPEUI[8] = {0x0C, 0x7E, 0x45, 0x01, 0x02, 0x03, 0x03, 0x03}; // 8 octets
void os_getArtEui(u1_t* buf) { memcpy_P(buf, APPEUI, 8); }

static const u1_t PROGMEM DEVEUI[8] = { 0x40, 0x03, 0x03, 0x02, 0x01, 0x45, 0x7E, 0x0C };
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

static const u1_t PROGMEM APPKEY[16] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x03, 0x40 };
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}


const lmic_pinmap lmic_pins = 
{   .nss = 5, 
    .rxtx = LMIC_UNUSED_PIN, 
    .rst = 26, 
    .dio = {36, 35, 36} };


const char *ssid = "Montre";
const char *password = "MotDePasseVraimentDureJazz";
const int cryptingKey = 24;
String expectedSignature = "Signature super securise jazz";

IPAddress staticIP(192, 168, 1, 10);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

WiFiServer server(80);
WiFiClient client;

const int maxMessages = 7;
String messages[maxMessages];

// Définition de l'état des boutons
bool etatBoutonA = true;
bool etatBoutonB = false;

// Structure pour stocker les informations de la pièce
struct Room {
  String ssid;
  int x; // Coordonnée x de la pièce
  int y; // Coordonnée y de la pièce
};

struct ExtractedData {
  String ssid;
  String temp; 
  String humidity; 
  String humidex;
};

ExtractedData messageExtractedData;

// Liste des pièces avec leurs informations
Room rooms[] = {
  {"Salon", 3, 6},
  {"florent", 8, 8},
  {"Gryffondor", 3, 2},
  // Ajoutez autant de pièces que nécessaire
};

String authorizedSSIDs[] = {"Salon", "florent", "Gryffondor"}; // Ajoutez les SSIDs que vous souhaitez suivre

void setup() {
  M5.begin();
  initDisplay();          
  Serial.begin(115200);   // Initialise la communication série
  wifiSetup();
  M5.Speaker.begin();
  
  // Initialisation LoRaWAN
  os_init();
  LMIC_reset();
}

void loop() {
  M5.update();
  affichageEtatInitial(messageExtractedData.ssid, messageExtractedData.temp, messageExtractedData.humidity, messageExtractedData.humidex);
  // Gestion de l'état du bouton A
  if (etatBoutonA && M5.BtnA.wasPressed()) {
    etatBoutonA = false;
    etatBoutonB = true;
    M5.Speaker.tone(440, 200); // Joue un son (440 Hz pendant 200 ms)
    affichageAlarmeDeclenchee();

    // Attendre 5 secondes pour la pression du bouton B
    unsigned long debut = millis();
    while (millis() - debut < 5000) {
      M5.update();
      if (etatBoutonB && M5.BtnB.wasPressed()) {
        etatBoutonA = true;
        etatBoutonB = false;
        affichageDesactivationAlarme();
        affichageEtatInitial(messageExtractedData.ssid, messageExtractedData.temp, messageExtractedData.humidity, messageExtractedData.humidex);
        Serial.println("Annulé !");
        break;
      }
    }

    // Si le bouton B n'a pas été pressé dans les 5 secondes
    if (etatBoutonB) {
      Room currentRoom = getRoomCoordinates();
      alarme(currentRoom);
      etatBoutonA = true;
      etatBoutonB = false;
      affichageEtatInitial(messageExtractedData.ssid, messageExtractedData.temp, messageExtractedData.humidity, messageExtractedData.humidex); // Exemple de valeurs
      delay(5000); // Affiche le message pendant 5 secondes
    }
  }
  handleClient();
  
  os_runloop_once(); // Ajouté pour la boucle LoRa
}

////////////////////////////////////////////////////////////////// Edwin : y a pleins de trucs qui vont pas, la fonction lora elle devrait s'appeler sendLora avec une donnée en plus de la room (du reste pas de x et y qui faudra enlever partout dans le code), qui sera une string alarme ("0" ou "1"), et qui en fonction de si "1" ou "0", si on a 1 c'est que c'est une alarme, sinon c'est que c'est un envoie basique des données de temp hum  pour remplir l'historique du site toutes les genre 5secondes (ou à chaque fois qu'il reçoit un message si on configure les capteurs pour émettre que toutes les 5s). 
//De plus, il faudra changer l'affichage avec celui de ny avo que je trouve beaucoup plus beau 
// il faudra faire le point sur l'envoie des données des capteurs, et le traitement des alarmes suite à température ou humindex (ou batterie mais ça c'est un sujet différent)
//je fais faire un doc dans le dossier gateway lora avec un test d'envoie basique sur MQTT d'un message qui ressemblera au notre avec des données, et normalement Ny Avo devra le tester demain matin avec un tuto MQTT explorer que je vais aussi mettre dans le dossier
//R404 partie Lora à faire attention => Réduisez le délai entre 2 émissions pour respectez la législation sub-gigahertz d’un rapport cyclique de 1%. Expliquez pourquoi !

void alarme(Room currentRoomToSend) {
    
  // Formatage des données à envoyer
  sprintf ((char*)dataToSend, "{\"Alarme\":\"%s\", \"ssid\":\"%s\", \"temp\":\"%s\", \"humidity\":\"%s\", \"humidex\":\"%s\"}", , (char*)!!!!!stringalarmeaparamétrer!!!!!, (char*)currentRoomToSend.ssid, (char*)messageExtractedData.temp, (char*)messageExtractedData.humidity, (char*)messageExtractedData.humidex ) 
  
  // Envoyer les données via LoRaWAN
  LMIC_setTxData2(1, dataToSend, strlen((char*)dataToSend), 0);
}


void affichageDesactivationAlarme() {
  M5.Lcd.clear();
  M5.Lcd.setCursor(60, 110);
  M5.Lcd.print("L'Alarme a bien été désactivée");
  delay(2000); // Affichage pendant 2 secondes
}

void affichageEtatInitial(String nomPiece, String temperature, String humidite, String humindex) {
  M5.Lcd.clear();
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.print("Nom de la pièce: " + nomPiece);
  M5.Lcd.setCursor(10, 60);
  M5.Lcd.print("Température: " + String(temperature) + " °C");
  M5.Lcd.setCursor(210, 60);
  M5.Lcd.print("Humidité: " + String(humidite) + " %");
  M5.Lcd.setCursor(10, 110);
  M5.Lcd.print("Humindex: " + String(humindex));
  M5.Lcd.setCursor(10, 210);
  M5.Lcd.print("Activation Alarme");
}

void affichageAlarmeDeclenchee() {
  for (int i = 0; i < 5; i++) {
    M5.Lcd.clear(RED);
    M5.Lcd.setCursor(60, 110);
    M5.Lcd.print("Alarme déclenchée !");
    M5.Lcd.setCursor(70, 210);
    M5.Lcd.print("Désactivation Alarme");
    delay(500);
    M5.Lcd.clear();
    delay(500);
  }
}

////////////////////////////////////////////////////////////////// Edwin FIN

/////////////////////////////////////////////////////////////////////////Loca Debut

// Fonction pour obtenir les coordonnées de la pièce actuelle
Room getRoomCoordinates() {
  String ssid = "";
  int rssi = 0;

  int numberOfNetworks = WiFi.scanNetworks(); // Analyse des réseaux Wi-Fi disponibles et obtention du nombre

  if (numberOfNetworks != 0) {
    int strongestSignal = -100; // Initialise strongestSignal à une valeur basse
    for (int i = 0; i < numberOfNetworks; i++) {
      String currentSSID = WiFi.SSID(i); // Obtient le SSID du réseau actuel
      int currentRssi = WiFi.RSSI(i); // Obtient le RSSI du réseau actuel

      // Vérifie si le SSID actuel est dans la liste des pièces
      for (int j = 0; j < sizeof(authorizedSSIDs) / sizeof(authorizedSSIDs[0]); j++) {
        if (currentSSID.equals(authorizedSSIDs[j]) && currentRssi > strongestSignal) {
          // Si le réseau actuel a un signal plus fort et est dans la liste des pièces, mettez à jour strongestSignal, ssid et rssi
          strongestSignal = currentRssi;
          ssid = currentSSID; // Stocke le SSID du réseau le plus fort
          rssi = currentRssi; // Stocke le RSSI du réseau le plus fort
        }
      }
    }
  }

  Room currentRoom;
  currentRoom.ssid = ssid;
  currentRoom.x = -1; // Valeur par défaut si le SSID n'est pas trouvé
  currentRoom.y = -1; // Valeur par défaut si le SSID n'est pas trouvé
  //Cela permet de garantir que currentRoom a toujours des valeurs définies, même si le SSID ne correspond pas à une pièce répertoriée

  // Obtient les coordonnées de la pièce actuelle
  for (int i = 0; i < sizeof(rooms) / sizeof(rooms[0]); i++) {
    if (ssid.equals(rooms[i].ssid)) {
      currentRoom.x = rooms[i].x;
      currentRoom.y = rooms[i].y;
      break; // Sort de la boucle une fois que la correspondance est trouvée
    }
  }

  return currentRoom;
}

/////////////////////////////////////////////////////////////////////////Loca FIN

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

// Fonction de gestion d'une alerte
void handleAlert(String alertMessage, IPAddress clientIP, Room alertRoom) {
  // Trouve la position du premier espace dans le message d'alerte
  int start = alertMessage.indexOf(" ") + 1;

  // Trouve la position du deuxième espace dans le message d'alerte en commençant à partir de la position du premier espace
  int end = alertMessage.indexOf(" ", start);

  // Extrait le nom de l'AP à partir du message d'alerte
  String apName = alertMessage.substring(start, end);

  // Ajoute le message d'alerte au stockage
  addMessage("Seuil dépassé dans la pièce : " + apName + "\n De : " + clientIP.toString() + "\n\n");

  // Affiche le nom de l'AP en couleur rouge sur l'écran LCD M5
  M5.Lcd.fillScreen(TFT_RED);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_RED);
  M5.Lcd.drawString(apName, 160, 120);

  alarme(alertRoom);

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
    Serial.println(decryptedMessage);
    return false;
  }
}

void handleClient() {
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

      // Trouve la position du dernier caractère dans la chaîne déchiffrée
      int lastCharPos = decryptedMessage.length() - 1;

      // Extrait la sous-chaîne jusqu'au dernier caractère
      String messageToDisplay = decryptedMessage.substring(signaturePos + expectedSignature.length(), lastCharPos);
      
      // Vérifie si le message est une alerte
      if (messageToDisplay.startsWith("Alerte!")) {
        Room currentRoom = getRoomCoordinates();          // Obtient le SSID de la pièce actuelle 
        // On va mettre cette variable en global pour qu'elle soit utilisé par la fonction envoi Lora. 
        // Elle set ici pour ne pas entré en concurence avec le boutton. La vitesse d'éxecution de la loop et le temps d'éxecution de la fonction de localisation
        // Rendent impossible la présence de la fonction de localisation autre part qu'ici.
        // Gère l'alerte et affiche l'adresse IP du client
        handleAlert(messageToDisplay, clientIP, currentRoom);
      } else {
        // Second endroit qui ne détruit pas le fonctionnement des boutons
        Room currentRoom = getRoomCoordinates();
        // Vérifie si le SSID du message correspond au SSID de la pièce actuelle
        if (messageToDisplay.startsWith("SSID :" + currentRoom.ssid)) {
          // Ajoute le message au stockage et affiche les messages
          addMessage(messageToDisplay + "\n De : " + clientIP.toString() + "\n\n");
          displayMessages();
          // On extraie les données dans une fonction global pour pouvoir les envoyés en Lora, par alerte ou message normal
          messageExtractedData = extraireInfoDuMessage(messageToDisplay);
        } else {
          Serial.println("\nLe message ne provient pas de la pièce la plus proche. Ignoré.");
          Serial.printf("\n%s", currentRoom.ssid);
        }
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




//code batterie + alarme batterie à ajouter ici

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

///////////////////////////////////////////////////////////////////////// Fonction pour extraire la température, humidité et ssid du message reçus
ExtractedData extraireInfoDuMessage(String messageToDisplay) {
  ExtractedData data;

  // Find the position of "SSID :"
  int startSsid = messageToDisplay.indexOf("SSID :") + 6;

  // Find the position of "Température : "
  int startTemp = messageToDisplay.indexOf("Température : ") + 13;

  // Find the position of "Humidity : "
  int startHumidity = messageToDisplay.indexOf("Humidity : ") + 11;

  // Find the position of "Humidex : "
  int startHumidex = messageToDisplay.indexOf("Humidex : ") + 10; // le nombre après le "+" correspond à la longueur du repère pour trouver le repère de début

  // Extract SSID
  data.ssid = messageToDisplay.substring(startSsid, startTemp - 13);
  data.ssid.trim();

  // Extract Temperature
  int endTemp = messageToDisplay.indexOf("°C", startTemp); // Find the position of "°C"
  data.temp = messageToDisplay.substring(startTemp + 2, endTemp);
  data.temp.trim();

  // Extract Humidity
  int endHumidity = messageToDisplay.indexOf("%", startHumidity); // Find the position of "Humidex : "
  data.humidity = messageToDisplay.substring(startHumidity, endHumidity);
  data.humidity.trim();

  // Extract Humidex
  int endHumidex = messageToDisplay.indexOf(" Message", startHumidex); // Find the position of "°C"
  data.humidex = messageToDisplay.substring(startHumidex, endHumidex);
  data.humidex.trim();

  Serial.printf("\n%s", messageToDisplay.c_str());
  Serial.printf("\n%s", data.ssid.c_str());
  Serial.printf("\n%s", data.temp.c_str());
  Serial.printf("\n%s", data.humidity.c_str());
  Serial.printf("\n%s", data.humidex.c_str());

  return data;
}
///////////////////////////////////////////////////////////////////////// Fonction pour extraire la température, humidité et ssid du message reçus


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

