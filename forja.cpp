#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <cstdlib>
using namespace std;
// DICCIONARIO Y ESTRUCTURA DE TOKENS

enum class TipoToken {
    PALABRA_CLAVE, 
    IDENTIFICADOR, 
    NUMERO,        
    CADENA,        
    SIMBOLO,       
    DIRECTIVA,     
    DESCONOCIDO    
};

struct Token {
    TipoToken tipo;
    string valor; 
};

bool modoAislado = false;
int profundidadPila = 0;
const int LIMITE_CICLOS = 100000;
const int LIMITE_PILA = 256;
string rutaKiwii = "";

// UTILIDAD DE LECTURA Y PREPROCESADOR (#incluir) 

bool esRutaSegura(const string& ruta) {
    if (ruta.empty()) return false;
    if (ruta.find("..") != string::npos) return false;
    if (ruta.find(':') != string::npos) return false;
    if (ruta[0] == '/' || ruta[0] == '\\') return false;
    return true;
}

string leerArchivo(const string& ruta) {
    if (!esRutaSegura(ruta)) {
        cerr << "[Seguridad Kernel]: Acceso bloqueado por Path Traversal en '" << ruta << "'" << endl;
        return "";
    }
    ifstream archivo(ruta);
    if (!archivo.is_open() && !rutaKiwii.empty()) {
        archivo.open(rutaKiwii + ruta);
    }
    if (!archivo.is_open() && getenv("PATH")) {
        stringstream sp(getenv("PATH"));
        string dir;
        while (getline(sp, dir, ';')) {
            if (!dir.empty() && ifstream(dir + "/kiwii.exe").good()) {
                archivo.open(dir + "/" + ruta);
                if (archivo.is_open()) break;
            }
        }
    }
    if (!archivo.is_open()) {
        cerr << "[Error Kernel]: No se encontro el archivo '" << ruta << "'" << endl;
        return "";
    }
    stringstream buffer;
    buffer << archivo.rdbuf();
    return buffer.str();
}

// FuZion de archivos #incluir "otro_archivo.kiw" antes de pasar al Lexer... no preguntes xd
string preprocesar(const string& codigo, set<string>& incluidos) {
    stringstream ss(codigo);
    string linea, resultado = "";
    
    while (getline(ss, linea)) {
        size_t pos = linea.find("#incluir");
        if (pos != string::npos) {
            size_t comilla1 = linea.find('"', pos);
            size_t comilla2 = linea.find('"', comilla1 + 1);
            // si tiene comillas es un archivo .kiw externo (ES DECIR POR EJEMPLO: #incluir "mate.kiw")
            if (comilla1 != string::npos && comilla2 != string::npos) {
                string rutaLib = linea.substr(comilla1 + 1, comilla2 - comilla1 - 1);
                if (!incluidos.count(rutaLib)) {
                    incluidos.insert(rutaLib);
                    string contenidoLib = leerArchivo(rutaLib);
                    resultado += preprocesar(contenidoLib, incluidos) + "\n";
                }
                continue;}}resultado += linea + "\n";}return resultado;}

// EL MOTOR DEL LEXER

vector<Token> tokenizar(const string& codigo) {
    vector<Token> listaTokens;
    size_t i = 0;

    while (i < codigo.length()) {
        char actual = codigo[i];

        if (isspace(actual)) { i++; continue; }

        if (actual == '>' && i + 1 < codigo.length() && codigo[i+1] == '>') {
            while (i < codigo.length() && codigo[i] != '\n') i++;
            continue;
        }

        if (actual == '{' || actual == '}' || actual == '(' || actual == ')' || 
            actual == ';' || actual == '<' || actual == '>' || actual == '=' ||
            actual == '+' || actual == '-' || actual == '*' || actual == '/' || 
            actual == '!' || actual == ',') {
            
            string sim = string(1, actual);
            if ((actual == '=' || actual == '!' || actual == '<' || actual == '>') 
                && i + 1 < codigo.length() && codigo[i+1] == '=') {
                sim += "="; 
                i++;
            }
            listaTokens.push_back({TipoToken::SIMBOLO, sim});
            i++;
            continue;
        }

        if (isalpha(actual) || actual == '#' || actual == '&') {
            string palabra = "";
            while (i < codigo.length() && (isalnum(codigo[i]) || 
                   codigo[i] == '#' || codigo[i] == '.' || codigo[i] == '&')) {
                palabra += codigo[i];
                i++;
            }
            
            if (palabra == "ent" || palabra == "texto" || palabra == "dual" || palabra == "dec" || palabra == "vacio" ||
                palabra == "retorna" || palabra == "salida" || palabra == "entrada" ||
                palabra == "si" || palabra == "sino" || palabra == "demas" || palabra == "y" || palabra == "o" ||
                palabra == "mientras" || palabra == "para" || palabra == "rompe" || palabra == "salta" ||
                palabra == "intenta" || palabra == "excepto" || palabra == "estructura" || palabra == "nuevo" ||
                palabra == "contrato" || palabra == "implementa") {
                listaTokens.push_back({TipoToken::PALABRA_CLAVE, palabra});
            } else if (palabra == "#incluir") {
                listaTokens.push_back({TipoToken::DIRECTIVA, palabra});
            } else {
                listaTokens.push_back({TipoToken::IDENTIFICADOR, palabra});
            }
            continue;
        }

        if (actual == '"') {
            string texto = "\x01";
            i++;
            while (i < codigo.length() && codigo[i] != '"') {
                texto += codigo[i]; i++;
            }
            listaTokens.push_back({TipoToken::CADENA, texto});
            i++;
            continue;
        }

        if (isdigit(actual)) {
            string numero = "";
            while (i < codigo.length() && isdigit(codigo[i])) {
                numero += codigo[i]; i++;
            }
            listaTokens.push_back({TipoToken::NUMERO, numero});
            continue;
        }

        i++; 
    }
    return listaTokens;
}

