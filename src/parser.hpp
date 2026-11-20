#pragma once
#include "tipos.hpp"
#include "ast.hpp"

// EL MOTOR DEL PARSER 

class Parser {
    const vector<Token>& tokens;
    const vector<pair<string, int>>& mapaLineas;
    size_t pos = 0;

    bool esFin() const { return pos >= tokens.size(); }

    string ubicacion(int linGlobal) const {
        return formatearUbicacion(linGlobal, mapaLineas);
    }

    const Token& actual() const {
        if (esFin()) {
            int lin = tokens.empty() ? 1 : tokens.back().linea;
            throw runtime_error("[Error Sintaxis en " + ubicacion(lin) + "]: Fin de archivo inesperado");
        }
        return tokens[pos];
    }

    bool esSimbolo(const string& val) const {
        return !esFin() && tokens[pos].tipo == TipoToken::SIMBOLO && tokens[pos].valor == val;
    }

    bool esClave(const string& val) const {
        return !esFin() && tokens[pos].tipo == TipoToken::PALABRA_CLAVE && tokens[pos].valor == val;
    }

    bool siguienteEsSimbolo(const string& val) const {
        return (pos + 1 < tokens.size() && tokens[pos + 1].tipo == TipoToken::SIMBOLO && tokens[pos + 1].valor == val);
    }

    bool esTipoDato(const Token& tok) const {
        return tok.tipo == TipoToken::PALABRA_CLAVE &&
               (tok.valor == "ent" || tok.valor == "texto" || tok.valor == "dual" || tok.valor == "dec");
    }

    bool esTipoRetorno(const Token& tok) const {
        return esTipoDato(tok) || (tok.tipo == TipoToken::PALABRA_CLAVE && tok.valor == "vacio");
    }

    bool coincideSimbolo(const string& val) {
        if (esSimbolo(val)) { pos++; return true; }
        return false;
    }

    bool coincideClave(const string& val) {
        if (esClave(val)) { pos++; return true; }
        return false;
    }

    void consumirSimbolo(const string& esperado, const string& msg) {
        if (esFin()) {
            int lin = tokens.empty() ? 1 : tokens.back().linea;
            throw runtime_error("[Error Sintaxis en " + ubicacion(lin) + "]: " + msg + " (fin de archivo)");
        }
        if (tokens[pos].tipo != TipoToken::SIMBOLO || tokens[pos].valor != esperado) {
            throw runtime_error("[Error Sintaxis en " + ubicacion(tokens[pos].linea) + "]: " + msg + ", se encontro '" + tokens[pos].valor + "'");
        }
        pos++;
    }

    string consumirIdentificador(const string& msg) {
        if (esFin() || tokens[pos].tipo != TipoToken::IDENTIFICADOR) {
            int lin = esFin() ? (tokens.empty() ? 1 : tokens.back().linea) : tokens[pos].linea;
            string enc = esFin() ? "fin de archivo" : tokens[pos].valor;
            throw runtime_error("[Error Sintaxis en " + ubicacion(lin) + "]: " + msg + ", se encontro '" + enc + "'");
        }
        return tokens[pos++].valor;
    }

    NodoExpr* parsearExpresion() { return parsearO(); }

    NodoExpr* parsearO() {
        NodoExpr* izq = parsearY();
        while (esClave("o")) {
            string op = tokens[pos++].valor;
            izq = new NodoBinario(izq, op, parsearY());
        }
        return izq;
    }

    NodoExpr* parsearY() {
        NodoExpr* izq = parsearIgualdad();
        while (esClave("y")) {
            string op = tokens[pos++].valor;
            izq = new NodoBinario(izq, op, parsearIgualdad());
        }
        return izq;
    }

    NodoExpr* parsearIgualdad() {
        NodoExpr* izq = parsearComparacion();
        while (esSimbolo("==") || esSimbolo("!=")) {
            string op = tokens[pos++].valor;
            izq = new NodoBinario(izq, op, parsearComparacion());
        }
        return izq;
    }

    NodoExpr* parsearComparacion() {
        NodoExpr* izq = parsearTermino();
        while (esSimbolo("<") || esSimbolo(">") || esSimbolo("<=") || esSimbolo(">=")) {
            string op = tokens[pos++].valor;
            izq = new NodoBinario(izq, op, parsearTermino());
        }
        return izq;
    }

    NodoExpr* parsearTermino() {
        NodoExpr* izq = parsearFactor();
        while (esSimbolo("+") || esSimbolo("-")) {
            string op = tokens[pos++].valor;
            izq = new NodoBinario(izq, op, parsearFactor());
        }
        return izq;
    }

