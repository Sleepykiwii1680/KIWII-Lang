#pragma once
#include "tipos.hpp"
#include "ast.hpp"

struct Diagnostico {
    string nivel;
    string archivo;
    int linea;
    string mensaje;
};

struct InfoVar {
    int lineaDecl = 1;
    bool usada = false;
    bool esParametro = false;
    string claseInstancia = "";
};

class Analizador {
    NodoPrograma* programa;
    const vector<pair<string, int>>& mapaLineas;
    vector<Diagnostico> diagnosticos;
    set<string> nativas = {
        "salida", "largo", "letraen", "codigoascii",
        "aentero", "adecimal", "atexto", "leerarchivo"
    };
    map<string, int> aridadNativas = {
        {"largo", 1}, {"letraen", 2}, {"codigoascii", 1},
        {"aentero", 1}, {"adecimal", 1}, {"atexto", 1}, {"leerarchivo", 1}
    };

    void agregar(const string& nivel, int linGlobal, const string& msg) {
        string arch = "";
        int linReal = linGlobal;
        if (linGlobal >= 1 && (size_t)linGlobal <= mapaLineas.size()) {
            arch = mapaLineas[(size_t)linGlobal - 1].first;
            linReal = mapaLineas[(size_t)linGlobal - 1].second;
        }
        diagnosticos.push_back({nivel, arch, linReal, msg});
    }

    bool estructuraTieneAtributo(NodoEstructura* est, const string& attr) {
        for (auto a : est->atributos) {
            if (a->nombre == attr) return true;
        }
        return false;
    }

    void verificarAccesoVariable(const string& nom, int lin, map<string, InfoVar>& vars, bool esEscritura) {
        if (vars.count(nom)) {
            if (!esEscritura) vars[nom].usada = true;
            return;
        }
        size_t punto = nom.find('.');
        if (punto != string::npos) {
            string obj = nom.substr(0, punto);
            string attr = nom.substr(punto + 1);
            if (vars.count(obj)) {
                vars[obj].usada = true;
                string clase = vars[obj].claseInstancia;
                if (!clase.empty() && programa->estructuras.count(clase)) {
                    NodoEstructura* est = programa->estructuras[clase];
                    if (!estructuraTieneAtributo(est, attr)) {
                        agregar("ERROR", lin, "El atributo '" + attr + "' no existe en la estructura '" + clase + "'");
                    }
                }
                return;
            }
        }
        agregar("ERROR", lin, (esEscritura ? "Asignacion a variable no declarada '" : "La variable '") + nom + (esEscritura ? "'" : "' no esta declarada"));
    }

