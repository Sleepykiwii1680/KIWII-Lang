#pragma once
#include "tipos.hpp"

// ARVOL PRINCIPAL

class NodoExpr {
public:
    virtual ~NodoExpr() = default;
};

class NodoNumero : public NodoExpr {
public:
    Valor valor;
    NodoNumero(const string& raw) {
        if (raw.find('.') != string::npos) valor = Valor::desdeDec(stod(raw));
        else valor = Valor::desdeEnt(stoll(raw));
    }
    NodoNumero(Valor v) : valor(v) {}
};

class NodoCadena : public NodoExpr {
public:
    string valor;
    NodoCadena(const string& v) : valor(v) {}
};

class NodoVariable : public NodoExpr {
public:
    string nombre;
    NodoVariable(const string& n) : nombre(n) {}
};

class NodoUnario : public NodoExpr {
public:
    string operador;
    NodoExpr* operando;
    NodoUnario(const string& op, NodoExpr* expr) : operador(op), operando(expr) {}
    ~NodoUnario() { delete operando; }
};

class NodoBinario : public NodoExpr {
public:
    NodoExpr* izq;
    string operador;
    NodoExpr* der;
    NodoBinario(NodoExpr* i, const string& op, NodoExpr* d) : izq(i), operador(op), der(d) {}
    ~NodoBinario() { delete izq; delete der; }
};

class NodoLlamadaExpr : public NodoExpr {
public:
    string nombreFuncion;
    vector<NodoExpr*> argumentos;
    NodoLlamadaExpr(const string& n, vector<NodoExpr*> args) : nombreFuncion(n), argumentos(args) {}
    ~NodoLlamadaExpr() { for (auto a : argumentos) delete a; }
};

class NodoAST {
public:
    int linea = 1;
    virtual ~NodoAST() = default;
};

class NodoSentenciaExpr : public NodoAST {
public:
    NodoExpr* expr;
    NodoSentenciaExpr(NodoExpr* e) : expr(e) {}
    ~NodoSentenciaExpr() { delete expr; }
};

class NodoAsignacion : public NodoAST {
public:
    string tipoVariable, nombre;
    NodoExpr* expr;
    NodoAsignacion(string t, string n, NodoExpr* e) : tipoVariable(t), nombre(n), expr(e) {}
    ~NodoAsignacion() { delete expr; }
};

class NodoRetorna : public NodoAST {
public:
    NodoExpr* expr;
    NodoRetorna(NodoExpr* e) : expr(e) {}
    ~NodoRetorna() { delete expr; }
};

class NodoEntrada : public NodoAST {
public:
    string nombreVariable;
    NodoEntrada(string n) : nombreVariable(n) {}
};

class NodoSi : public NodoAST {
public:
    NodoExpr* condicion;
    vector<NodoAST*> bloqueSi;
    vector<pair<NodoExpr*, vector<NodoAST*>>> ramasSino;
    vector<NodoAST*> bloqueDemas;
    NodoSi(NodoExpr* c) : condicion(c) {}
    ~NodoSi() {
        delete condicion;
        for (auto n : bloqueSi) delete n;
        for (auto& r : ramasSino) {
            delete r.first;
            for (auto n : r.second) delete n;
        }
        for (auto n : bloqueDemas) delete n;
    }
};

class NodoRompe : public NodoAST {};
class NodoSalta : public NodoAST {};

class NodoMientras : public NodoAST {
public:
    NodoExpr* condicion;
    vector<NodoAST*> bloque;
    NodoMientras(NodoExpr* c) : condicion(c) {}
    ~NodoMientras() { delete condicion; for (auto n : bloque) delete n; }
};

class NodoPara : public NodoAST {
public:
    NodoAST* inicializacion;
    NodoExpr* condicion;
    NodoAST* actualizacion;
    vector<NodoAST*> bloque;
    NodoPara(NodoAST* ini, NodoExpr* cond, NodoAST* act)
        : inicializacion(ini), condicion(cond), actualizacion(act) {}
    ~NodoPara() {
        delete inicializacion;
        delete condicion;
        delete actualizacion;
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

class NodoFuncion {
public:
    int linea = 1;
    string tipoRetorno, nombre;
    vector<string> parametros;
    vector<NodoAST*> bloque;
};

class NodoEstructura {
public:
    int linea = 1;
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