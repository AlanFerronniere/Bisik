<?php
class Database {
    private static $instance = null;
    private $pdo;

    private $host = 'localhost'; // Sur le serveur de prod, c'est localhost
    private $db   = 'bisik';
    private $user = 'bisik';
    private $pass = 'Bsk32Bsk32#';
    private $charset = 'utf8mb4';

    private function __construct() {
        // Détection automatique de l'environnement pour le développement local vs prod
        if ($_SERVER['SERVER_NAME'] !== 'bisik.bellocq.local') {
             $this->host = 'bellocq.local';
        }

        $dsn = "mysql:host=$this->host;dbname=$this->db;charset=$this->charset";
        $options = [
            PDO::ATTR_ERRMODE            => PDO::ERRMODE_EXCEPTION,
            PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
            PDO::ATTR_EMULATE_PREPARES   => false,
        ];

        try {
            $this->pdo = new PDO($dsn, $this->user, $this->pass, $options);
        } catch (\PDOException $e) {
            throw new \PDOException($e->getMessage(), (int)$e->getCode());
        }
    }

    public static function getInstance() {
        if (self::$instance == null) {
            self::$instance = new Database();
        }
        return self::$instance;
    }

    public function getConnection() {
        return $this->pdo;
    }
}
?>