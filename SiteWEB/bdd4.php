<?php
// Inclusion de l'autoloader de Composer pour charger les classes de Guzzle et InfluxDB
require 'vendor/autoload.php';

// Configuration de la connexion à la base de données InfluxDB
$serverUrl = '83.113.15.108';
$database = 'Maison1';

// Options de configuration pour Guzzle
$guzzleOptions = [
    'verify' => false,
    'base_uri' => $serverUrl,
];

// Création d'instances de Guzzle pour l'utilisation avec InfluxDB
$guzzleHttpClient = new GuzzleHttp\Client($guzzleOptions);
$guzzleClient = new InfluxDB\Driver\Guzzle($guzzleHttpClient);

// Création de l'objet client InfluxDB
$client = new InfluxDB\Client($serverUrl, 8086, null, null, $database, $guzzleClient);

// Requête InfluxDB pour récupérer les 20 dernières entrées de température et d'humidité dans la pièce 'sdb'
$query = "SELECT time, temp, hum FROM Mesure WHERE piece = 'sdb' ORDER BY time DESC LIMIT 20;";
$result = $client->query($database, $query);

// Déclaration de la chaîne HTML pour le rendu final
$htmlOutput = '<style>
    table {
        border-collapse: collapse;
        width: 100%;
        margin-bottom: 20px;
    }
    th, td {
        border: 1px solid #dddddd;
        text-align: left;
        padding: 8px;
    }
    th {
        background-color: #f2f2f2;
    }
    caption {
        caption-side: top;
        font-size: 1.5em;
        font-weight: bold;
        margin-bottom: 10px;
        background-color: #f2f2f2; 
        padding: 8px; 
        border: 1px solid #dddddd;
    }
</style>';

$htmlOutput .= '<table>';        
$htmlOutput .= '<caption>Salle de Bain</caption>';
$htmlOutput .= '<tr><th>Time</th><th>Temperature</th><th>Humidité</th></tr>';

// Parcours des points de résultat de la requête InfluxDB
foreach ($result->getPoints() as $point) {
    // Formattage de la date et de l'heure
    $dateTimeParts = explode('T', $point['time']);
    
    if (strpos($dateTimeParts[1], '.') !== false) {
        $timeParts = explode('.', $dateTimeParts[1]);
        $formattedTime = $dateTimeParts[0] . ' ' . $timeParts[0];
    } else {
        $formattedTime = $point['time'];
    }

    // Ajout des données dans la table HTML
    $htmlOutput .= '<tr>';
    $htmlOutput .= '<td>' . $formattedTime . '</td>';
    $htmlOutput .= '<td>' . $point['temp'] . '</td>';
    $htmlOutput .= '<td>' . $point['hum'] . '</td>';
    $htmlOutput .= '</tr>';
}

$htmlOutput .= '</table>';

// Retourner la chaîne HTML plutôt que l'afficher avec echo
return $htmlOutput;
?>
