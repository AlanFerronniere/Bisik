<?php
require __DIR__ . '/vendor/autoload.php';
require_once 'db.php';

use PhpMqtt\Client\MqttClient;

header('Content-Type: application/json');

$id = $_GET['id'] ?? null;
$param = $_GET['param'] ?? '';
$isTest = isset($_GET['test']);

if (!$id) {
    echo json_encode(['status' => 'error', 'message' => 'ID manquant']);
    exit;
}

try {
    $pdo = Database::getInstance()->getConnection();
    $stmt = $pdo->prepare("SELECT actions FROM choreographies WHERE id = ?");
    $stmt->execute([$id]);
    $choreo = $stmt->fetch();

    if (!$choreo) {
        echo json_encode(['status' => 'error', 'message' => 'Chorégraphie introuvable']);
        exit;
    }

    $actions = json_decode($choreo['actions'], true);

    // Remplacement dynamique des paramètres
    foreach ($actions as &$action) {
        if ($action['type'] === 'display' && isset($action['text'])) {
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
    $payload = json_encode($actions);
    $mqtt->publish($topic, $payload, 0);
    $mqtt->disconnect();

    echo json_encode(['status' => 'success', 'message' => 'Notification envoyée', 'payload' => json_decode($payload, true)]);

} catch (Exception $e) {
    http_response_code(500);
    echo json_encode(['status' => 'error', 'message' => $e->getMessage()]);
}
?>