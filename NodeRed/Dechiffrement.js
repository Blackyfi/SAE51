// Déclaration d'une variable contenant la clé de cryptage
let cryptingKey = 24;

// Définition de la fonction de décryptage
function messageDecryption(messageToDecrypt, cryptingKey) {
    // Initialisation d'une variable pour stocker le message décrypté
    let decryptedMessage = "";

    // Boucle à travers chaque caractère du message à décrypter
    for (let i = 0; i < messageToDecrypt.length; i++) {
        // Récupération du code ASCII du caractère courant
        let currentChar = messageToDecrypt.charCodeAt(i);

        // Décryptage en soustrayant la clé de cryptage au code ASCII du caractère
        decryptedMessage += String.fromCharCode(currentChar - cryptingKey);
    }

    // Retour du message décrypté
    return decryptedMessage;
}
