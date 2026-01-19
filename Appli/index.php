<?php
require_once 'db.php';

// Gestion de la suppression
if (isset($_GET['delete'])) {
    $id = (int)$_GET['delete'];
    $pdo = Database::getInstance()->getConnection();
    $stmt = $pdo->prepare("DELETE FROM choreographies WHERE id = ?");
    $stmt->execute([$id]);
    header('Location: index.php');
    exit;
}

$pdo = Database::getInstance()->getConnection();
$stmt = $pdo->query("SELECT * FROM choreographies ORDER BY created_at DESC");
$choreographies = $stmt->fetchAll();
?>

<!doctype html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Bisik - Gestion des chorégraphies</title>
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css" rel="stylesheet">
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap-icons@1.10.0/font/bootstrap-icons.css">
    <style>
        .webhook-url {
            font-size: 0.75rem;
            max-width: 300px;
            overflow: hidden;
            text-overflow: ellipsis;
            white-space: nowrap;
            cursor: pointer;
        }
        .webhook-url:hover {
            overflow: visible;
            white-space: normal;
            word-break: break-all;
        }
    </style>
</head>
<body class="bg-light">
    <div class="container-fluid py-4">
        <div class="d-flex justify-content-between align-items-center mb-4">
            <h1 class="h3 text-primary"><i class="bi bi-robot"></i> Bisik Manager</h1>
            <a href="editor.php" class="btn btn-success"><i class="bi bi-plus-lg"></i> Nouvelle Chorégraphie</a>
        </div>

        <?php if (empty($choreographies)): ?>
            <div class="text-center py-5">
                <p class="text-muted">Aucune chorégraphie créée pour le moment.</p>
                <a href="editor.php" class="btn btn-primary">Créer ma première notification</a>
            </div>
        <?php else: ?>
            <div class="card shadow-sm">
                <div class="card-body p-0">
                    <div class="table-responsive">
                        <table class="table table-hover align-middle mb-0">
                            <thead class="table-light">
                                <tr>
                                    <th style="width: 5%;">ID</th>
                                    <th style="width: 20%;">Nom</th>
                                    <th style="width: 25%;">Description</th>
                                    <th style="width: 10%;">Actions</th>
                                    <th style="width: 30%;">Webhook URL</th>
                                    <th style="width: 10%;">Actions</th>
                                </tr>
                            </thead>
                            <tbody>
                                <?php foreach ($choreographies as $choreo): 
                                    $actions = json_decode($choreo['actions'], true);
                                    $actionCount = is_array($actions) ? count($actions) : 0;
                                ?>
                                    <tr>
                                        <td class="text-muted"><?php echo $choreo['id']; ?></td>
                                        <td><strong><?php echo htmlspecialchars($choreo['name']); ?></strong></td>
                                        <td class="text-muted small"><?php echo htmlspecialchars($choreo['description']); ?></td>
                                        <td>
                                            <span class="badge bg-info"><?php echo $actionCount; ?> action<?php echo $actionCount > 1 ? 's' : ''; ?></span>
                                        </td>
                                        <td>
                                            <?php 
                                                $base = 'https://bisik.bellocq.local/webhook.php';
                                                $query = http_build_query(['id' => $choreo['id'], 'param' => 'TEXTE']);
                                                $webhookUrl = $base . '?' . $query;
                                            ?>
                                            <code class="webhook-url user-select-all bg-light p-1 rounded d-inline-block" 
                                                  title="Cliquer pour copier"
                                                  onclick="copyToClipboard(this.textContent)"><?php echo htmlspecialchars($webhookUrl, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8'); ?></code>
                                        </td>
                                        <td>
                                            <div class="btn-group btn-group-sm" role="group">
                                                <a href="editor.php?id=<?php echo $choreo['id']; ?>" 
                                                   class="btn btn-outline-secondary" 
                                                   title="Éditer">
                                                    <i class="bi bi-pencil"></i>
                                                </a>
                                                <button onclick="testChoreo(<?php echo $choreo['id']; ?>)" 
                                                        class="btn btn-outline-primary" 
                                                        title="Tester">
                                                    <i class="bi bi-play-fill"></i>
                                                </button>
                                                <button onclick="deleteChoreo(<?php echo $choreo['id']; ?>, '<?php echo addslashes($choreo['name']); ?>')" 
                                                        class="btn btn-outline-danger" 
                                                        title="Supprimer">
                                                    <i class="bi bi-trash"></i>
                                                </button>
                                            </div>
                                        </td>
                                    </tr>
                                <?php endforeach; ?>
                            </tbody>
                        </table>
                    </div>
                </div>
                <div class="card-footer text-muted small">
                    Total : <?php echo count($choreographies); ?> chorégraphie<?php echo count($choreographies) > 1 ? 's' : ''; ?>
                </div>
            </div>
        <?php endif; ?>
    </div>

    <script>
        function testChoreo(id) {
            const p = prompt('Texte pour {param}', 'Test');
            const url = 'webhook.php?id=' + id + (p !== null ? '&param=' + encodeURIComponent(p) : '') + '&test=true';
            fetch(url)
                .then(response => response.json())
                .then(data => {
                    alert((data.status || 'ok') + (data.payload ? '\nPayload: ' + JSON.stringify(data.payload) : ''));
                })
                .catch(error => alert('Erreur lors du test'));
        }
        
        function deleteChoreo(id, name) {
            if (confirm('Êtes-vous sûr de vouloir supprimer la chorégraphie "' + name + '" ?\n\nCette action est irréversible.')) {
                window.location.href = 'index.php?delete=' + id;
            }
        }
        
        function copyToClipboard(text) {
            navigator.clipboard.writeText(text).then(() => {
                // Afficher une notification temporaire
                const toast = document.createElement('div');
                toast.className = 'position-fixed top-0 end-0 p-3';
                toast.style.zIndex = '9999';
                toast.innerHTML = '<div class="alert alert-success alert-dismissible fade show" role="alert">' +
                    '<i class="bi bi-check-circle"></i> URL copiée dans le presse-papier !' +
                    '<button type="button" class="btn-close" data-bs-dismiss="alert"></button></div>';
                document.body.appendChild(toast);
                setTimeout(() => toast.remove(), 3000);
            }).catch(err => {
                alert('Erreur lors de la copie : ' + err);
            });
        }
    </script>
</body>
</html>