// Déclaration d'une variable pour le tableau Markdown
var tableauMarkdown = '';

// Déclaration d'une variable pour le titre Markdown
var titre = '---\ntitle: "Analyse spectrale"\n---\n\n\n\n\n' + "#Gestion du confort\n' + 'Dans cette page vous trouverez l'historique des données environnementales de chaque pièce, ainsi qu'un suivi de l'humindex des 12 dernières heures (dans les pièces dans lesquelles vous êtes passé)\n\n\n\n\n";

// Déclaration de balises HTML avec des iframes pour les graphiques de différentes pièces
var cadphp = 'cadphp:p1:bdd7\n';

// Déclaration de la balise HTML pour ajouter des espaces dans le Markdown
var vide = '<br><br> \n';

// Construction de la chaîne Markdown en utilisant les variables précédemment définies
msg.payload = titre + vide + cadphp;

// Retour du message
return msg;