    NodoExpr* parsearFactor() {
        NodoExpr* izq = parsearUnario();
        while (esSimbolo("*") || esSimbolo("/") || esSimbolo("%")) {
            string op = tokens[pos++].valor;
            izq = new NodoBinario(izq, op, parsearUnario());
        }
        return izq;
    }

    NodoExpr* parsearUnario() {
        if (esSimbolo("-") || esSimbolo("!") || esClave("no")) {
            string op = tokens[pos++].valor;
            return new NodoUnario(op, parsearUnario());
        }
        return parsearPrimario();
    }

    NodoExpr* parsearPrimario() {
        const Token& tok = actual();

        if (tok.tipo == TipoToken::NUMERO) {
            pos++;
            return new NodoNumero(tok.valor);
        }
        if (tok.tipo == TipoToken::CADENA) {
            pos++;
            return new NodoCadena(tok.valor);
        }
        if (esClave("verdadero")) {
            pos++;
            return new NodoNumero(Valor::desdeEnt(1));
        }
        if (esClave("falso")) {
            pos++;
            return new NodoNumero(Valor::desdeEnt(0));
        }
        if (esSimbolo("(")) {
            pos++;
            NodoExpr* expr = parsearExpresion();
            consumirSimbolo(")", "Se esperaba ')' para cerrar la expresion");
            return expr;
        }
        if (tok.tipo == TipoToken::IDENTIFICADOR || esClave("salida")) {
            string nom = tokens[pos++].valor;
            if (esSimbolo("(")) {
                pos++;
                vector<NodoExpr*> args;
                if (!esSimbolo(")")) {
                    args.push_back(parsearExpresion());
                    while (coincideSimbolo(",")) {
                        args.push_back(parsearExpresion());
                    }
                }
                consumirSimbolo(")", "Se esperaba ')' en la llamada a '" + nom + "'");
                return new NodoLlamadaExpr(nom, args);
            }
            return new NodoVariable(nom);
        }

        throw runtime_error("[Error Sintaxis en " + ubicacion(tok.linea) + "]: Expresion inesperada cerca de '" + tok.valor + "'");
    }

    NodoAST* parsearSentencia() {
        int lin = actual().linea;
     NodoAST* nodo = parsearSentenciaInterna();
     nodo->linea = lin;
     return nodo;
    }

