
// Déclaration d'une variable pour le tableau Markdown
var tableauMarkdown = '';

// Déclaration d'une variable pour le titre Markdown
var titre = '---\ntitle: "Gestion du confort"\n---\n\n\n\n\n' + "# Gestion du confort\n' + 'Dans cette page vous trouverez l'historique des données environnementales de chaque pièce, ainsi qu'un suivi de l'humindex des 12 dernières heures (dans les pièces dans lesquelles vous êtes passé)\n\n\n\n\n";

// Déclaration de variables contenant des chaînes de configuration pour des graphiques
var cadphp = 'cadphp:p1:bdd\n';
var cadphp2 = 'cadphp:p2:bdd2\n';
var cadphp3 = 'cadphp:p3:bdd3\n';
var cadphp4 = 'cadphp:p4:bdd4\n';
var cadphp5 = 'cadphp:p5:bdd5\n';
var cadphp6 = 'cadphp:p6:bdd6\n';

// Déclaration de la balise HTML pour ajouter des espaces dans le Markdown
var vide = '<br><br> \n';

// Déclaration de balises HTML avec des iframes pour les graphiques de différentes pièces
var graphsallon = '<iframe src="http://83.113.15.108:3000/d-solo/ef12063d-2c73-4f6f-8a9f-24fc6e84baee/sdb?orgId=1&from=now-12h&to=now&theme=light&panelId=1" width="800" height="550" frameborder="0"></iframe>\n';
var graphchambre = '<iframe src="http://83.113.15.108:3000/d-solo/ef12063d-2c73-4f6f-8a9f-24fc6e84baee/sdb?orgId=1&from=now-12h&to=now&theme=light&panelId=2" width="800" height="550" frameborder="0"></iframe>'
var graphwc = '<iframe src="http://83.113.15.108:3000/d-solo/ef12063d-2c73-4f6f-8a9f-24fc6e84baee/sdb?orgId=1&from=now-12h&to=now&theme=light&panelId=5" width="800" height="550" frameborder="0"></iframe>'
var graphsdb = '<iframe src="http://83.113.15.108:3000/d-solo/ef12063d-2c73-4f6f-8a9f-24fc6e84baee/sdb?orgId=1&from=now-12h&to=now&theme=light&panelId=3" width="800" height="550" frameborder="0"></iframe>'
var graphcuisine = '<iframe src="http://83.113.15.108:3000/d-solo/ef12063d-2c73-4f6f-8a9f-24fc6e84baee/sdb?orgId=1&from=now-12h&to=now&theme=light&panelId=4" width="800" height="550" frameborder="0"></iframe>'
var graphsdr = '<iframe src="http://83.113.15.108:3000/d-solo/ef12063d-2c73-4f6f-8a9f-24fc6e84baee/sdb?orgId=1&from=now-12h&to=now&theme=light&panelId=6" width="800" height="550" frameborder="0"></iframe>'

// Construction de la chaîne Markdown en utilisant les variables précédemment définies
msg.payload = titre + vide + cadphp + graphsallon + vide + vide + vide + cadphp2 + graphchambre + vide + vide + vide + cadphp3 + graphwc + vide + vide + vide + cadphp4 + graphsdb + vide + vide + vide + cadphp5 + graphcuisine + vide + vide + vide + cadphp6 + graphsdr;

// Retour du message
return msg;

// tests
/*
var inputMsg = msg.payload;
var inputMsg = JSON.parse(inputMsg);
let mydataString = inputMsg.object.mydata;

var mydataObject = JSON.parse(mydataString);


var temp1 = parseFloat(mydataObject.temp);
var hum1 = parseFloat(mydataObject.hum);
var humindex1 = parseFloat(mydataObject.humindex);
var nom1 = parseFloat(mydataObject.piece);


function formatDate(date) {
    var mois = date.toLocaleString('default', { month: 'short' });
    var jour = date.getDate();
    var annee = date.getFullYear();
    var heures = date.getHours();
    var minutes = date.getMinutes();
    var secondes = date.getSeconds();

    return `${mois} ${jour} ${annee} ${heures}:${minutes}:${secondes}`;
}
// Charger le tableau existant à partir du contexte
context.dataArray = context.dataArray || [];

// Ajouter une nouvelle entrée au tableau de données
var nouvelleEntree = {
    time: formatDate(new Date()),
    temperature: temp1,
    humidity: hum1,
    humindex: humindex1,
    piece: nom1
};

// Ajouter la nouvelle entrée au début du tableau
context.dataArray.unshift(nouvelleEntree);

// Limiter le tableau à 10 entrées
context.dataArray = context.dataArray.slice(0, 40);

// Créer le tableau Markdown à partir du tableau de données
tableauMarkdown = '| Time | Temperature | Humidity | Humindex | Piece |\n';
tableauMarkdown += '|------|-------------|----------|----------|-------|\n';

for (var i = 0; i < context.dataArray.length; i++) {
    var entry = context.dataArray[i];
    tableauMarkdown += `| ${entry.time} | ${entry.temperature} | ${entry.humidity} | ${entry.humindex} | ${entry.piece} |\n`;
}

// Mise à jour du contexte avec le tableau mis à jour
context.dataArray = context.dataArray;
*/
