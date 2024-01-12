// Récupération du message d'entrée du payload
var inputMsg = msg.payload;

// Parsing de la chaîne JSON contenue dans le payload
inputMsg = JSON.parse(inputMsg);

// Extraction de la chaîne 'mydata' de l'objet 'object' dans le message d'entrée
let mydataString = inputMsg.object.mydata;

// Parsing de la chaîne JSON 'mydataString' pour obtenir un objet JavaScript
var mydataObject = JSON.parse(mydataString);

// Extraction et conversion des propriétés de l'objet en nombres flottants si nécessaire
var temp1 = parseFloat(mydataObject.temp);
var piece1 = mydataObject.piece;
var humindex1 = parseFloat(mydataObject.humindex);
var bouton1 = parseFloat(mydataObject.bouton);

// Création d'un nouvel objet contenant les propriétés converties
var resultat = { humindex: humindex1, piece: piece1, temp: temp1, bouton: bouton1 }

// Mise à jour du payload du message avec le nouvel objet
msg.payload = resultat;

// Retour du message modifié
return msg;