// ARVOL PRINCIPAL

class NodoAST {
public:
    virtual ~NodoAST() = default;
};

class NodoAsignacion : public NodoAST {
public:
    string tipoVariable, nombre, valorIzq, operador, valorDer;
    NodoAsignacion(string t, string n, string vI, string op = "", string vD = "") 
        : tipoVariable(t), nombre(n), valorIzq(vI), operador(op), valorDer(vD) {}
};

class NodoEntrada : public NodoAST {
public:
    string nombreVariable;
    NodoEntrada(string n) : nombreVariable(n) {}
};

class NodoSi : public NodoAST {
public:
    string valorIzq, operadorLogico, valorDer;
    vector<NodoAST*> bloqueSi, bloqueDemas;
    NodoSi(string vI, string op, string vD) : valorIzq(vI), operadorLogico(op), valorDer(vD) {}
    ~NodoSi() {
        for (auto n : bloqueSi) delete n;
        for (auto n : bloqueDemas) delete n;
    }
};

class NodoRompe : public NodoAST {};
class NodoSalta : public NodoAST {};

class NodoMientras : public NodoAST {
public:
    string valorIzq, operadorLogico, valorDer;
    vector<NodoAST*> bloque;
    NodoMientras(string vI, string op, string vD) : valorIzq(vI), operadorLogico(op), valorDer(vD) {}
    ~NodoMientras() { for (auto n : bloque) delete n; }
};

class NodoPara : public NodoAST {
public:
    NodoAST* inicializacion;
    string valorIzq, operadorLogico, valorDer;
    NodoAST* actualizacion;
    vector<NodoAST*> bloque;
    NodoPara(NodoAST* ini, string vI, string op, string vD, NodoAST* act) 
        : inicializacion(ini), valorIzq(vI), operadorLogico(op), valorDer(vD), actualizacion(act) {}
    ~NodoPara() { 
        delete inicializacion; delete actualizacion;
        for (auto n : bloque) delete n; 
    }
};

class NodoIntenta : public NodoAST {
public:
    vector<NodoAST*> bloqueIntenta, bloqueExcepto;
    ~NodoIntenta() {
        for (auto n : bloqueIntenta) delete n;
        for (auto n : bloqueExcepto) delete n;
    }
};

class NodoLlamadaFuncion : public NodoAST {
public:
    string nombreFuncion;
    vector<string> argumentos;
    string variableDestino;
    NodoLlamadaFuncion(string n, vector<string> args, string dest = "") 
        : nombreFuncion(n), argumentos(args), variableDestino(dest) {}
};

class NodoFuncion {
public:
    string tipoRetorno, nombre;
    vector<string> parametros;
    vector<NodoAST*> bloque;
};

class NodoEstructura {
public:
    string nombre;
    string contrato;
    vector<NodoAsignacion*> atributos;
    map<string, NodoFuncion*> metodos;
    ~NodoEstructura() {
        for (auto a : atributos) delete a;
        for (auto const& [nom, func] : metodos) {
            for (auto n : func->bloque) delete n;
            delete func;
        }
    }
};

