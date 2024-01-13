#include <M5Stack.h> // Inclut la bibliothèque pour le M5Stack
#include <WiFi.h> // Inclut la bibliothèque pour la gestion du WiFi
#include <lmic.h> // Inclut la bibliothèque pour LoRaWAN
#include <hal/hal.h> // Inclut la bibliothèque pour le matériel LoRaWAN
#include <SPI.h> // Inclut la bibliothèque pour le bus SPI
#include <Ticker.h> // Inclut la bibliothèque Ticker pour la gestion des temporisateurs

Ticker wifiScanTimer; // Déclare un objet Ticker pour gérer un temporisateur

// Définition des clés pour LoRaWAN
static const u1_t PROGMEM APPEUI[8]={ 0x0C, 0x7E, 0x45, 0x01, 0x02, 0x03, 0x03, 0x3B };
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

static const u1_t PROGMEM DEVEUI[8]={ 0x3B, 0x03, 0x03, 0x02, 0x01, 0x45, 0x7E, 0x0C };
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

static const u1_t PROGMEM APPKEY[16] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x03, 0x3B };
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

// Configuration des broches pour LoRa
const lmic_pinmap lmic_pins = {   
  .nss = 5, 
  .rxtx = LMIC_UNUSED_PIN, 
  .rst = 26, 
  .dio = {36, 35, 36}
};

// Configuration des paramètres WiFi
const char *ssid = "Montre";
const char *password = "MotDePasseVraimentDureJazz";
const int cryptingKey = 24;
String expectedSignature = "Signature super securise jazz";

// Configuration de l'adresse IP statique pour le WiFi
IPAddress staticIP(192, 168, 1, 10);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// Création d'un serveur et d'un client WiFi
WiFiServer server(80);
WiFiClient client;

// Déclaration des états des boutons
bool etatBoutonA = true;
bool etatBoutonB = false;

// Structure pour stocker des données extraites
struct ExtractedData {
  String ssid;
  String temp; 
  String humidity; 
  String humidex;
};

// Variable pour stocker les données extraites
struct ExtractedData messageExtractedData;

// Liste des SSID autorisés
String authorizedSSIDs[] = {"sdr","sdb","chambre","sallon","cuisine","wc"};

// Initialisation des Données à envoyer via LoRawan 
static uint8_t mydata[] = "ceci est la chaîne initiale qui sera modifié dès que l'on aura reçu nos premières données par un AP Wifi connecté";
static osjob_t sendjob;

// Intervalle d'envoi des données via LoRawan
const unsigned TX_INTERVAL = 45;
String etatAlerte = "0";

// Flag pour indiquer si les données sont prêtes au niveau de la récupération de données par wifi
bool dataReady = false;

// Déclaration des fonctions
void wifiSetup();
void wifiScanAndSend();
void printHex2(unsigned v);
void onEvent (ev_t ev);
String messageDecryption(String messageToDecrypt);
bool authenticateSender(String encryptedMessage);
void handleAlert(String alertMessage, IPAddress clientIP) ;
String getRoomCoordinates();
struct ExtractedData extraireInfoDuMessage(String messageToDisplay);
void handleClient();
void sendLora(osjob_t* j);
void wifiScanAndSend();
void affichageDesactivationAlarme();
void affichageEtatInitial(String nomPiece, String temperature, String humidite, String humindex);
void affichageAlarmeDeclenchee();


// Fonction setup, exécutée au démarrage de la carte
void setup() {
    M5.begin(); // Initialise le M5Stack
    Serial.begin(115200); // Initialise la communication série à 115200 bauds
    wifiSetup(); // Appelle la fonction de configuration du WiFi
    
    // Attente de 5 secondes au démarrage
    while (millis() < 5000) {}
    
    Serial.println(F("Starting"));
    
    // Initialisation de LoRaWAN
    os_init(); // Initialise le stack LoRaWAN
    LMIC_reset(); // Réinitialise la configuration LoRaWAN
    delay(5000); // Délai pour s'assurer que LoRaWAN est prêt
    handleClient(); // Gère les requêtes clients
    // Boucle d'attente jusqu'à ce que les données soient prêtes
    while (dataReady != true) {
        delay(1000);
    }
    // Affiche l'état initial sur l'écran après réception des données
    affichageEtatInitial(messageExtractedData.ssid, messageExtractedData.temp, messageExtractedData.humidity, messageExtractedData.humidex);
    dataReady = false; // Réinitialise le drapeau de données prêtes
    wifiScanTimer.attach(45, wifiScanAndSend); // Configure le temporisateur pour scanner et envoyer les données toutes les 45 secondes
}