    void analizarExpr(NodoExpr* expr, int lin, map<string, InfoVar>& vars) {
        if (!expr) return;
        if (auto var = dynamic_cast<NodoVariable*>(expr)) {
            verificarAccesoVariable(var->nombre, lin, vars, false);
        }
        else if (auto un = dynamic_cast<NodoUnario*>(expr)) {
            analizarExpr(un->operando, lin, vars);
        }
        else if (auto bin = dynamic_cast<NodoBinario*>(expr)) {
            analizarExpr(bin->izq, lin, vars);
            analizarExpr(bin->der, lin, vars);
            if (bin->operador == "/" || bin->operador == "%") {
                if (auto numDer = dynamic_cast<NodoNumero*>(bin->der)) {
                    bool esCero = (numDer->valor.tipo == TipoVal::ENT && numDer->valor.entVal == 0) ||
                                  (numDer->valor.tipo == TipoVal::DEC && numDer->valor.decVal == 0.0);
                    if (esCero) {
                        agregar("AVISO", lin, "Posible division o modulo por cero (CWE-369)");
                    }
                }
            }
        }
        else if (auto llamada = dynamic_cast<NodoLlamadaExpr*>(expr)) {
            for (auto arg : llamada->argumentos) {
                analizarExpr(arg, lin, vars);
            }
            string nom = llamada->nombreFuncion;

            if (nom == "leerarchivo" && !llamada->argumentos.empty()) {
                if (auto cad = dynamic_cast<NodoCadena*>(llamada->argumentos[0])) {
                    if (!esRutaSegura(cad->valor)) {
                        agregar("AVISO", lin, "Ruta peligrosa en leerarchivo('" + cad->valor + "'): posible Path Traversal (CWE-22)");
                    }
                }
            }

            if (nativas.count(nom)) {
                if (nom == "salida") {
                    if (llamada->argumentos.empty()) {
                        agregar("AVISO", lin, "'salida()' llamada sin argumentos");
                    }
                } else if (aridadNativas.count(nom) && (int)llamada->argumentos.size() != aridadNativas[nom]) {
                    agregar("ERROR", lin, "La funcion nativa '" + nom + "' espera " + to_string(aridadNativas[nom]) + " argumento(s), pero recibio " + to_string(llamada->argumentos.size()));
                }
                return;
            }

            size_t punto = nom.find('.');
            if (punto != string::npos) {
                string obj = nom.substr(0, punto);
                string met = nom.substr(punto + 1);
                if (!vars.count(obj)) {
                    agregar("ERROR", lin, "El objeto '" + obj + "' no esta declarado");
                    return;
                }
                vars[obj].usada = true;
                string clase = vars[obj].claseInstancia;
                if (!clase.empty() && programa->estructuras.count(clase)) {
                    NodoEstructura* est = programa->estructuras[clase];
                    if (!est->metodos.count(met)) {
                        agregar("ERROR", lin, "El metodo '" + met + "' no existe en la estructura '" + clase + "'");
                    } else {
                        size_t esp = est->metodos[met]->parametros.size();
                        if (llamada->argumentos.size() != esp) {
                            agregar("ERROR", lin, "El metodo '" + clase + "." + met + "' espera " + to_string(esp) + " argumento(s), pero recibio " + to_string(llamada->argumentos.size()));
                        }
                    }
                }
                return;
            }

            if (programa->funciones.count(nom)) {
                size_t esp = programa->funciones[nom]->parametros.size();
                if (llamada->argumentos.size() != esp) {
                    agregar("ERROR", lin, "La funcion '" + nom + "' espera " + to_string(esp) + " argumento(s), pero recibio " + to_string(llamada->argumentos.size()));
                }
            } else {
                agregar("ERROR", lin, "La funcion '" + nom + "' no esta definida");
            }
        }
    }

    void analizarBloque(const vector<NodoAST*>& bloque, map<string, InfoVar>& vars, int& profCiclo, bool& tieneRet) {
        bool terminado = false;
        bool avisoMuerto = false;

        for (auto nodo : bloque) {
            if (terminado && !avisoMuerto) {
                agregar("AVISO", nodo->linea, "Codigo inalcanzable despues de retorna/rompe/salta");
                avisoMuerto = true;
            }

            if (auto inst = dynamic_cast<NodoInstancia*>(nodo)) {
                if (!programa->estructuras.count(inst->nombreEstructura)) {
                    agregar("ERROR", nodo->linea, "La estructura '" + inst->nombreEstructura + "' no existe");
                }
                bool yaUsada = vars.count(inst->nombreVar) ? vars[inst->nombreVar].usada : false;
                vars[inst->nombreVar] = {nodo->linea, yaUsada, false, inst->nombreEstructura};
            }
            else if (auto asig = dynamic_cast<NodoAsignacion*>(nodo)) {
                analizarExpr(asig->expr, nodo->linea, vars);
                if (!asig->tipoVariable.empty()) {
                    vars[asig->nombre] = {nodo->linea, false, false, ""};
                } else {
                    verificarAccesoVariable(asig->nombre, nodo->linea, vars, true);
                }
            }
            else if (auto ent = dynamic_cast<NodoEntrada*>(nodo)) {
                if (!vars.count(ent->nombreVariable)) {
                    agregar("ERROR", nodo->linea, "La variable '" + ent->nombreVariable + "' en entrada() no esta declarada");
                } else {
                    vars[ent->nombreVariable].usada = true;
                }
            }
            else if (auto sent = dynamic_cast<NodoSentenciaExpr*>(nodo)) {
                analizarExpr(sent->expr, nodo->linea, vars);
            }
            else if (auto ret = dynamic_cast<NodoRetorna*>(nodo)) {
                analizarExpr(ret->expr, nodo->linea, vars);
                tieneRet = true;
                terminado = true;
            }
            else if (dynamic_cast<NodoRompe*>(nodo)) {
                if (profCiclo == 0) agregar("ERROR", nodo->linea, "'rompe' solo puede usarse dentro de un ciclo");
                terminado = true;
            }
            else if (dynamic_cast<NodoSalta*>(nodo)) {
                if (profCiclo == 0) agregar("ERROR", nodo->linea, "'salta' solo puede usarse dentro de un ciclo");
                terminado = true;
            }
            else if (auto nSi = dynamic_cast<NodoSi*>(nodo)) {
                analizarExpr(nSi->condicion, nodo->linea, vars);
                analizarBloque(nSi->bloqueSi, vars, profCiclo, tieneRet);
                for (auto& rama : nSi->ramasSino) {
                    analizarExpr(rama.first, nodo->linea, vars);
                    analizarBloque(rama.second, vars, profCiclo, tieneRet);
                }
                if (!nSi->bloqueDemas.empty()) {
                    analizarBloque(nSi->bloqueDemas, vars, profCiclo, tieneRet);
                }
            }
            else if (auto nM = dynamic_cast<NodoMientras*>(nodo)) {
                analizarExpr(nM->condicion, nodo->linea, vars);
                profCiclo++;
                analizarBloque(nM->bloque, vars, profCiclo, tieneRet);
                profCiclo--;
            }
            else if (auto nP = dynamic_cast<NodoPara*>(nodo)) {
                if (auto ini = dynamic_cast<NodoAsignacion*>(nP->inicializacion)) {
                    analizarExpr(ini->expr, nodo->linea, vars);
                    if (!ini->tipoVariable.empty()) {
                        vars[ini->nombre] = {nodo->linea, false, false, ""};
                    } else {
                        verificarAccesoVariable(ini->nombre, nodo->linea, vars, true);
                    }
                }
                analizarExpr(nP->condicion, nodo->linea, vars);
                if (auto act = dynamic_cast<NodoAsignacion*>(nP->actualizacion)) {
                    analizarExpr(act->expr, nodo->linea, vars);
                    verificarAccesoVariable(act->nombre, nodo->linea, vars, true);
                }
                profCiclo++;
                analizarBloque(nP->bloque, vars, profCiclo, tieneRet);
                profCiclo--;
            }
            else if (auto nInt = dynamic_cast<NodoIntenta*>(nodo)) {
                analizarBloque(nInt->bloqueIntenta, vars, profCiclo, tieneRet);
                analizarBloque(nInt->bloqueExcepto, vars, profCiclo, tieneRet);
            }
        }
    }

