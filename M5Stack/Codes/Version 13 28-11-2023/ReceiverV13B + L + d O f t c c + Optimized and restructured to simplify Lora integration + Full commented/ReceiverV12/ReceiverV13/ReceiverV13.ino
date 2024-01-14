#include <M5Stack.h>
#include <WiFi.h>
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

// Ajout des définitions pour LoRa
static const u1_t PROGMEM APPEUI[8]={ 0x0C, 0x7E, 0x45, 0x01, 0x02, 0x03, 0x03, 0x3B  };         // 8 octets  
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

//0x0C, 0x7E, 0x45, 0x01, 0x02, 0x03, 0x03, 0x32    nouvelle clé si besoin, c'est la 50 
// This should also be in little endian format, see above.

static const u1_t PROGMEM DEVEUI[8]={ 0x3B, 0x03, 0x03, 0x02, 0x01, 0x45, 0x7E, 0x0C };    // la 60 dans le tableau
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

//0x32, 0x0C, 0x03, 0x02, 0x01, 0x45, 0x7E, 0x0C
// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.

static const u1_t PROGMEM APPKEY[16] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x03, 0x3B }; //16 octets
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

//0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x03, 0x32


const lmic_pinmap lmic_pins = {   
  .nss = 5, 
  .rxtx = LMIC_UNUSED_PIN, 
  .rst = 26, 
  .dio = {36, 35, 36}
};

const char *ssid = "Montre";
const char *password = "MotDePasseVraimentDureJazz";
const int cryptingKey = 24;
String expectedSignature = "Signature super securise jazz";

IPAddress staticIP(192, 168, 1, 10);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

WiFiServer server(80);
WiFiClient client;

// Définition de l'état des boutons
bool etatBoutonA = true;
bool etatBoutonB = false;

struct ExtractedData {
  String ssid;
  String temp; 
  String humidity; 
  String humidex;
};

struct ExtractedData messageExtractedData;

String authorizedSSIDs[] = {"sdr","sdb","chambre","sallon","cuisine","wc"}; // Ajoutez les SSIDs que vous souhaitez suivre

static uint8_t mydata[] = "init";
static osjob_t sendjob;
const unsigned TX_INTERVAL = 10;

void setup() {
  M5.begin();
  initDisplay();          
  Serial.begin(115200);   // Initialise la communication série
  wifiSetup();
  M5.Speaker.begin();
  
  while (millis() < 5000) {
    Serial.print("millis() = "); Serial.println(millis());
    delay(500);
    }
    Serial.println(F("Starting"));
    M5.Lcd.println("Starting ");
  
// Initialisation LoRaWAN
    #ifdef VCC_ENABLE
        // For Pinoccio Scout boards
        pinMode(VCC_ENABLE, OUTPUT);
        digitalWrite(VCC_ENABLE, HIGH);
        delay(1000);
    #endif

    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();
}

void loop() {
  M5.update();
  handleClient();
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
      String currentRoom = getRoomCoordinates();
      sendLora("1", &sendjob);
      etatBoutonA = true;
      etatBoutonB = false;
      affichageEtatInitial(messageExtractedData.ssid, messageExtractedData.temp, messageExtractedData.humidity, messageExtractedData.humidex); // Exemple de valeurs
      delay(5000); // Affiche le message pendant 5 secondes
    }
  }
  sendLora("0", &sendjob);
  os_runloop_once(); // Ajouté pour la boucle LoRa
}

/*/////////////////////////////////////////////////////////////////// Edwin 
// il faudra faire le point sur l'envoie des données des capteurs, et le traitement des alarmes suite à température ou humindex (ou batterie mais ça c'est un sujet différent)
//je fais faire un doc dans le dossier gateway lora avec un test d'envoie basique sur MQTT d'un message qui ressemblera au notre avec des données, et normalement Ny Avo devra le tester demain matin avec un tuto MQTT explorer que je vais aussi mettre dans le dossier
//R404 partie Lora à faire attention => Réduisez le délai entre 2 émissions pour respectez la législation sub-gigahertz d’un rapport cyclique de 1%. Expliquez pourquoi !
*/