// Boucle principale, exécutée en continu après le setup
void loop() {
    etatAlerte = "0"; // Réinitialise l'état d'alerte
    M5.update(); // Met à jour l'état du M5Stack
    
    // Gestion de l'état du bouton A
    if (etatBoutonA && M5.BtnA.wasPressed()) {
    etatBoutonA = false;
    etatBoutonB = true;
    M5.Speaker.tone(440, 200); // Émet un son
    affichageAlarmeDeclenchee(); // Affiche l'alarme déclenchée
    // Attente de 5 secondes pour la pression du bouton B
    unsigned long debut = millis();
    while (millis() - debut < 5000) {
      M5.update();
      if (etatBoutonB && M5.BtnB.wasPressed()) {
        etatBoutonA = true;
        etatBoutonB = false;
        affichageDesactivationAlarme(); // Affiche la désactivation de l'alarme
        affichageEtatInitial(messageExtractedData.ssid, messageExtractedData.temp, messageExtractedData.humidity, messageExtractedData.humidex);
        Serial.println("Annulé !");
        break;
      }
    }
    
    // Si le bouton B n'a pas été pressé dans les 5 secondes
    if (etatBoutonB) {
      etatAlerte = "1"; // Met à jour l'état d'alerte
      handleClient(); // Gère les requêtes clients
      sendLora(&sendjob); // Envoie les données via LoRa
      etatBoutonA = true;
      etatBoutonB = false;
      affichageEtatInitial(messageExtractedData.ssid, messageExtractedData.temp, messageExtractedData.humidity, messageExtractedData.humidex); // Affiche l'état initial
    }
  }
  os_runloop_once(); // Exécute une itération de la boucle d'événements LoRaWAN
}


// Fonction pour afficher une valeur hexadécimale
void printHex2(unsigned v) {
v &= 0xff; // Assure que la valeur est sur 8 bits
if (v < 16) {
Serial.print('0'); // Ajoute un zéro pour les valeurs inférieures à 16 (format hexadécimal)
}
Serial.print(v, HEX); // Affiche la valeur en hexadécimal
}

// Gestion des événements LoRaWAN
// Switch pour différents types d'événements LoRaWAN
void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));        
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            {
              u4_t netid = 0;
              devaddr_t devaddr = 0;
              u1_t nwkKey[16];
              u1_t artKey[16];
              LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
              Serial.print("netid: ");
              Serial.println(netid, DEC);
              Serial.print("devaddr: ");
              Serial.println(devaddr, HEX);
              Serial.print("AppSKey: ");
              for (size_t i=0; i<sizeof(artKey); ++i) {
                if (i != 0)
                {
                  Serial.print("-");
                }
                printHex2(artKey[i]);
              }
              Serial.println("");
              Serial.print("NwkSKey: ");
              for (size_t i=0; i<sizeof(nwkKey); ++i) {
                      if (i != 0)
                        {
                              Serial.print("-");
                        }                    
                      printHex2(nwkKey[i]);
              }
              Serial.println();
            }
            LMIC_setLinkCheckMode(0);
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            if (LMIC.dataLen) {
              Serial.print(F("Received "));
              Serial.print(LMIC.dataLen);
              Serial.println(F(" bytes of payload"));
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), sendLora);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE "));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
        case EV_TXSTART:
            Serial.println(F("EV_TXSTART"));
            break;
        case EV_TXCANCELED:
            Serial.println(F("EV_TXCANCELED"));
            break;
        case EV_JOIN_TXCOMPLETE:
            Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
            break;

        default:
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned) ev);
            break;
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



// Configuration du Wi-Fi
void wifiSetup() {
  // Configure le point d'accès (AP) Wi-Fi avec le SSID et le mot de passe fournis
  WiFi.softAP(ssid, password);

  // Configure le point d'accès (AP) avec une adresse IP statique, une passerelle et un masque de sous-réseau
  WiFi.softAPConfig(staticIP, gateway, subnet);

  // Démarre le serveur Wi-Fi pour accepter les connexions
  server.begin();
}



// Fonction de gestion d'une alerte
void handleAlert(String alertMessage, IPAddress clientIP) {
  // Trouve la position du premier espace dans le message d'alerte
  int start = alertMessage.indexOf(" ") + 1;

  // Trouve la position du deuxième espace dans le message d'alerte en commençant à partir de la position du premier espace
  int end = alertMessage.indexOf(" ", start);

  // Extrait le nom de l'AP à partir du message d'alerte
  String apName = alertMessage.substring(start, end);

  // Affiche le nom de l'AP en couleur rouge sur l'écran LCD M5
  M5.Lcd.clear();
  M5.Lcd.fillScreen(TFT_RED);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_RED);
  M5.Lcd.drawString(apName, 160, 120);
}



