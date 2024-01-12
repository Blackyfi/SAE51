// Récupération du message d'entrée du payload
var inputMsg = msg.payload;

// Extraction de la pièce et de la température du message d'entrée
var salle = inputMsg.piece;
var temp = inputMsg.temp; 

// Création du message d'alerte en fonction de la température et de la pièce
var resultat = "Alerte température, le seuil a été dépassé. La température est actuellement de " + temp + " °C dans " + salle + ". Un besoin d'assistance est peut-être requis";

// Mise à jour du payload du message avec le message d'alerte
msg.payload = resultat; 

// Retour du message modifié
return msg; 

