// Récupération du message d'entrée du payload
var inputMsg = msg.payload;

// Extraction de la pièce du message d'entrée
var salle = inputMsg.piece;

// Création du message d'alerte incluant la pièce
var resultat = "Alerte déclenchée dans " + salle + ": par Madame Michu. Assistance nécessaire au 12 AV des apôtres.";

// Mise à jour du payload du message avec le message d'alerte
msg.payload = resultat;

// Retour du message modifié
return msg;