class NodoInstancia : public NodoAST {
public:
    string nombreVar, nombreEstructura;
    NodoInstancia(string nV, string nE) : nombreVar(nV), nombreEstructura(nE) {}
};

class NodoPrograma : public NodoAST {
public:
    map<string, NodoFuncion*> funciones;
    map<string, NodoEstructura*> estructuras;
    map<string, vector<string>> contratos;
    ~NodoPrograma() {
        for (auto const& [nom, func] : funciones) {
            for (auto n : func->bloque) delete n;
            delete func;
        }
        for (auto const& [nom, est] : estructuras) delete est;
    }
};

// EL MOTOR DEL PARSER 

vector<NodoAST*> parsearBloque(const vector<Token>& tokens, size_t& pos, bool esperarLlaves = true) {
    vector<NodoAST*> instrucciones;
    if (esperarLlaves) pos++; 

    while (pos < tokens.size() && tokens[pos].valor != "}") {
        

        if (tokens[pos].tipo == TipoToken::PALABRA_CLAVE && 
           (tokens[pos].valor == "ent" || tokens[pos].valor == "texto" || 
            tokens[pos].valor == "dual" || tokens[pos].valor == "dec")) {
            string tipoVar = tokens[pos].valor; pos++;
            string nombreVar = tokens[pos].valor; pos += 2;
            
            if (tokens[pos].valor == "nuevo") {
                pos++; 
                string claseNom = tokens[pos].valor; pos += 3; 
                pos++; 
                instrucciones.push_back(new NodoInstancia(nombreVar, claseNom));
            } 
            else if (pos + 1 < tokens.size() && tokens[pos+1].valor == "(") {
                string nomFunc = tokens[pos].valor; pos += 2;
                vector<string> args;
                while (pos < tokens.size() && tokens[pos].valor != ")") {
                    if (tokens[pos].valor != ",") args.push_back(tokens[pos].valor);
                    pos++;
                }
                pos += 2;
                instrucciones.push_back(new NodoLlamadaFuncion(nomFunc, args, nombreVar));
            } 
            else {
                string valorIzq = tokens[pos].valor; pos++;
                string operador = "", valorDer = "";
                if (tokens[pos].tipo == TipoToken::SIMBOLO && tokens[pos].valor != ";") {
                    operador = tokens[pos].valor; pos++;
                    valorDer = tokens[pos].valor; pos++;
                }
                pos++; 
                instrucciones.push_back(new NodoAsignacion(tipoVar, nombreVar, valorIzq, operador, valorDer));
            }
        }
        else if (tokens[pos].tipo == TipoToken::IDENTIFICADOR && pos + 1 < tokens.size() && tokens[pos+1].valor == "=") {
            string nombreVar = tokens[pos].valor; pos += 2;
            
            if (tokens[pos].valor == "nuevo") {
                pos++;
                string claseNom = tokens[pos].valor; pos += 3;
                pos++;
                instrucciones.push_back(new NodoInstancia(nombreVar, claseNom));
            } 
            else if (pos + 1 < tokens.size() && tokens[pos+1].valor == "(") {
                string nomFunc = tokens[pos].valor; pos += 2;
                vector<string> args;
                while (pos < tokens.size() && tokens[pos].valor != ")") {
                    if (tokens[pos].valor != ",") args.push_back(tokens[pos].valor);
                    pos++;
                }
                pos += 2;
                instrucciones.push_back(new NodoLlamadaFuncion(nomFunc, args, nombreVar));
            } 
            else {
                string valorIzq = tokens[pos].valor; pos++;
                string operador = "", valorDer = "";
                if (tokens[pos].tipo == TipoToken::SIMBOLO && tokens[pos].valor != ";") {
                    operador = tokens[pos].valor; pos++;
                    valorDer = tokens[pos].valor; pos++;
                }
                pos++;
                instrucciones.push_back(new NodoAsignacion("", nombreVar, valorIzq, operador, valorDer));
            }
        }

        else if (tokens[pos].tipo == TipoToken::PALABRA_CLAVE && tokens[pos].valor == "entrada") {
            pos += 2;
            string variableDestino = tokens[pos].valor; pos += 3;
            instrucciones.push_back(new NodoEntrada(variableDestino));
        }

        else if (tokens[pos].tipo == TipoToken::PALABRA_CLAVE && tokens[pos].valor == "si") {
            pos += 2; 
            string vIzq = tokens[pos].valor; pos++;
            string op = tokens[pos].valor; pos++;
            string vDer = tokens[pos].valor; pos += 2; 
            
            NodoSi* nodoSi = new NodoSi(vIzq, op, vDer);
            nodoSi->bloqueSi = parsearBloque(tokens, pos, true); 
            
            if (pos < tokens.size() && tokens[pos].tipo == TipoToken::PALABRA_CLAVE && tokens[pos].valor == "demas") {
                pos++;
                nodoSi->bloqueDemas = parsearBloque(tokens, pos, true);
            }
            instrucciones.push_back(nodoSi);
        }

        else if (tokens[pos].tipo == TipoToken::PALABRA_CLAVE && tokens[pos].valor == "mientras") {
            pos += 2;
            string vIzq = tokens[pos].valor; pos++;
            string op = tokens[pos].valor; pos++;
            string vDer = tokens[pos].valor; pos += 2;
            
            NodoMientras* nodoMientras = new NodoMientras(vIzq, op, vDer);
            nodoMientras->bloque = parsearBloque(tokens, pos, true);
            instrucciones.push_back(nodoMientras);
        }

        else if (tokens[pos].tipo == TipoToken::PALABRA_CLAVE && tokens[pos].valor == "para") {
            pos += 2; 
            NodoAST* inicializacion = nullptr;
            if (tokens[pos].valor == "ent") {
                pos++;
                string nom = tokens[pos].valor; pos += 2; 
                string val = tokens[pos].valor; pos += 2; 
                inicializacion = new NodoAsignacion("ent", nom, val, "", "");
            }
            
            string vIzq = tokens[pos].valor; pos++;
            string op = tokens[pos].valor; pos++;
            string vDer = tokens[pos].valor; pos += 2; 
            
            NodoAST* actualizacion = nullptr;
            string nomAct = tokens[pos].valor; pos += 2; 
            string valActIzq = tokens[pos].valor; pos++;
            string opAct = tokens[pos].valor; pos++;
            string valActDer = tokens[pos].valor; pos += 2; 
            actualizacion = new NodoAsignacion("", nomAct, valActIzq, opAct, valActDer);
            
            NodoPara* nodoPara = new NodoPara(inicializacion, vIzq, op, vDer, actualizacion);
            nodoPara->bloque = parsearBloque(tokens, pos, true);
            instrucciones.push_back(nodoPara);
        }
        else if (tokens[pos].valor == "rompe") { pos += 2; instrucciones.push_back(new NodoRompe()); }
        else if (tokens[pos].valor == "salta") { pos += 2; instrucciones.push_back(new NodoSalta()); }
        else if (tokens[pos].valor == "intenta") {
            pos++;
            NodoIntenta* nodoInt = new NodoIntenta();
            nodoInt->bloqueIntenta = parsearBloque(tokens, pos, true);
            if (pos < tokens.size() && tokens[pos].valor == "excepto") {
                pos++;
                nodoInt->bloqueExcepto = parsearBloque(tokens, pos, true);
            }
            instrucciones.push_back(nodoInt);
        }
        // porque no?
        else if (tokens[pos].tipo == TipoToken::IDENTIFICADOR || tokens[pos].valor == "salida") {
            if (pos + 1 < tokens.size() && tokens[pos+1].valor == "(") {
                string nomFunc = tokens[pos].valor; pos += 2;
                vector<string> args;
                while (pos < tokens.size() && tokens[pos].valor != ")") {
                    if (tokens[pos].valor != ",") args.push_back(tokens[pos].valor);
                    pos++;
                }
                pos += 2; 
                instrucciones.push_back(new NodoLlamadaFuncion(nomFunc, args));
            } else { pos++; }
        }
        else if (tokens[pos].valor == "retorna") {
            pos++; 
            string valRetorno = tokens[pos].valor; pos += 2; 
            instrucciones.push_back(new NodoAsignacion("", "__retorno__", valRetorno, "", ""));
        }
        else { pos++; }
    }
    if (esperarLlaves) pos++; 
    return instrucciones;
}