    NodoAST* parsearSentenciaInterna() {
        const Token& tok = actual();

        if (esTipoDato(tok)) {
            string tipoVar = tokens[pos++].valor;
            string nombreVar = consumirIdentificador("Se esperaba nombre de variable");
            consumirSimbolo("=", "Se esperaba '=' en declaracion de '" + nombreVar + "'");
            if (coincideClave("nuevo")) {
                string claseNom = consumirIdentificador("Se esperaba nombre de estructura despues de 'nuevo'");
                consumirSimbolo("(", "Se esperaba '(' despues de '" + claseNom + "'");
                consumirSimbolo(")", "Se esperaba ')' en instanciacion de '" + claseNom + "'");
                consumirSimbolo(";", "Se esperaba ';' despues de instanciacion");
                return new NodoInstancia(nombreVar, claseNom);
            }
            NodoExpr* expr = parsearExpresion();
            consumirSimbolo(";", "Se esperaba ';' al final de la declaracion de '" + nombreVar + "'");
            return new NodoAsignacion(tipoVar, nombreVar, expr);
        }

        if (tok.tipo == TipoToken::IDENTIFICADOR && siguienteEsSimbolo("=")) {
            string nombreVar = tokens[pos++].valor;
            pos++;
            if (coincideClave("nuevo")) {
                string claseNom = consumirIdentificador("Se esperaba nombre de estructura despues de 'nuevo'");
                consumirSimbolo("(", "Se esperaba '(' despues de '" + claseNom + "'");
                consumirSimbolo(")", "Se esperaba ')' en instanciacion de '" + claseNom + "'");
                consumirSimbolo(";", "Se esperaba ';' despues de instanciacion");
                return new NodoInstancia(nombreVar, claseNom);
            }
            NodoExpr* expr = parsearExpresion();
            consumirSimbolo(";", "Se esperaba ';' al final de la asignacion a '" + nombreVar + "'");
            return new NodoAsignacion("", nombreVar, expr);
        }

        if (coincideClave("entrada")) {
            consumirSimbolo("(", "Se esperaba '(' despues de 'entrada'");
            string varDest = consumirIdentificador("Se esperaba variable en 'entrada'");
            consumirSimbolo(")", "Se esperaba ')' en 'entrada'");
            consumirSimbolo(";", "Se esperaba ';' despues de 'entrada'");
            return new NodoEntrada(varDest);
        }

        if (coincideClave("si")) {
            consumirSimbolo("(", "Se esperaba '(' despues de 'si'");
            NodoExpr* cond = parsearExpresion();
            consumirSimbolo(")", "Se esperaba ')' en la condicion de 'si'");
            NodoSi* nodoSi = new NodoSi(cond);
            nodoSi->bloqueSi = parsearBloque();
            while (coincideClave("sino")) {
                coincideClave("si");
                consumirSimbolo("(", "Se esperaba '(' despues de 'sino'");
                NodoExpr* condSino = parsearExpresion();
                consumirSimbolo(")", "Se esperaba ')' en la condicion de 'sino'");
                vector<NodoAST*> bloqueSino = parsearBloque();
                nodoSi->ramasSino.push_back({condSino, bloqueSino});
            }
            if (coincideClave("demas")) {
                nodoSi->bloqueDemas = parsearBloque();
            }
            return nodoSi;
        }

        if (coincideClave("mientras")) {
            consumirSimbolo("(", "Se esperaba '(' despues de 'mientras'");
            NodoExpr* cond = parsearExpresion();
            consumirSimbolo(")", "Se esperaba ')' en la condicion de 'mientras'");
            NodoMientras* nodoM = new NodoMientras(cond);
            nodoM->bloque = parsearBloque();
            return nodoM;
        }

        if (coincideClave("para")) {
            consumirSimbolo("(", "Se esperaba '(' despues de 'para'");
            NodoAST* init = nullptr;
            if (!esSimbolo(";")) {
                string tipoIni = "";
                if (esTipoDato(actual())) tipoIni = tokens[pos++].valor;
                string nomIni = consumirIdentificador("Se esperaba variable en inicializacion de 'para'");
                consumirSimbolo("=", "Se esperaba '=' en inicializacion de 'para'");
                init = new NodoAsignacion(tipoIni, nomIni, parsearExpresion());
            }
            consumirSimbolo(";", "Se esperaba ';' despues de la inicializacion de 'para'");
            NodoExpr* cond = parsearExpresion();
            consumirSimbolo(";", "Se esperaba ';' despues de la condicion de 'para'");
            NodoAST* act = nullptr;
            if (!esSimbolo(")")) {
                string nomAct = consumirIdentificador("Se esperaba variable en actualizacion de 'para'");
                consumirSimbolo("=", "Se esperaba '=' en actualizacion de 'para'");
                act = new NodoAsignacion("", nomAct, parsearExpresion());
            }
            consumirSimbolo(")", "Se esperaba ')' en cabecera de 'para'");
            NodoPara* nodoP = new NodoPara(init, cond, act);
            nodoP->bloque = parsearBloque();
            return nodoP;
        }

        if (coincideClave("rompe")) {
            consumirSimbolo(";", "Se esperaba ';' despues de 'rompe'");
            return new NodoRompe();
        }

        if (coincideClave("salta")) {
            consumirSimbolo(";", "Se esperaba ';' despues de 'salta'");
            return new NodoSalta();
        }

        if (coincideClave("intenta")) {
            NodoIntenta* nodoInt = new NodoIntenta();
            nodoInt->bloqueIntenta = parsearBloque();
            if (coincideClave("excepto")) {
                nodoInt->bloqueExcepto = parsearBloque();
            }
            return nodoInt;
        }

        if (coincideClave("retorna")) {
            NodoExpr* expr = nullptr;
            if (!esSimbolo(";")) {
                expr = parsearExpresion();
            }
            consumirSimbolo(";", "Se esperaba ';' despues de 'retorna'");
            return new NodoRetorna(expr);
        }

        NodoExpr* expr = parsearExpresion();
        consumirSimbolo(";", "Se esperaba ';' al final de la instruccion");
        return new NodoSentenciaExpr(expr);
    }

    vector<NodoAST*> parsearBloque() {
        consumirSimbolo("{", "Se esperaba '{' para abrir el bloque");
        vector<NodoAST*> instrucciones;
        while (!esFin() && !esSimbolo("}")) {
            instrucciones.push_back(parsearSentencia());
        }
        consumirSimbolo("}", "Se esperaba '}' para cerrar el bloque");
        return instrucciones;
    }

