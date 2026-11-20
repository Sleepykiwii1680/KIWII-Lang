#pragma once
#include "tipos.hpp"

// FuZion de archivos #incluir "otro_archivo.kiw" antes de pasar al Lexer... no preguntes xd
string preprocesar(const string& codigo, const string& archivoActual, set<string>& incluidos, vector<pair<string, int>>& mapaLineas) {
    stringstream ss(codigo);
    string linea, resultado = "";
    int numLinea = 0;
    
    while (getline(ss, linea)) {
        numLinea++;
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
                    resultado += preprocesar(contenidoLib, rutaLib, incluidos, mapaLineas);
                }
                continue;}}resultado += linea + "\n"; mapaLineas.push_back({archivoActual, numLinea});}return resultado;}

// EL MOTOR DEL LEXER

vector<Token> tokenizar(const string& codigo, const vector<pair<string, int>>& mapaLineas) {
    vector<Token> listaTokens;
    size_t i = 0;
    int linea = 1;

    if (codigo.size() >= 3 && (unsigned char)codigo[0] == 0xEF &&
        (unsigned char)codigo[1] == 0xBB && (unsigned char)codigo[2] == 0xBF) {
        i = 3;
    }

    while (i < codigo.length()) {
        char actual = codigo[i];

        if (actual == '\n') { linea++; i++; continue; }
        if (isspace((unsigned char)actual)) { i++; continue; }

        if (actual == '>' && i + 1 < codigo.length() && codigo[i+1] == '>') {
            while (i < codigo.length() && codigo[i] != '\n') i++;
            continue;
        }

        if (actual == '{' || actual == '}' || actual == '(' || actual == ')' || 
            actual == ';' || actual == '<' || actual == '>' || actual == '=' ||
            actual == '+' || actual == '-' || actual == '*' || actual == '/' || 
            actual == '%' || actual == '!' || actual == ',') {
            
            string sim = string(1, actual);
            if ((actual == '=' || actual == '!' || actual == '<' || actual == '>') 
                && i + 1 < codigo.length() && codigo[i+1] == '=') {
                sim += "="; 
                i++;
            }
            listaTokens.push_back({TipoToken::SIMBOLO, sim, linea});
            i++;
            continue;
        }

        if (isalpha((unsigned char)actual) || actual == '#' || actual == '&') {
            string palabra = "";
            while (i < codigo.length() && (isalnum((unsigned char)codigo[i]) || 
                   codigo[i] == '#' || codigo[i] == '.' || codigo[i] == '&')) {
                palabra += codigo[i];
                i++;
            }
            
            if (palabra == "ent" || palabra == "texto" || palabra == "dual" || palabra == "dec" || palabra == "vacio" ||
                palabra == "retorna" || palabra == "salida" || palabra == "entrada" ||
                palabra == "si" || palabra == "sino" || palabra == "demas" || palabra == "y" || palabra == "o" ||
                palabra == "no" || palabra == "verdadero" || palabra == "falso" ||
                palabra == "mientras" || palabra == "para" || palabra == "rompe" || palabra == "salta" ||
                palabra == "intenta" || palabra == "excepto" || palabra == "estructura" || palabra == "nuevo" ||
                palabra == "contrato" || palabra == "implementa") {
                listaTokens.push_back({TipoToken::PALABRA_CLAVE, palabra, linea});
            } else if (palabra == "#incluir") {
                listaTokens.push_back({TipoToken::DIRECTIVA, palabra, linea});
            } else {
                listaTokens.push_back({TipoToken::IDENTIFICADOR, palabra, linea});
            }
            continue;
        }

        if (actual == '"') {
            string texto = "";
            int lineaInicio = linea;
            i++;
            while (i < codigo.length() && codigo[i] != '"') {
                if (codigo[i] == '\\' && i + 1 < codigo.length()) {
                    char sig = codigo[i+1];
                    if (sig == 'n') { texto += '\n'; i += 2; continue; }
                    if (sig == 't') { texto += '\t'; i += 2; continue; }
                    if (sig == '"') { texto += '"'; i += 2; continue; }
                    if (sig == '\\') { texto += '\\'; i += 2; continue; }
                }
                if (codigo[i] == '\n') linea++;
                texto += codigo[i];
                i++;
            }
            if (i >= codigo.length()) {
                throw runtime_error("[Error Sintaxis en " + formatearUbicacion(lineaInicio, mapaLineas) + "]: Cadena de texto sin cerrar");
            }
            listaTokens.push_back({TipoToken::CADENA, texto, lineaInicio});
            i++;
            continue;
        }

        if (isdigit((unsigned char)actual)) {
            string numero = "";
            bool tienePunto = false;
            while (i < codigo.length() && (isdigit((unsigned char)codigo[i]) ||
                   (codigo[i] == '.' && !tienePunto && i + 1 < codigo.length() && isdigit((unsigned char)codigo[i+1])))) {
                if (codigo[i] == '.') tienePunto = true;
                numero += codigo[i];
                i++;
            }
            listaTokens.push_back({TipoToken::NUMERO, numero, linea});
            continue;
        }

        throw runtime_error("[Error Sintaxis en " + formatearUbicacion(linea, mapaLineas) + "]: Caracter no reconocido '" + string(1, actual) + "'");
    }
    return listaTokens;
}