NodoPrograma* parsear(const vector<Token>& tokens) {
    NodoPrograma* programa = new NodoPrograma();
    size_t pos = 0;
    while (pos < tokens.size()) {
        if (tokens[pos].tipo == TipoToken::PALABRA_CLAVE && tokens[pos].valor == "contrato") {
            pos++;
            string nomContrato = tokens[pos].valor; pos += 2;
            vector<string> firmas;
            while (pos < tokens.size() && tokens[pos].valor != "}") {
                if (tokens[pos].valor == "ent" || tokens[pos].valor == "texto" || 
                    tokens[pos].valor == "vacio" || tokens[pos].valor == "dual" || tokens[pos].valor == "dec") {
                    pos++;
                    if (pos < tokens.size()) {
                        firmas.push_back(tokens[pos].valor);
                        pos++;
                    }
                    while (pos < tokens.size() && tokens[pos].valor != ";" && tokens[pos].valor != "}") pos++;
                    if (pos < tokens.size() && tokens[pos].valor == ";") pos++;
                } else { pos++; }
            }
            if (pos < tokens.size()) pos++;
            programa->contratos[nomContrato] = firmas;
        }
        else if (tokens[pos].tipo == TipoToken::PALABRA_CLAVE && tokens[pos].valor == "estructura") {
            pos++;
            string nomEstructura = tokens[pos].valor; pos++;
            NodoEstructura* nEst = new NodoEstructura();
            nEst->nombre = nomEstructura;
            if (pos < tokens.size() && tokens[pos].valor == "implementa") {
                pos++;
                nEst->contrato = tokens[pos].valor; pos++;
            }
            pos++;
            while (pos < tokens.size() && tokens[pos].valor != "}") {
                if (tokens[pos].valor == "ent" || tokens[pos].valor == "texto" || 
                    tokens[pos].valor == "dual" || tokens[pos].valor == "dec" || tokens[pos].valor == "vacio") {
                    string t = tokens[pos].valor; pos++;
                    string n = tokens[pos].valor; pos++;
                    if (pos < tokens.size() && tokens[pos].valor == "(") {
                        NodoFuncion* met = new NodoFuncion();
                        met->tipoRetorno = t;
                        met->nombre = n;
                        pos++;
                        while (pos < tokens.size() && tokens[pos].valor != ")") {
                            if (tokens[pos].tipo == TipoToken::PALABRA_CLAVE) pos++;
                            if (tokens[pos].tipo == TipoToken::IDENTIFICADOR) {
                                met->parametros.push_back(tokens[pos].valor);
                                pos++;
                            } else pos++;
                        }
                        pos++;
                        met->bloque = parsearBloque(tokens, pos, true);
                        nEst->metodos[n] = met;
                    } else {
                        pos++;
                        string v = tokens[pos].valor; pos += 2;
                        nEst->atributos.push_back(new NodoAsignacion(t, n, v, "", ""));
                    }
                } else { pos++; }
            }
            if (pos < tokens.size()) pos++;
            programa->estructuras[nomEstructura] = nEst;
        }
        else if (tokens[pos].tipo == TipoToken::PALABRA_CLAVE && 
           (tokens[pos].valor == "ent" || tokens[pos].valor == "texto" || tokens[pos].valor == "vacio" || tokens[pos].valor == "dual" || tokens[pos].valor == "dec")) {
            NodoFuncion* nuevaFunc = new NodoFuncion();
            nuevaFunc->tipoRetorno = tokens[pos].valor; pos++;
            nuevaFunc->nombre = tokens[pos].valor; pos++; 
            pos++; 
            while (pos < tokens.size() && tokens[pos].valor != ")") {
                if (tokens[pos].tipo == TipoToken::PALABRA_CLAVE) pos++; 
                if (tokens[pos].tipo == TipoToken::IDENTIFICADOR) {
                    nuevaFunc->parametros.push_back(tokens[pos].valor);
                    pos++; 
                }
                else pos++;
            }
            pos++; 
            nuevaFunc->bloque = parsearBloque(tokens, pos, true);
            programa->funciones[nuevaFunc->nombre] = nuevaFunc;
        } else {
            pos++;
        }
    }
    return programa;
}

