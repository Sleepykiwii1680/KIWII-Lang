#include "src/tipos.hpp"
#include "src/lexer.hpp"
#include "src/ast.hpp"
#include "src/parser.hpp"
#include "src/ejecutor.hpp"
#include "src/analizador.hpp"

//FALTA AGREGAR PARA LECTURA NETWORKING (...no lo esperes xd)

// PUNTO DE ENTRADA 

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "KIWII Kernel v4.0 | Uso: kiwii <archivo.kiw> [--debug] [--aislado] [--verificar]" << endl;
        return 1;
    }
    string bin = argv[0];
    size_t barra = bin.find_last_of("/\\");
    if (barra != string::npos) rutaKiwii = bin.substr(0, barra + 1);
    string rutaArchivo = "";
    bool modoDebug = false;
    bool modoVerificar = false;
    bool modoStdin = false;
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--debug") modoDebug = true;
        else if (arg == "--aislado") modoAislado = true;
        else if (arg == "--verificar") modoVerificar = true;
        else if (arg == "--stdin") modoStdin = true;
        else if (rutaArchivo.empty()) rutaArchivo = arg;
    }
    string codigoBruto = "";
    if (modoStdin) {
        stringstream buffer;
        buffer << cin.rdbuf();
        codigoBruto = buffer.str();
        if (rutaArchivo.empty()) rutaArchivo = "editor.kiw";
    } else {
        if (rutaArchivo.empty()) {
            cout << "KIWII Kernel v4.0 | Uso: kiwii <archivo.kiw> [--debug] [--aislado] [--verificar]" << endl;
            return 1;
        }
        codigoBruto = leerArchivo(rutaArchivo);
        if (codigoBruto.empty()) return 1;
    }
    for (char& c : rutaArchivo) if (c == '\\') c = '/';
    set<string> archivosIncluidos = {rutaArchivo};
    vector<pair<string, int>> mapaLineas;
    string codigoFuente = "";
    if (modoVerificar && rutaArchivo.rfind("lib/", 0) == 0) {
        archivosIncluidos.insert("estandar.kiw");
        string est = leerArchivo("estandar.kiw");
        if (!est.empty()) {
            codigoFuente += preprocesar(est, "estandar.kiw", archivosIncluidos, mapaLineas);
        }
    }
    codigoFuente += preprocesar(codigoBruto, rutaArchivo, archivosIncluidos, mapaLineas);
    NodoPrograma* arbol = nullptr;
    try {
        vector<Token> tokens = tokenizar(codigoFuente, mapaLineas);
        Parser parser(tokens, mapaLineas);
        arbol = parser.parsear();

        if (modoVerificar) {
            Analizador analizador(arbol, mapaLineas);
            vector<Diagnostico> diags = analizador.analizar();
            bool hayErrores = false;
            for (const auto& d : diags) {
                cout << d.nivel << "|" << d.archivo << "|" << d.linea << "|" << d.mensaje << endl;
                if (d.nivel == "ERROR") hayErrores = true;
            }
            if (diags.empty()) cout << "OK|Sin errores ni advertencias" << endl;
            delete arbol;
            return hayErrores ? 1 : 0;
        }

        if (modoDebug) {
            cout << " Archivos fusionados: " << archivosIncluidos.size() << endl;
            cout << " Tokens generados: " << tokens.size() << endl;
            cout << " Funciones en memoria: " << arbol->funciones.size() << endl;
            cout << " Estructuras en memoria: " << arbol->estructuras.size() << endl;
            cout << " Modo aislado (Sandbox): " << (modoAislado ? "ACTIVO" : "INACTIVO") << endl;
            cout << "================ SALIDA ================" << endl;
        }
        ejecutar(arbol);
        if (modoDebug) {
            cout << "=================================================" << endl;
        }
    } catch (const exception& e) {
        if (modoVerificar) {
            string err = e.what();
            size_t pEn = err.find(" en ");
            size_t pLin = err.find(" Linea ");
            size_t pFin = err.find("]: ");
            if (pEn != string::npos && pLin != string::npos && pFin != string::npos && pEn < pLin && pLin < pFin) {
                string arch = err.substr(pEn + 4, pLin - (pEn + 4));
                string lin = err.substr(pLin + 7, pFin - (pLin + 7));
                string msg = err.substr(pFin + 3);
                cout << "ERROR|" << arch << "|" << lin << "|" << msg << endl;
            } else {
                cout << "ERROR|" << rutaArchivo << "|1|" << err << endl;
            }
        } else {
            cerr << "[ERROR GRAVE XD]: " << e.what() << endl;
        }
        delete arbol;
        return 1;
    }
    delete arbol;
    return 0;
}