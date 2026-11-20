#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <cstdlib>
#include <cmath>
#include <stdexcept>
#include <iomanip>
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
    int linea;
};

enum class TipoVal {
    ENT,
    DEC,
    TEXTO
};

bool esEnteroStr(const string& str) {
    if (str.empty()) return false;
    size_t start = (str[0] == '-') ? 1 : 0;
    if (start == 1 && str.length() == 1) return false;
    for (size_t i = start; i < str.length(); i++) {
        if (!isdigit((unsigned char)str[i])) return false;
    }
    return true;
}

bool esNumeroStr(const string& str) {
    if (str.empty()) return false;
    size_t start = (str[0] == '-') ? 1 : 0;
    if (start == 1 && str.length() == 1) return false;
    bool punto = false;
    bool dig = false;
    for (size_t i = start; i < str.length(); i++) {
        if (str[i] == '.') {
            if (punto) return false;
            punto = true;
        } else if (isdigit((unsigned char)str[i])) {
            dig = true;
        } else {
            return false;
        }
    }
    return dig;
}

string formatearDecimal(double d) {
    if (d == 0.0) return "0";
    ostringstream oss;
    oss << fixed << setprecision(6) << d;
    string s = oss.str();
    s.erase(s.find_last_not_of('0') + 1, string::npos);
    if (!s.empty() && s.back() == '.') s.pop_back();
    return s;
}

struct Valor {
    TipoVal tipo = TipoVal::ENT;
    long long entVal = 0;
    double decVal = 0.0;
    string str = "";

    static Valor desdeEnt(long long v) {
        Valor res;
        res.tipo = TipoVal::ENT;
        res.entVal = v;
        return res;
    }

    static Valor desdeDec(double v) {
        Valor res;
        res.tipo = TipoVal::DEC;
        res.decVal = v;
        return res;
    }

    static Valor desdeTexto(const string& v) {
        Valor res;
        res.tipo = TipoVal::TEXTO;
        res.str = v;
        return res;
    }

    string aTexto() const {
        if (tipo == TipoVal::ENT) return to_string(entVal);
        if (tipo == TipoVal::DEC) return formatearDecimal(decVal);
        return str;
    }

    bool esNumerico() const {
        if (tipo == TipoVal::ENT || tipo == TipoVal::DEC) return true;
        return esNumeroStr(str);
    }

    bool tienePunto() const {
        if (tipo == TipoVal::DEC) return true;
        if (tipo == TipoVal::TEXTO) return str.find('.') != string::npos;
        return false;
    }

    long long aEnt() const {
        if (tipo == TipoVal::ENT) return entVal;
        if (tipo == TipoVal::DEC) return (long long)decVal;
        if (esNumeroStr(str)) return (long long)stod(str);
        throw runtime_error("No se puede convertir '" + str + "' a entero");
    }

    double aDec() const {
        if (tipo == TipoVal::ENT) return (double)entVal;
        if (tipo == TipoVal::DEC) return decVal;
        if (esNumeroStr(str)) return stod(str);
        throw runtime_error("No se puede convertir '" + str + "' a decimal");
    }

    bool esVerdadero() const {
        if (tipo == TipoVal::ENT) return entVal != 0;
        if (tipo == TipoVal::DEC) return decVal != 0.0;
        return !str.empty() && str != "0";
    }
};

bool modoAislado = false;
int profundidadPila = 0;
const int LIMITE_CICLOS = 100000;
const int LIMITE_PILA = 256;
string rutaKiwii = "";

string formatearUbicacion(int linGlobal, const vector<pair<string, int>>& mapaLineas) {
    if (linGlobal >= 1 && (size_t)linGlobal <= mapaLineas.size()) {
        return mapaLineas[(size_t)linGlobal - 1].first + " Linea " + to_string(mapaLineas[(size_t)linGlobal - 1].second);
    }
    return "Linea " + to_string(linGlobal);
}

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