    void analizarFuncion(NodoFuncion* func, const string& claseActual = "") {
        map<string, InfoVar> vars;
        if (!claseActual.empty()) {
            vars["este"] = {func->linea, true, true, claseActual};
        }
        for (const string& p : func->parametros) {
            string pReal = (!p.empty() && p[0] == '&') ? p.substr(1) : p;
            vars[pReal] = {func->linea, false, true, ""};
        }
        int profCiclo = 0;
        bool tieneRet = false;
        analizarBloque(func->bloque, vars, profCiclo, tieneRet);

        if (func->tipoRetorno != "vacio" && !tieneRet) {
            string prefijo = claseActual.empty() ? "La funcion '" : "El metodo '" + claseActual + ".";
            agregar("AVISO", func->linea, prefijo + func->nombre + "' declara retorno '" + func->tipoRetorno + "' pero no contiene 'retorna'");
        }
        for (auto const& [nom, info] : vars) {
            if (!info.esParametro && !info.usada) {
                agregar("AVISO", info.lineaDecl, "Variable '" + nom + "' declarada pero nunca utilizada");
            }
        }
    }

public:
    Analizador(NodoPrograma* p, const vector<pair<string, int>>& m) : programa(p), mapaLineas(m) {}

    vector<Diagnostico> analizar() {
        for (auto const& [nom, est] : programa->estructuras) {
            if (!est->contrato.empty()) {
                if (!programa->contratos.count(est->contrato)) {
                    agregar("ERROR", est->linea, "El contrato '" + est->contrato + "' no existe");
                } else {
                    for (const string& req : programa->contratos[est->contrato]) {
                        if (!est->metodos.count(req)) {
                            agregar("ERROR", est->linea, "La estructura '" + est->nombre + "' incumple el contrato '" + est->contrato + "': falta '" + req + "'");
                        }
                    }
                }
            }
            map<string, InfoVar> vacio;
            for (auto attr : est->atributos) {
                analizarExpr(attr->expr, est->linea, vacio);
            }
            for (auto const& [nomMet, met] : est->metodos) {
                analizarFuncion(met, est->nombre);
            }
        }

        for (auto const& [nom, func] : programa->funciones) {
            analizarFuncion(func);
        }

        if (!programa->funciones.count("principal")) {
            agregar("AVISO", 1, "No se encontro la funcion 'principal()' (ignorar si es una libreria)");
        }
        return diagnosticos;
    }
};