void printHex2(unsigned v) {
    v &= 0xff;
    if (v < 16)
      {
        Serial.print('0');
        M5.Lcd.print("0");
      }  
    Serial.print(v, HEX);
    M5.Lcd.println(v);
}

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    M5.Lcd.print(os_getTime());
    Serial.print(": ");
    M5.Lcd.println(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            M5.Lcd.println("EV_SCAN_TIMEOUT ");
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            M5.Lcd.println("EV_BEACON_FOUND ");
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            M5.Lcd.print("EV_BEACON_MISSED ");          
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            M5.Lcd.println("EV_BEACON_TRACKED ");
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            M5.Lcd.println("EV_JOINING ");
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            M5.Lcd.println("EV_JOINED ");
            {
              u4_t netid = 0;
              devaddr_t devaddr = 0;
              u1_t nwkKey[16];
              u1_t artKey[16];
              LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
              Serial.print("netid: ");
              M5.Lcd.print("netid: ");
              Serial.println(netid, DEC);
              M5.Lcd.print(netid);
              M5.Lcd.print(" ");
              Serial.print("devaddr: ");
              M5.Lcd.print("devaddr: ");
              Serial.println(devaddr, HEX);
              M5.Lcd.print(devaddr);
              M5.Lcd.print(" ");
              Serial.print("AppSKey: ");
              M5.Lcd.print("AppSKey: ");
              for (size_t i=0; i<sizeof(artKey); ++i) {
                if (i != 0)
                {
                  Serial.print("-");
                  M5.Lcd.print("-");
                }
                printHex2(artKey[i]);
              }
              Serial.println("");
              M5.Lcd.print(" NwkSKey: ");
              Serial.print("NwkSKey: ");
              for (size_t i=0; i<sizeof(nwkKey); ++i) {
                      if (i != 0)
                        {
                              Serial.print("-");
                              M5.Lcd.print("-");
                        }                    
                      printHex2(nwkKey[i]);
              }
              Serial.println();
              M5.Lcd.println(" ");
            }
            // Disable link check validation (automatically enabled
            // during join, but because slow data rates change max TX
	          // size, we don't use it in this example.
            LMIC_setLinkCheckMode(0);
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_RFU1:
        ||     Serial.println(F("EV_RFU1"));
        ||     break;
        */
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            M5.Lcd.println("EV_JOIN_FAILED ");
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            M5.Lcd.println("EV_REJOIN_FAILED ");
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            M5.Lcd.println("EV_TXCOMPLETE (includes waiting for RX windows) ");
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
              M5.Lcd.println("Received ack ");
            if (LMIC.dataLen) {
              Serial.print(F("Received "));
              M5.Lcd.print("Received ");
              Serial.print(LMIC.dataLen);
              M5.Lcd.print(LMIC.dataLen);
              Serial.println(F(" bytes of payload"));
              M5.Lcd.println(" bytes of payload ");
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), sendLora);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            M5.Lcd.println("EV_LOST_TSYNC ");
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            M5.Lcd.println("EV_RESET ");
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE "));
            M5.Lcd.println("EV_RXCOMPLETE ");
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            M5.Lcd.println("EV_LINK_DEAD ");
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            M5.Lcd.println("EV_LINK_ALIVE ");
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_SCAN_FOUND:
        ||    Serial.println(F("EV_SCAN_FOUND"));
        ||    break;
        */
        case EV_TXSTART:
            Serial.println(F("EV_TXSTART"));
            M5.Lcd.println("EV_TXSTART ");
            break;
        case EV_TXCANCELED:
            Serial.println(F("EV_TXCANCELED"));
            M5.Lcd.println("EV_TXCANCELED ");
            break;
        case EV_RXSTART:
            /* do not print anything -- it wrecks timing */
            break;
        case EV_JOIN_TXCOMPLETE:
            Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
            M5.Lcd.println("EV_JOIN_TXCOMPLETE: no JoinAccept ");
            break;

        default:
            Serial.print(F("Unknown event: "));
            M5.Lcd.print("Unknown event: ");
            Serial.println((unsigned) ev);
            M5.Lcd.print((unsigned) ev);
            M5.Lcd.println(" ");
            break;
    }
} 