// Fonction pour obtenir les coordonnées de la pièce actuelle
String getRoomCoordinates() {
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

  String currentRoom;
  currentRoom = ssid;
  //Cela permet de garantir que currentRoom a toujours des valeurs définies, même si le SSID ne correspond pas à une pièce répertoriée

  return currentRoom;
}


///////////////////////////////////////////////////////////////////////// Fonction pour extraire la température, humidité et ssid du message reçus
struct ExtractedData extraireInfoDuMessage(String messageToDisplay) {
  ExtractedData data;

  int startSsid = messageToDisplay.indexOf("SSID :") + 6;

  int startTemp = messageToDisplay.indexOf("Température : ") + 13;

  int startHumidity = messageToDisplay.indexOf("Humidity : ") + 11;

  int startHumidex = messageToDisplay.indexOf("Humidex : ") + 10;

  data.ssid = messageToDisplay.substring(startSsid, startTemp - 13);
  data.ssid.trim();

  int endTemp = messageToDisplay.indexOf("°C", startTemp); 
  data.temp = messageToDisplay.substring(startTemp + 2, endTemp);
  data.temp.trim();

  int endHumidity = messageToDisplay.indexOf("%", startHumidity); 
  data.humidity = messageToDisplay.substring(startHumidity, endHumidity);
  data.humidity.trim();

  int endHumidex = messageToDisplay.indexOf(" Message", startHumidex); 
  data.humidex = messageToDisplay.substring(startHumidex, endHumidex);
  data.humidex.trim();

  return data;
}