// HERRAMIENTAS MATEMÁTICAS Y LÓGICAS
bool esNumero(const string& str) {
    if(str.empty()) return false;
    size_t start = (str[0] == '-') ? 1 : 0;
    if(start == 1 && str.length() == 1) return false;
    for(size_t i = start; i < str.length(); i++) if(!isdigit(str[i])) return false;
    return true;
}

string resolverToken(const string& tok, map<string, string>& memoria) {
    if (!tok.empty() && tok[0] == '\x01') return tok.substr(1);
    return memoria.count(tok) ? memoria[tok] : tok;
}

string evaluarMatematica(string v1, string op, string v2, map<string, string>& memoria, string tipoDest = "") {
    bool lit1 = (!v1.empty() && v1[0] == '\x01');
    bool lit2 = (!v2.empty() && v2[0] == '\x01');
    string val1 = resolverToken(v1, memoria);
    string val2 = resolverToken(v2, memoria);

    if (op == "+" && (lit1 || lit2 || tipoDest == "texto")) {
        return val1 + val2;
    }
    if (op == "*" && (!esNumero(val1) || !esNumero(val2))) {
        string texto = !esNumero(val1) ? val1 : val2;
        int veces = stoi(!esNumero(val1) ? val2 : val1);
        string resultado = "";
        for (int i = 0; i < veces; i++) resultado += texto;
        return resultado; 
    }
    if (esNumero(val1) && esNumero(val2)) {
        int n1 = stoi(val1);
        int n2 = stoi(val2);
        if (op == "+") return to_string(n1 + n2);
        if (op == "-") return to_string(n1 - n2);
        if (op == "*") return to_string(n1 * n2);
        if (op == "/") {
            if (n2 == 0) throw runtime_error("Division por cero");
            return to_string(n1 / n2);
        }
    }

    if (op == "+") return val1 + val2;
    return val1; 
}
bool evaluarLogica(string v1, string op, string v2, map<string, string>& memoria) {
    string val1 = resolverToken(v1, memoria);
    string val2 = resolverToken(v2, memoria);

    if (esNumero(val1) && esNumero(val2)) {
        int n1 = stoi(val1); int n2 = stoi(val2);
        if (op == "==") return n1 == n2;
        if (op == "!=") return n1 != n2;
        if (op == "<") return n1 < n2;
        if (op == ">") return n1 > n2;
        if (op == "<=") return n1 <= n2;
        if (op == ">=") return n1 >= n2;
    } else { 
        if (op == "==") return val1 == val2;
        if (op == "!=") return val1 != val2;
    }
    return false;
}
// EJECUTOR Y API NATIVA
int ejecutarInstrucciones(const vector<NodoAST*>& instrucciones, map<string, string>& memoria, NodoPrograma* programa) {
    for (auto nodo : instrucciones) {
        
        if (auto instancia = dynamic_cast<NodoInstancia*>(nodo)) {
            if (programa->estructuras.count(instancia->nombreEstructura)) {
                NodoEstructura* estObj = programa->estructuras[instancia->nombreEstructura];
                if (!estObj->contrato.empty()) {
                    if (!programa->contratos.count(estObj->contrato)) {
                        throw runtime_error("El contrato '" + estObj->contrato + "' no existe");
                    }
                    for (const string& req : programa->contratos[estObj->contrato]) {
                        if (!estObj->metodos.count(req)) {
                            throw runtime_error("Contrato '" + estObj->contrato + "' incumplido en '" + estObj->nombre + "': falta '" + req + "'");
                        }
                    }
                }
                memoria[instancia->nombreVar + ".__clase__"] = instancia->nombreEstructura;
                for (auto attr : estObj->atributos) {
                    memoria[instancia->nombreVar + "." + attr->nombre] = resolverToken(attr->valorIzq, memoria);
                }
            } else {
                cerr << "Error KIWII: La estructura '" << instancia->nombreEstructura << "' no existe." << endl;
            }
        }
        else if (auto asignacion = dynamic_cast<NodoAsignacion*>(nodo)) {
            if (asignacion->operador == "") {
                memoria[asignacion->nombre] = resolverToken(asignacion->valorIzq, memoria);
            }
            else memoria[asignacion->nombre] = evaluarMatematica(asignacion->valorIzq, asignacion->operador, asignacion->valorDer, memoria, asignacion->tipoVariable);
            
            if (asignacion->nombre == "__retorno__") return 3;
        }
        else if (auto entrada = dynamic_cast<NodoEntrada*>(nodo)) {
            string valorIngresado; getline(cin, valorIngresado); 
            memoria[entrada->nombreVariable] = valorIngresado; 
        }
        else if (auto nodoSi = dynamic_cast<NodoSi*>(nodo)) {
            if (evaluarLogica(nodoSi->valorIzq, nodoSi->operadorLogico, nodoSi->valorDer, memoria)) {
                int est = ejecutarInstrucciones(nodoSi->bloqueSi, memoria, programa); if (est != 0) return est;
            } else if (!nodoSi->bloqueDemas.empty()) {
                int est = ejecutarInstrucciones(nodoSi->bloqueDemas, memoria, programa); if (est != 0) return est;
            }
        }
        else if (auto nodoMientras = dynamic_cast<NodoMientras*>(nodo)) {
            int ciclos = 0;
            while (evaluarLogica(nodoMientras->valorIzq, nodoMientras->operadorLogico, nodoMientras->valorDer, memoria)) {
                if (++ciclos > LIMITE_CICLOS) throw runtime_error("Limite de iteraciones excedido (Proteccion DoS)");
                int est = ejecutarInstrucciones(nodoMientras->bloque, memoria, programa);
                if (est == 1) break; if (est == 2) continue; if (est == 3) return 3;
            }
        }
        else if (auto nodoPara = dynamic_cast<NodoPara*>(nodo)) {
            int ciclos = 0;
            vector<NodoAST*> ini = {nodoPara->inicializacion}; ejecutarInstrucciones(ini, memoria, programa);
            while (evaluarLogica(nodoPara->valorIzq, nodoPara->operadorLogico, nodoPara->valorDer, memoria)) {
                if (++ciclos > LIMITE_CICLOS) throw runtime_error("Limite de iteraciones excedido (Proteccion DoS)");
                int est = ejecutarInstrucciones(nodoPara->bloque, memoria, programa);
                if (est == 1) break; if (est == 3) return 3;
                vector<NodoAST*> act = {nodoPara->actualizacion}; ejecutarInstrucciones(act, memoria, programa);
            }
        }
        else if (auto nodoInt = dynamic_cast<NodoIntenta*>(nodo)) {
            int pilaBase = profundidadPila;
            try {
                int est = ejecutarInstrucciones(nodoInt->bloqueIntenta, memoria, programa);
                if (est != 0) return est;
            } catch (...) {
                profundidadPila = pilaBase;
                if (!nodoInt->bloqueExcepto.empty()) {
                    int est = ejecutarInstrucciones(nodoInt->bloqueExcepto, memoria, programa);
                    if (est != 0) return est;
                }
            }
        }
        else if (auto llamada = dynamic_cast<NodoLlamadaFuncion*>(nodo)) {

            auto obtenerVal = [&](size_t idx) -> string {
                if (idx >= llamada->argumentos.size()) return "";
                return resolverToken(llamada->argumentos[idx], memoria);
            };
            string resultadoRetorno = "";
            size_t puntoPos = llamada->nombreFuncion.find('.');

            if (llamada->nombreFuncion == "salida") {
                cout << obtenerVal(0) << endl;
            } 
            else if (llamada->nombreFuncion == "largo") {
                resultadoRetorno = to_string(obtenerVal(0).length());
            }
            else if (llamada->nombreFuncion == "letraen") {
                string txt = obtenerVal(0);
                int idx = stoi(obtenerVal(1));
                if (idx >= 0 && idx < (int)txt.length()) resultadoRetorno = string(1, txt[idx]);
            }
            else if (llamada->nombreFuncion == "codigoascii") {
                string txt = obtenerVal(0);
                resultadoRetorno = txt.empty() ? "0" : to_string((int)(unsigned char)txt[0]);
            }
            else if (llamada->nombreFuncion == "leerarchivo") {
                if (modoAislado) {
                    cerr << "[Sandbox KIWII]: Lectura de archivos bloqueada en modo --aislado" << endl;
                    resultadoRetorno = "";
                } else {
                    resultadoRetorno = leerArchivo(obtenerVal(0));
                }
            }
            else if (puntoPos != string::npos) {
                string objNom = llamada->nombreFuncion.substr(0, puntoPos);
                string metNom = llamada->nombreFuncion.substr(puntoPos + 1);
                string claseKey = objNom + ".__clase__";
                if (memoria.count(claseKey) && programa->estructuras.count(memoria[claseKey])) {
                    NodoEstructura* estObj = programa->estructuras[memoria[claseKey]];
                    if (estObj->metodos.count(metNom)) {
                        if (++profundidadPila > LIMITE_PILA) {
                            profundidadPila--;
                            throw runtime_error("Desbordamiento de pila bloqueado (Proteccion DoS)");
                        }
                        NodoFuncion* funcObj = estObj->metodos[metNom];
                        map<string, string> memoriaLocal;
                        memoriaLocal["este.__clase__"] = estObj->nombre;
                        for (auto attr : estObj->atributos) {
                            memoriaLocal["este." + attr->nombre] = memoria[objNom + "." + attr->nombre];
                        }
                        vector<string> refOriginal, refLocal;
                        for (size_t k = 0; k < funcObj->parametros.size() && k < llamada->argumentos.size(); k++) {
                            string param = funcObj->parametros[k];
                            string arg = llamada->argumentos[k];
                            if (param[0] == '&') {
                                string paramReal = param.substr(1);
                                memoriaLocal[paramReal] = resolverToken(arg, memoria);
                                refOriginal.push_back(arg);
                                refLocal.push_back(paramReal);
                            } else {
                                memoriaLocal[param] = resolverToken(arg, memoria);
                            }
                        }
                        ejecutarInstrucciones(funcObj->bloque, memoriaLocal, programa);
                        profundidadPila--;
                        for (auto attr : estObj->atributos) {
                            memoria[objNom + "." + attr->nombre] = memoriaLocal["este." + attr->nombre];
                        }
                        for (size_t k = 0; k < refOriginal.size(); k++) {
                            memoria[refOriginal[k]] = memoriaLocal[refLocal[k]];
                        }
                        if (memoriaLocal.count("__retorno__")) {
                            resultadoRetorno = memoriaLocal["__retorno__"];
                        }
                    }
                }
            }
            else if (programa->funciones.count(llamada->nombreFuncion)) {
                if (++profundidadPila > LIMITE_PILA) {
                    profundidadPila--;
                    throw runtime_error("Desbordamiento de pila bloqueado (Proteccion DoS)");
                }
                NodoFuncion* funcObj = programa->funciones[llamada->nombreFuncion];
                map<string, string> memoriaLocal; 
                
                vector<string> refOriginal, refLocal;
                
                for (size_t k = 0; k < funcObj->parametros.size() && k < llamada->argumentos.size(); k++) {
                    string param = funcObj->parametros[k];
                    string arg = llamada->argumentos[k];
                    
                    if (param[0] == '&') {
                        string paramReal = param.substr(1); 
                        memoriaLocal[paramReal] = resolverToken(arg, memoria);
                        refOriginal.push_back(arg);
                        refLocal.push_back(paramReal);
                    } else {
                        memoriaLocal[param] = resolverToken(arg, memoria);
                    }
                }
                ejecutarInstrucciones(funcObj->bloque, memoriaLocal, programa);
                profundidadPila--;
                for (size_t k = 0; k < refOriginal.size(); k++) {
                    memoria[refOriginal[k]] = memoriaLocal[refLocal[k]];
                }
                if (memoriaLocal.count("__retorno__")) {
                    resultadoRetorno = memoriaLocal["__retorno__"];
                }
            }
            if (!llamada->variableDestino.empty()) {
                memoria[llamada->variableDestino] = resultadoRetorno;
            }
        }
        else if (dynamic_cast<NodoRompe*>(nodo)) return 1;
        else if (dynamic_cast<NodoSalta*>(nodo)) return 2;
    }
    return 0; 
}

