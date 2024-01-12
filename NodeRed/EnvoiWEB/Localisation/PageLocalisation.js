// Récupération du message d'entrée du payload
var inputMsg = msg.payload;

// Parsing de la chaîne JSON contenue dans le payload
inputMsg = JSON.parse(inputMsg);

// Extraction de la chaîne 'mydata' de l'objet 'object' dans le message d'entrée
let mydataString = inputMsg.object.mydata;

// Parsing de la chaîne JSON 'mydataString' pour obtenir un objet JavaScript
var mydataObject = JSON.parse(mydataString);

// Extraction du nom de la pièce
var piece = mydataObject.piece;

// Création d'un nouveau contenu au format Markdown en utilisant des variables
var newContent = `
---
title: 'Données localisation'
---

# Données Actuelles

Dans cette rubrique, vous trouverez les informations en temps réel de la pièce dans laquelle vous êtes, ainsi qu'un baromètre du confort de la pièce. 

![ ` + piece + `](cartographie_MIB_` + piece +`.png)

<iframe src="http://83.113.15.108:3000/d-solo/ef12063d-2c73-4f6f-8a9f-24fc6e84baee/sdb?orgId=1&from=now-12h&to=now&theme=light&panelId=7" width="800" height="550" frameborder="0"></iframe>
`;

// Mise à jour du payload du message avec le nouveau contenu Markdown
msg.payload = newContent;

// Retour du message modifié
return msg;