void handleClient() {
  client = server.available();  // Accepte la connexion du client si disponible

  // Vérifie si un client est connecté
  if (client) {
    // Attend que des données soient disponibles ou que la connexion soit fermée

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
        String currentRoom = getRoomCoordinates();          // Obtient le SSID de la pièce actuelle 
        // On va mettre cette variable en global pour qu'elle soit utilisé par la fonction envoi Lora. 
        // Elle set ici pour ne pas entré en concurence avec le boutton. La vitesse d'éxecution de la loop et le temps d'éxecution de la fonction de localisation
        // Rendent impossible la présence de la fonction de localisation autre part qu'ici.
        // Gère l'alerte et affiche l'adresse IP du client
        handleAlert(messageToDisplay, clientIP);
      } else {
        // Second endroit qui ne détruit pas le fonctionnement des boutons
        String currentRoom = getRoomCoordinates();
        // Vérifie si le SSID du message correspond au SSID de la pièce actuelle
        if (messageToDisplay.startsWith("SSID :" + currentRoom)) {
          // On extraie les données dans une fonction global pour pouvoir les envoyés en Lora, par alerte ou message normal
          messageExtractedData = extraireInfoDuMessage(messageToDisplay);
        } else {
          Serial.println("\nLe message ne provient pas de la pièce la plus proche. Ignoré.");
          Serial.printf("\n%s", currentRoom);
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
  dataReady = true;
}



void sendLora(osjob_t* j) {
  if (LMIC.opmode & OP_TXRXPEND) 
  {
      Serial.println("OP_TXRXPEND, not sending ");
  } 
  else {
      

      //snprintf((char*)mydata, sizeof(mydata), "piece:%s, temp:%s, hum:%s, humindex:%s, bouton:%s", messageExtractedData.ssid.c_str(), messageExtractedData.temp.c_str(), messageExtractedData.humidity.c_str(), messageExtractedData.humidex.c_str(), etatAlerte.c_str());
    snprintf((char*)mydata, sizeof(mydata), "{\"piece\":\"%s\", \"temp\":\"%s\", \"hum\":\"%s\", \"humindex\":\"%s\", \"bouton\":\"%s\"}", 
         messageExtractedData.ssid.c_str(), 
         messageExtractedData.temp.c_str(), 
         messageExtractedData.humidity.c_str(), 
         messageExtractedData.humidex.c_str(), 
         etatAlerte.c_str());

    

      Serial.println(("%d", etatAlerte));
      Serial.println((char*)mydata);

      LMIC_setTxData2(1, mydata, strlen((char*)mydata), 0);

      //sqn++;
      
      //LMIC_setTxData2(1, mydata, sizeof(mydata)-1, 0);
      Serial.println(F("Packet queued"));
  }
}



void wifiScanAndSend() {
  handleClient();
  // ... Code pour scanner les réseaux WiFi ...

  affichageEtatInitial(messageExtractedData.ssid, messageExtractedData.temp, messageExtractedData.humidity, messageExtractedData.humidex);
  // Envoyer les données via LoRaWAN
  sendLora(&sendjob);
}



// Initialisation de l'affichage sur l'écran LCD M5
/*void initDisplay() {
  M5.Lcd.clear();
  // Configure la couleur du texte en blanc sur fond noir
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);

  // Définit le point de référence pour le texte au centre de l'écran
  M5.Lcd.setTextDatum(MC_DATUM);
}
*/

void affichageDesactivationAlarme() {
  M5.Lcd.clear();
  M5.Lcd.setCursor(60, 110);
  M5.Lcd.print("L'Alarme a bien ete desactivee");
}


void affichageEtatInitial(String nomPiece, String temperature, String humidite, String humindex) {
  M5.Lcd.clear();
  //float batterie = M5.Power.getBatteryLevel();   => affichage de la batterie restante, mais pas fonctionnel avec Lora

    // Effacer uniquement la zone où le SSID est affiché
    M5.Lcd.fillRect(0, 0, 320, 50, BLACK); // Ajustez les dimensions au besoin

    // Afficher le SSID et les autres informations
    M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    M5.Lcd.drawString(String(nomPiece), 240, 10, 4);

    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Lcd.setTextDatum(TL_DATUM);
    M5.Lcd.setTextSize(1.5);
    //M5.Lcd.drawString("Batt: " + String(batterie) + "%", 10, 10, 2);

    M5.Lcd.drawFastVLine(100, 0, 50, WHITE);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextSize(2);
    M5.Lcd.drawLine(0, 50, 320, 50, WHITE);

    M5.Lcd.drawString("Temp:", 80, 80, 2); 
    M5.Lcd.drawString(String(temperature) + "C", 240, 80, 2); 
    M5.Lcd.drawString("Hum:", 80, 120, 2); 
    M5.Lcd.drawString(String(humidite) + "%", 240, 120, 2); 
    M5.Lcd.drawString("HumIndex:", 80, 160, 2); 
    M5.Lcd.drawString(String(humindex), 240, 160, 2); 
    M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
    M5.Lcd.drawString("Alerte", 65, 220, 2);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);

}




/*void affichageAlarmeDeclenchee() {
    M5.Lcd.clear();

    // Clignotement de l'écran avec des couleurs vives
    for (int i = 0; i < 5; i++) {
        M5.Lcd.fillScreen(RED); // Écran en rouge
        M5.Lcd.setTextColor(WHITE); // Texte en blanc pour contraste
        M5.Lcd.setTextSize(2.5); // Taille de texte plus grande
        M5.Lcd.setCursor(20, 110);
        M5.Lcd.print("Alarme declenchee !");
        M5.Lcd.setTextSize(1.2); // Taille de texte plus grande
         M5.Lcd.setCursor(90, 210);
    M5.Lcd.print("Desactivation Alarme");

        unsigned long debut = millis();
        while (millis() - debut < 500) {
        }

        M5.Lcd.fillScreen(BLACK); // Écran en noir
        M5.Lcd.setTextColor(RED); // Texte en rouge pour contraste
        M5.Lcd.setTextSize(2.5); // Taille de texte plus grande

        M5.Lcd.setCursor(20, 110);
        M5.Lcd.print("Alarme declenchee !");
        M5.Lcd.setTextSize(1.2); // Taille de texte plus grande
         M5.Lcd.setCursor(90, 210);
    M5.Lcd.print("Desactivation Alarme");

        unsigned long fin = millis();
        while (millis() - fin < 500) {
        }
    }  
}*/

void affichageAlarmeDeclenchee() {
  M5.Lcd.clear();
  for (int i = 0; i < 5; i++) {
    M5.Lcd.fillScreen(BLACK); // Écran en noir
    M5.Lcd.setTextColor(RED); // Texte en rouge pour contraste
    M5.Lcd.setTextSize(2.5); // Taille de texte plus grande

    M5.Lcd.setCursor(20, 110);
    M5.Lcd.print("Alarme declenchee !");
    M5.Lcd.setTextSize(1.2); // Taille de texte plus grande
    M5.Lcd.setCursor(90, 210);
    M5.Lcd.print("Desactivation Alarme");
  }
}
