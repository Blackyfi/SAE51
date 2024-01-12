// Récupération du message d'entrée du payload
let inputMsg = msg.payload;

// Extraction de la valeur de la propriété 'gain' du message d'entrée
let gain1 = inputMsg.gain;

// Création d'un nouvel objet avec la propriété 'gain'
let resultat = { gain: gain1 };

// Mise à jour du payload du message avec le nouvel objet
msg.payload = resultat;

// Retour du message modifié
return msg;