void sendLora(String etatAlerte, osjob_t* j) {
  /// Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
        M5.Lcd.println("OP_TXRXPEND, not sending ");
    } else {
        // Prepare upstream data transmission at the next possible time.

        sprintf((char*)mydata, {"temp:25, addr:Blagnac, piece:salon, hum:20, humindex:20,bouton:0"});


        LMIC_setTxData2(1, mydata, strlen((char*)mydata), 0);

        //sqn++;
        
        //LMIC_setTxData2(1, mydata, sizeof(mydata)-1, 0);
        Serial.println(F("Packet queued"));
        M5.Lcd.println("Packet queued ");
    }
}

void affichageDesactivationAlarme() {
  M5.Lcd.clear();
  M5.Lcd.setCursor(60, 110);
  M5.Lcd.print("L'Alarme a bien été désactivée");
  delay(2000); // Affichage pendant 2 secondes
}

void affichageEtatInitial(String nomPiece, String temperature, String humidite, String humindex) {
  float batterie = M5.Power.getBatteryLevel();

    // Effacer uniquement la zone où le SSID est affiché
    M5.Lcd.fillRect(0, 0, 320, 50, BLACK); // Ajustez les dimensions au besoin

    // Afficher le SSID et les autres informations
    M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    M5.Lcd.drawString("maison", 240, 10, 4);

    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Lcd.setTextDatum(TL_DATUM);
    M5.Lcd.setTextSize(1.5);
    M5.Lcd.drawString("Batt: " + String(batterie) + "%", 10, 10, 2);

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

/////////////////////////////////////////////////////////////////////////Loca FIN

///////////////////////////////////////////////////////////////////////// Fonction pour extraire la température, humidité et ssid du message reçus
// Ajout de la définition de la fonction extraireInfoDuMessage()
struct ExtractedData extraireInfoDuMessage(String messageToDisplay) {
  ExtractedData data;

  // Find the position of "SSID :"
  int startSsid = messageToDisplay.indexOf("SSID :") + 6;

  // Find the position of "Température : "
  int startTemp = messageToDisplay.indexOf("Température : ") + 13;

  // Find the position of "Humidity : "
  int startHumidity = messageToDisplay.indexOf("Humidity : ") + 11;

  // Find the position of "Humidex : "
  int startHumidex = messageToDisplay.indexOf("Humidex : ") + 10;

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

  return data;
}
///////////////////////////////////////////////////////////////////////// Fonction pour extraire la température, humidité et ssid du message reçus

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
void handleAlert(String alertMessage, IPAddress clientIP) {
  // Trouve la position du premier espace dans le message d'alerte
  int start = alertMessage.indexOf(" ") + 1;

  // Trouve la position du deuxième espace dans le message d'alerte en commençant à partir de la position du premier espace
  int end = alertMessage.indexOf(" ", start);

  // Extrait le nom de l'AP à partir du message d'alerte
  String apName = alertMessage.substring(start, end);

  // Affiche le nom de l'AP en couleur rouge sur l'écran LCD M5
  M5.Lcd.fillScreen(TFT_RED);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_RED);
  M5.Lcd.drawString(apName, 160, 120);

  sendLora("1", &sendjob);

  // Attend 1 seconde pour l'affichage
  delay(1000);
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
}




//code batterie + alarme batterie à ajouter ici

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

