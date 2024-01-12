// Récupération du message d'entrée du payload
var inputMsg = msg.payload;

// Parsing de la chaîne JSON contenue dans le payload
inputMsg = JSON.parse(inputMsg);

// Extraction de la chaîne 'mydata' de l'objet 'object' dans le message d'entrée
let mydataString = inputMsg.object.mydata;

// Parsing de la chaîne JSON 'mydataString' pour obtenir un objet JavaScript
var mydataObject = JSON.parse(mydataString);

// Extraction des propriétés de l'objet et conversion en nombres flottants si nécessaire
var temp1 = parseFloat(mydataObject.temp);
var hum1 = parseFloat(mydataObject.hum);
var humindex1 = parseFloat(mydataObject.humindex);

// Extraction et normalisation du nom de la pièce (en minuscules)
var nom1 = mydataObject.piece;
nom1 = nom1.toLowerCase();

// Création d'un nouvel objet contenant les propriétés converties et modifiées
var resultat = { temp: temp1, hum: hum1, humindex: humindex1, piece: nom1 }

// Mise à jour du payload du message avec le nouvel objet
msg.payload = resultat;

// Retour du message modifié
return msg;