    vector<string> parsearParametros() {
        consumirSimbolo("(", "Se esperaba '(' para lista de parametros");
        vector<string> params;
        while (!esFin() && !esSimbolo(")")) {
            if (esTipoDato(actual())) pos++;
            if (!esFin() && actual().tipo == TipoToken::IDENTIFICADOR) {
                params.push_back(tokens[pos++].valor);
            } else {
                throw runtime_error("[Error Sintaxis en " + ubicacion(actual().linea) + "]: Parametro invalido '" + actual().valor + "'");
            }
            if (!coincideSimbolo(",")) break;
        }
        consumirSimbolo(")", "Se esperaba ')' al cerrar parametros");
        return params;
    }

public:
    Parser(const vector<Token>& t, const vector<pair<string, int>>& m) : tokens(t), mapaLineas(m) {}

    NodoPrograma* parsear() {
        NodoPrograma* programa = new NodoPrograma();
        while (!esFin()) {
            if (coincideClave("contrato")) {
                string nomContrato = consumirIdentificador("Se esperaba nombre de contrato");
                consumirSimbolo("{", "Se esperaba '{' en contrato '" + nomContrato + "'");
                vector<string> firmas;
                while (!esFin() && !esSimbolo("}")) {
                    if (esTipoRetorno(actual())) {
                        pos++;
                        firmas.push_back(consumirIdentificador("Se esperaba nombre de metodo en contrato"));
                        parsearParametros();
                        consumirSimbolo(";", "Se esperaba ';' al final de la firma en contrato");
                    } else {
                        throw runtime_error("[Error Sintaxis en " + ubicacion(actual().linea) + "]: Declaracion invalida en contrato '" + nomContrato + "'");
                    }
                }
                consumirSimbolo("}", "Se esperaba '}' al cerrar contrato '" + nomContrato + "'");
                programa->contratos[nomContrato] = firmas;
            }
            else if (coincideClave("estructura")) {
                int linEst = tokens[pos - 1].linea;
                string nomEstructura = consumirIdentificador("Se esperaba nombre de estructura");
                NodoEstructura* nEst = new NodoEstructura();
                nEst->linea = linEst;
                nEst->nombre = nomEstructura;
                if (coincideClave("implementa")) {
                    nEst->contrato = consumirIdentificador("Se esperaba nombre de contrato despues de 'implementa'");
                }
                consumirSimbolo("{", "Se esperaba '{' en estructura '" + nomEstructura + "'");
                while (!esFin() && !esSimbolo("}")) {
                    if (esTipoRetorno(actual())) {
                        string t = tokens[pos++].valor;
                        string n = consumirIdentificador("Se esperaba nombre de atributo o metodo");
                        if (esSimbolo("(")) {
                            NodoFuncion* met = new NodoFuncion();
                            met->linea = actual().linea;
                            met->tipoRetorno = t;
                            met->nombre = n;
                            met->parametros = parsearParametros();
                            met->bloque = parsearBloque();
                            nEst->metodos[n] = met;
                        } else {
                            consumirSimbolo("=", "Se esperaba '=' para inicializar atributo '" + n + "'");
                            NodoExpr* expr = parsearExpresion();
                            consumirSimbolo(";", "Se esperaba ';' despues de atributo '" + n + "'");
                            nEst->atributos.push_back(new NodoAsignacion(t, n, expr));
                        }
                    } else {
                        throw runtime_error("[Error Sintaxis en " + ubicacion(actual().linea) + "]: Miembro invalido en estructura '" + nomEstructura + "'");
                    }
                }
                consumirSimbolo("}", "Se esperaba '}' al cerrar estructura '" + nomEstructura + "'");
                programa->estructuras[nomEstructura] = nEst;
            }
            else if (esTipoRetorno(actual())) {
                int linFunc = actual().linea;
                NodoFuncion* nuevaFunc = new NodoFuncion();
                nuevaFunc->linea = linFunc;
                nuevaFunc->tipoRetorno = tokens[pos++].valor;
                nuevaFunc->nombre = consumirIdentificador("Se esperaba nombre de funcion");
                nuevaFunc->parametros = parsearParametros();
                nuevaFunc->bloque = parsearBloque();
                programa->funciones[nuevaFunc->nombre] = nuevaFunc;
            }
            else {
                throw runtime_error("[Error Sintaxis en " + ubicacion(actual().linea) + "]: Declaracion global inesperada '" + actual().valor + "'");
            }
        }
        return programa;
    }
};