void ejecutar(NodoPrograma* arbol) {
    map<string, string> memoriaGlobal;
    if (arbol->funciones.count("principal")) {
        ejecutarInstrucciones(arbol->funciones["principal"]->bloque, memoriaGlobal, arbol);
    } else {
        cerr << "Error: KIWII no encontro la funcion 'principal()'." << endl;
    }
}

//FALTA AGREGAR PARA LECTURA NETWORKING (...no lo esperes xd)

// PUNTO DE ENTRADA 

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "KIWII Kernel v1.5 | Uso: kiwii <archivo.kiw> [--debug] [--aislado]" << endl;
        return 1;
    }
    string bin = argv[0];
    size_t barra = bin.find_last_of("/\\");
    if (barra != string::npos) rutaKiwii = bin.substr(0, barra + 1);
    string rutaArchivo = argv[1];
    bool modoDebug = false;
    for (int i = 2; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--debug") modoDebug = true;
        if (arg == "--aislado") modoAislado = true;
    }
    string codigoBruto = leerArchivo(rutaArchivo);
    if (codigoBruto.empty()) return 1;
    set<string> archivosIncluidos = {rutaArchivo};
    string codigoFuente = preprocesar(codigoBruto, archivosIncluidos);
    vector<Token> tokens = tokenizar(codigoFuente);
    NodoPrograma* arbol = parsear(tokens);
    if (modoDebug) {
        cout << " Archivos fusionados: " << archivosIncluidos.size() << endl;
        cout << " Tokens generados: " << tokens.size() << endl;
        cout << " Funciones en memoria: " << arbol->funciones.size() << endl;
        cout << " Estructuras en memoria: " << arbol->estructuras.size() << endl;
        cout << " Modo aislado (Sandbox): " << (modoAislado ? "ACTIVO" : "INACTIVO") << endl;
        cout << "================ SALIDA ================" << endl;
    }
    try {
        ejecutar(arbol);
    } catch (const exception& e) {
        cerr << "[Aborto Seguro Kernel]: " << e.what() << endl;
    }
    if (modoDebug) {
        cout << "=================================================" << endl;
    }
    delete arbol;
    return 0;
}