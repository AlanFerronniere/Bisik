<?php
/**
 * API pour lister les fichiers MP3 disponibles
 * Retourne un tableau avec les fichiers et leur mapping de pistes
 */

header('Content-Type: application/json');

$sonsDir = __DIR__ . '/sons';
$sounds = [];

if (is_dir($sonsDir)) {
    $files = scandir($sonsDir);
    $mp3Files = array_filter($files, function($file) use ($sonsDir) {
        return pathinfo($file, PATHINFO_EXTENSION) === 'mp3' && is_file($sonsDir . '/' . $file);
    });
    
    // Tri alphabétique pour maintenir l'ordre cohérent avec le mapping DFPlayer
    sort($mp3Files);
    
    // Génération du mapping piste -> fichier
    $track = 1;
    foreach ($mp3Files as $file) {
        $sounds[] = [
            'track' => $track,
            'file' => $file,
            'trackFormatted' => sprintf('%04d', $track),
            'displayName' => str_replace(['_LaSonotheque.fr.mp3', 'ANMLCat_', 'ROBTVox_'], ['', '🐱 ', '🤖 '], $file)
        ];
        $track++;
    }
}

// Génération du fichier mapping.json pour référence
$mappingFile = $sonsDir . '/mapping.json';
$mapping = [];
foreach ($sounds as $sound) {
    $mapping[$sound['trackFormatted']] = $sound['file'];
}
file_put_contents($mappingFile, json_encode($mapping, JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE));

echo json_encode([
    'success' => true,
    'count' => count($sounds),
    'sounds' => $sounds
]);
