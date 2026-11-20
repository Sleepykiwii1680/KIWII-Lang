const vscode = require('vscode');
const cp = require('child_process');
const path = require('path');
const fs = require('fs');

function activate(context) {
    const diagnosticos = vscode.languages.createDiagnosticCollection('kiwii');
    const temporizadores = new Map();

    function esArchivoKiwii(doc) {
        return doc && doc.uri.scheme === 'file' && (
            doc.languageId.toLowerCase() === 'kiwii' ||
            doc.fileName.toLowerCase().endsWith('.kiw')
        );
    }

    function verificar(doc) {
        if (!esArchivoKiwii(doc)) return;

        const version = doc.version;
        const carpetaWs = vscode.workspace.getWorkspaceFolder(doc.uri);
        let cwd = carpetaWs ? carpetaWs.uri.fsPath : path.dirname(doc.uri.fsPath);
        let relPath = path.relative(cwd, doc.uri.fsPath).replace(/\\/g, '/');

        if (!relPath || relPath.startsWith('..')) {
            cwd = path.dirname(doc.uri.fsPath);
            relPath = path.basename(doc.uri.fsPath);
        }

        const exeLocal = path.join(cwd, 'kiwii.exe');
        const ejecutable = fs.existsSync(exeLocal) ? exeLocal : 'kiwii';

        const proc = cp.spawn(ejecutable, [relPath, '--verificar', '--stdin'], { cwd });
        let salida = '';

        proc.stdout.on('data', (chunk) => {
            salida += chunk.toString('utf8');
        });

        proc.on('error', () => {});

        proc.on('close', () => {
            if (doc.version !== version) return;

            const lista = [];
            const lineas = salida.split(/\r?\n/);

            for (const linea of lineas) {
                if (!linea || linea.startsWith('OK|')) continue;
                const partes = linea.split('|');
                if (partes.length < 4) continue;

                const nivel = partes[0].trim();
                const arch = partes[1].trim().replace(/\\/g, '/');
                const numLin = Math.max(0, (parseInt(partes[2], 10) || 1) - 1);
                const msg = partes.slice(3).join('|').trim();

                const esMismoArchivo =
                    arch === '' ||
                    arch === relPath ||
                    arch === path.basename(relPath);

                if (!esMismoArchivo) {
                    if (nivel === 'ERROR') {
                        const rango = new vscode.Range(0, 0, 0, Math.max(1, doc.lineAt(0).text.length));
                        lista.push(new vscode.Diagnostic(
                            rango,
                            `[${arch}:${numLin + 1}] ${msg}`,
                            vscode.DiagnosticSeverity.Error
                        ));
                    }
                    continue;
                }

                const idxSeguro = Math.min(numLin, Math.max(0, doc.lineCount - 1));
                const textoLin = doc.lineAt(idxSeguro);
                const inicio = textoLin.firstNonWhitespaceCharacterIndex;
                const fin = Math.max(inicio + 1, textoLin.text.length);
                const rango = new vscode.Range(idxSeguro, inicio, idxSeguro, fin);

                const severidad = nivel === 'ERROR'
                    ? vscode.DiagnosticSeverity.Error
                    : vscode.DiagnosticSeverity.Warning;

                const diag = new vscode.Diagnostic(rango, msg, severidad);
                diag.source = 'KIWII';
                lista.push(diag);
            }

            diagnosticos.set(doc.uri, lista);
        });

        proc.stdin.write(doc.getText());
        proc.stdin.end();
    }

    function programarVerificacion(doc) {
        if (!esArchivoKiwii(doc)) return;
        const clave = doc.uri.toString();
        if (temporizadores.has(clave)) clearTimeout(temporizadores.get(clave));
        temporizadores.set(clave, setTimeout(() => {
            temporizadores.delete(clave);
            verificar(doc);
        }, 250));
    }

    context.subscriptions.push(
        diagnosticos,
        vscode.workspace.onDidOpenTextDocument(verificar),
        vscode.workspace.onDidSaveTextDocument(verificar),
        vscode.workspace.onDidChangeTextDocument((e) => programarVerificacion(e.document)),
        vscode.workspace.onDidCloseTextDocument((doc) => diagnosticos.delete(doc.uri))
    );

    vscode.workspace.textDocuments.forEach(verificar);
}

function deactivate() {}

module.exports = { activate, deactivate };