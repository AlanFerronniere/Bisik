<?php
require_once 'db.php';

$id = $_GET['id'] ?? null;
$choreo = null;

if ($id) {
    $pdo = Database::getInstance()->getConnection();
    $stmt = $pdo->prepare("SELECT * FROM choreographies WHERE id = ?");
    $stmt->execute([$id]);
    $choreo = $stmt->fetch();
}

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $pdo = Database::getInstance()->getConnection();
    $name = $_POST['name'];
    $description = $_POST['description'];
    $actions = $_POST['actions']; // JSON string from frontend

    if ($id) {
        $stmt = $pdo->prepare("UPDATE choreographies SET name = ?, description = ?, actions = ? WHERE id = ?");
        $stmt->execute([$name, $description, $actions, $id]);
    } else {
        $stmt = $pdo->prepare("INSERT INTO choreographies (name, description, actions) VALUES (?, ?, ?)");
        $stmt->execute([$name, $description, $actions]);
    }
    header('Location: index.php');
    exit;
}
?>

<!doctype html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Éditeur de Chorégraphie - Bisik</title>
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css" rel="stylesheet">
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap-icons@1.10.0/font/bootstrap-icons.css">
    <style>
        .action-card { cursor: move; border-left: 4px solid #0d6efd; }
        .timeline-container { min-height: 200px; background: #f8f9fa; border: 2px dashed #dee2e6; border-radius: 8px; padding: 1rem; }
    </style>
</head>
<body class="bg-light">
    <div class="container py-4">
        <div class="row mb-4">
            <div class="col">
                <h2><?php echo $id ? 'Modifier' : 'Nouvelle'; ?> Chorégraphie</h2>
            </div>
            <div class="col-auto">
                <a href="index.php" class="btn btn-outline-secondary">Annuler</a>
            </div>
        </div>

        <form method="post" id="choreoForm">
            <div class="row">
                <div class="col-md-4">
                    <div class="card mb-3">
                        <div class="card-header">Informations</div>
                        <div class="card-body">
                            <div class="mb-3">
                                <label class="form-label">Nom</label>
                                <input type="text" name="name" class="form-control" value="<?php echo htmlspecialchars($choreo['name'] ?? ''); ?>" required>
                            </div>
                            <div class="mb-3">
                                <label class="form-label">Description</label>
                                <textarea name="description" class="form-control" rows="3"><?php echo htmlspecialchars($choreo['description'] ?? ''); ?></textarea>
                            </div>
                        </div>
                    </div>

                    <div class="card">
                        <div class="card-header">Boîte à outils</div>
                        <div class="card-body d-grid gap-2">
                            <button type="button" class="btn btn-outline-primary text-start" onclick="addAction('sound')">
                                <i class="bi bi-volume-up-fill me-2"></i> Ajouter Son MP3
                            </button>
                            <button type="button" class="btn btn-outline-success text-start" onclick="addAction('servo')">
                                <i class="bi bi-arrows-move me-2"></i> Ajouter Mouvement
                            </button>
                            <button type="button" class="btn btn-outline-warning text-start" onclick="addAction('display')">
                                <i class="bi bi-display me-2"></i> Ajouter Message
                            </button>
                            <button type="button" class="btn btn-outline-secondary text-start" onclick="addAction('wait')">
                                <i class="bi bi-hourglass me-2"></i> Ajouter Pause
                            </button>
                        </div>
                    </div>
                </div>

                <div class="col-md-8">
                    <div class="card">
                        <div class="card-header bg-primary text-white">Timeline des actions</div>
                        <div class="card-body">
                            <div id="timeline" class="timeline-container">
                                <!-- Les actions seront injectées ici par JS -->
                            </div>
                        </div>
                    </div>
                </div>
            </div>
            
            <input type="hidden" name="actions" id="actionsInput">
            <div class="fixed-bottom bg-white border-top p-3 text-end shadow">
                <button type="submit" class="btn btn-primary btn-lg">Enregistrer</button>
            </div>
        </form>
    </div>

    <!-- Templates pour JS -->
    <template id="tpl-sound">
        <div class="card mb-2 action-card" data-type="sound">
            <div class="card-body p-2">
                <div class="d-flex justify-content-between">
                    <strong><i class="bi bi-volume-up-fill"></i> Son MP3</strong>
                    <button type="button" class="btn-close btn-sm" onclick="this.closest('.action-card').remove()"></button>
                </div>
                <select class="form-select form-select-sm mt-2" data-field="file" onchange="updateTrackNumber(this)" required>
                    <option value="">Sélectionner un son...</option>
                </select>
                <input type="hidden" data-field="track">
                <div class="input-group input-group-sm mt-2">
                    <span class="input-group-text">Volume</span>
                    <input type="range" class="form-range" data-field="volume" min="0" max="30" value="20" oninput="this.nextElementSibling.textContent = this.value" style="flex: 1;">
                    <span class="input-group-text" style="min-width: 40px;">20</span>
                </div>
                <div class="mt-2">
                    <button type="button" class="btn btn-sm btn-outline-info" onclick="previewSound(this)">
                        <i class="bi bi-play-circle"></i> Écouter
                    </button>
                    <span class="ms-2 text-muted small">Piste: <span class="track-display">-</span></span>
                </div>
            </div>
        </div>
    </template>

    <template id="tpl-servo">
        <div class="card mb-2 action-card border-success" data-type="servo">
            <div class="card-body p-2">
                <div class="d-flex justify-content-between text-success">
                    <strong><i class="bi bi-arrows-move"></i> Mouvement Bras</strong>
                    <button type="button" class="btn-close btn-sm" onclick="this.closest('.action-card').remove()"></button>
                </div>
                <div class="input-group input-group-sm mt-2">
                    <span class="input-group-text">Angle (0-180)</span>
                    <input type="number" class="form-control" data-field="angle" min="0" max="180">
                </div>
                <div class="input-group input-group-sm mt-1">
                    <span class="input-group-text">Vitesse (ms)</span>
                    <input type="number" class="form-control" data-field="speed" value="500">
                </div>
            </div>
        </div>
    </template>

    <template id="tpl-display">
        <div class="card mb-2 action-card border-warning" data-type="display">
            <div class="card-body p-2">
                <div class="d-flex justify-content-between text-warning">
                    <strong><i class="bi bi-display"></i> Affichage Écran</strong>
                    <button type="button" class="btn-close btn-sm" onclick="this.closest('.action-card').remove()"></button>
                </div>
                <input type="text" class="form-control form-control-sm mt-2" placeholder="Message (utiliser {param} pour variable)" data-field="text">
                <div class="input-group input-group-sm mt-1">
                    <span class="input-group-text">Taille texte</span>
                    <input type="number" class="form-control" data-field="textSize" min="1" max="8" value="2">
                </div>
                <div class="input-group input-group-sm mt-1">
                    <span class="input-group-text">Durée (ms)</span>
                    <input type="number" class="form-control" data-field="duration" value="3000">
                </div>
            </div>
        </div>
    </template>
    
     <template id="tpl-wait">
        <div class="card mb-2 action-card border-secondary" data-type="wait">
            <div class="card-body p-2">
                <div class="d-flex justify-content-between text-secondary">
                    <strong><i class="bi bi-hourglass"></i> Pause</strong>
                    <button type="button" class="btn-close btn-sm" onclick="this.closest('.action-card').remove()"></button>
                </div>
                <div class="input-group input-group-sm mt-2">
                    <span class="input-group-text">Durée (ms)</span>
                    <input type="number" class="form-control" data-field="duration" value="1000">
                </div>
            </div>
        </div>
    </template>

    <script src="https://cdn.jsdelivr.net/npm/sortablejs@latest/Sortable.min.js"></script>
    <script>
        const timeline = document.getElementById('timeline');
        new Sortable(timeline, { animation: 150, ghostClass: 'bg-light' });

        let availableSounds = [];
        let audioPlayer = new Audio();
        let soundsLoaded = false;

        // Charger la liste des sons disponibles
        fetch('api_sounds.php')
            .then(res => res.json())
            .then(data => {
                if(data.success) {
                    availableSounds = data.sounds;
                    soundsLoaded = true;
                    console.log(`${data.count} fichiers MP3 chargés`);
                    // Charger les actions existantes après le chargement des sons
                    loadSavedActions();
                }
            })
            .catch(err => {
                console.error('Erreur chargement sons:', err);
                soundsLoaded = true; // Continuer même en cas d'erreur
                loadSavedActions();
            });

        function addAction(type) {
            const tpl = document.getElementById('tpl-' + type);
            const clone = tpl.content.cloneNode(true);
            timeline.appendChild(clone);
            
            // Si c'est un son, peupler le dropdown
            if(type === 'sound') {
                const select = timeline.lastElementChild.querySelector('select[data-field="file"]');
                availableSounds.forEach(sound => {
                    const option = document.createElement('option');
                    option.value = sound.file;
                    option.dataset.track = sound.track;
                    option.textContent = sound.displayName;
                    select.appendChild(option);
                });
            }
        }

        function updateTrackNumber(select) {
            const card = select.closest('.action-card');
            const selectedOption = select.options[select.selectedIndex];
            const track = selectedOption.dataset.track || '';
            
            card.querySelector('input[data-field="track"]').value = track;
            card.querySelector('.track-display').textContent = track ? `#${track}` : '-';
        }

        function previewSound(button) {
            const card = button.closest('.action-card');
            const file = card.querySelector('select[data-field="file"]').value;
            
            if(!file) {
                alert('Veuillez sélectionner un son d\'abord');
                return;
            }
            
            audioPlayer.src = 'sons/' + encodeURIComponent(file);
            audioPlayer.play()
                .catch(err => alert('Erreur lecture audio: ' + err.message));
            
            button.innerHTML = '<i class="bi bi-stop-circle"></i> Stop';
            button.onclick = function() {
                audioPlayer.pause();
                audioPlayer.currentTime = 0;
                button.innerHTML = '<i class="bi bi-play-circle"></i> Écouter';
                button.onclick = function() { previewSound(this); };
            };
        }

        // Chargement des données existantes
        function loadSavedActions() {
            const savedActions = <?php echo $choreo['actions'] ?? '[]'; ?>;
            savedActions.forEach(action => {
                addAction(action.type);
                const lastCard = timeline.lastElementChild;
                for (const [key, value] of Object.entries(action)) {
                    if(key !== 'type') {
                        const input = lastCard.querySelector(`[data-field="${key}"]`);
                        if(input) {
                            input.value = value;
                            // Pour les sons, mettre à jour l'affichage du numéro de piste
                            if(action.type === 'sound' && key === 'file') {
                                setTimeout(() => updateTrackNumber(input), 100);
                            }
                            // Mettre à jour l'affichage du volume
                            if(action.type === 'sound' && key === 'volume' && input.nextElementSibling) {
                                input.nextElementSibling.textContent = value;
                            }
                        }
                    }
                }
            });
        }

        document.getElementById('choreoForm').addEventListener('submit', function(e) {
            const actions = [];
            document.querySelectorAll('.action-card').forEach(card => {
                const action = { type: card.dataset.type };
                card.querySelectorAll('[data-field]').forEach(input => {
                    const value = input.value;
                    // Convertir en nombre pour track, angle, speed, duration, volume, textSize
                    if(['track', 'angle', 'speed', 'duration', 'volume', 'textSize'].includes(input.dataset.field)) {
                        action[input.dataset.field] = parseInt(value) || 0;
                    } else {
                        action[input.dataset.field] = value;
                    }
                });
                actions.push(action);
            });
            document.getElementById('actionsInput').value = JSON.stringify(actions);
        });
    </script>
</body>
</html>
