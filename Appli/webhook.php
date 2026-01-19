<?php
require __DIR__ . '/vendor/autoload.php';
require_once 'db.php';

use PhpMqtt\Client\MqttClient;

// Assurer l'encodage UTF-8
header('Content-Type: application/json; charset=utf-8');

$id = $_GET['id'] ?? null;
$param = isset($_GET['param']) ? urldecode($_GET['param']) : '';
$param = mb_convert_encoding($param, 'UTF-8', 'UTF-8');
$isTest = isset($_GET['test']);

if (!$id) {
    echo json_encode(['status' => 'error', 'message' => 'ID manquant'], JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
    exit;
}

try {
    $pdo = Database::getInstance()->getConnection();
    $stmt = $pdo->prepare("SELECT actions FROM choreographies WHERE id = ?");
    $stmt->execute([$id]);
    $choreo = $stmt->fetch();

    if (!$choreo) {
        echo json_encode(['status' => 'error', 'message' => 'Chorégraphie introuvable'], JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
        exit;
    }

    $actions = json_decode($choreo['actions'], true);

    // Remplacement dynamique des paramètres (avec support UTF-8)
    foreach ($actions as &$action) {
        if ($action['type'] === 'display' && isset($action['text'])) {
            // Remplacer {param} avec le paramètre en UTF-8
            $action['text'] = str_replace('{param}', $param, $action['text']);
        }
    }

    // Envoi MQTT sur le broker distant
    $server = 'mqtt.latetedanslatoile.fr';
    $port = 1883;
    $username = 'Epsi';
    $password = 'EpsiWis2018!';
    $topic = 'bisik/henry';
    $clientId = 'bisik-php-webhook-' . uniqid();

    $mqtt = new MqttClient($server, $port, $clientId);

    $settings = (new \PhpMqtt\Client\ConnectionSettings())
        ->setUsername($username)
        ->setPassword($password);

    $mqtt->connect($settings);
    $payload = json_encode($actions, JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
    $mqtt->publish($topic, $payload, 0);
    $mqtt->disconnect();

    echo json_encode(['status' => 'success', 'message' => 'Notification envoyée', 'payload' => json_decode($payload, true)], JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);

} catch (Exception $e) {
    http_response_code(500);
    echo json_encode(['status' => 'error', 'message' => $e->getMessage()], JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
}
?>