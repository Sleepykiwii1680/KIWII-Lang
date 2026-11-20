#pragma once
#include "tipos.hpp"
#include "ast.hpp"

// HERRAMIENTAS MATEMÁTICAS Y LÓGICAS

int ejecutarInstrucciones(const vector<NodoAST*>& instrucciones, map<string, Valor>& memoria, NodoPrograma* programa);

Valor evaluarExpr(NodoExpr* expr, map<string, Valor>& memoria, NodoPrograma* programa, bool forzarNumerico = false);

Valor aplicarTipoAsignacion(const string& tipoVar, const string& nombre, Valor val, map<string, Valor>& memoria) {
    if (tipoVar == "ent" || tipoVar == "dual") {
        if (val.tipo == TipoVal::DEC) return Valor::desdeEnt((long long)val.decVal);
        if (val.tipo == TipoVal::TEXTO && esNumeroStr(val.str)) return Valor::desdeEnt((long long)stod(val.str));
        return val;
    }
    if (tipoVar == "dec") {
        if (val.tipo == TipoVal::ENT) return Valor::desdeDec((double)val.entVal);
        if (val.tipo == TipoVal::TEXTO && esNumeroStr(val.str)) return Valor::desdeDec(stod(val.str));
        return val;
    }
    if (tipoVar == "texto") {
        return Valor::desdeTexto(val.aTexto());
    }
    if (tipoVar.empty() && memoria.count(nombre)) {
        TipoVal prev = memoria[nombre].tipo;
        if (prev == TipoVal::ENT && val.tipo == TipoVal::TEXTO && esNumeroStr(val.str)) {
            return Valor::desdeEnt((long long)stod(val.str));
        }
        if (prev == TipoVal::DEC) {
            if (val.tipo == TipoVal::ENT) return Valor::desdeDec((double)val.entVal);
            if (val.tipo == TipoVal::TEXTO && esNumeroStr(val.str)) return Valor::desdeDec(stod(val.str));
        }
    }
    return val;
}

Valor evaluarBinario(const Valor& v1, const string& op, const Valor& v2, bool forzarNumerico) {
    if (op == "+") {
        if (v1.tipo != TipoVal::TEXTO && v2.tipo != TipoVal::TEXTO) {
            if (v1.tipo == TipoVal::DEC || v2.tipo == TipoVal::DEC) return Valor::desdeDec(v1.aDec() + v2.aDec());
            return Valor::desdeEnt(v1.aEnt() + v2.aEnt());
        }
        if (v1.tipo != TipoVal::TEXTO && v2.tipo == TipoVal::TEXTO && esNumeroStr(v2.str)) {
            if (v1.tipo == TipoVal::DEC || v2.tienePunto()) return Valor::desdeDec(v1.aDec() + v2.aDec());
            return Valor::desdeEnt(v1.aEnt() + v2.aEnt());
        }
        if (forzarNumerico && v1.esNumerico() && v2.esNumerico()) {
            if (v1.tienePunto() || v2.tienePunto()) return Valor::desdeDec(v1.aDec() + v2.aDec());
            return Valor::desdeEnt(v1.aEnt() + v2.aEnt());
        }
        return Valor::desdeTexto(v1.aTexto() + v2.aTexto());
    }

    if (op == "-") {
        if (v1.tienePunto() || v2.tienePunto()) return Valor::desdeDec(v1.aDec() - v2.aDec());
        return Valor::desdeEnt(v1.aEnt() - v2.aEnt());
    }

    if (op == "*") {
        if (v1.esNumerico() && v2.esNumerico()) {
            if (v1.tienePunto() || v2.tienePunto()) return Valor::desdeDec(v1.aDec() * v2.aDec());
            return Valor::desdeEnt(v1.aEnt() * v2.aEnt());
        }
        if (!v1.esNumerico() && v2.esNumerico()) {
            string txt = v1.aTexto();
            long long veces = v2.aEnt();
            if (veces < 0) veces = 0;
            if ((long long)txt.length() * veces > 1000000) {
                throw runtime_error("Limite de memoria en repeticion de texto excedido (Proteccion DoS)");
            }
            string res = "";
            for (long long i = 0; i < veces; i++) res += txt;
            return Valor::desdeTexto(res);
        }
        if (v1.esNumerico() && !v2.esNumerico()) {
            string txt = v2.aTexto();
            long long veces = v1.aEnt();
            if (veces < 0) veces = 0;
            if ((long long)txt.length() * veces > 1000000) {
                throw runtime_error("Limite de memoria en repeticion de texto excedido (Proteccion DoS)");
            }
            string res = "";
            for (long long i = 0; i < veces; i++) res += txt;
            return Valor::desdeTexto(res);
        }
        throw runtime_error("Multiplicacion invalida entre cadenas de texto");
    }

    if (op == "/") {
        if (v1.tienePunto() || v2.tienePunto()) {
            double d2 = v2.aDec();
            if (d2 == 0.0) throw runtime_error("Division por cero");
            return Valor::desdeDec(v1.aDec() / d2);
        }
        long long n2 = v2.aEnt();
        if (n2 == 0) throw runtime_error("Division por cero");
        return Valor::desdeEnt(v1.aEnt() / n2);
    }

    if (op == "%") {
        long long n2 = v2.aEnt();
        if (n2 == 0) throw runtime_error("Modulo por cero");
        return Valor::desdeEnt(v1.aEnt() % n2);
    }

    if (v1.esNumerico() && v2.esNumerico()) {
        double n1 = v1.aDec();
        double n2 = v2.aDec();
        if (op == "==") return Valor::desdeEnt(n1 == n2 ? 1 : 0);
        if (op == "!=") return Valor::desdeEnt(n1 != n2 ? 1 : 0);
        if (op == "<") return Valor::desdeEnt(n1 < n2 ? 1 : 0);
        if (op == ">") return Valor::desdeEnt(n1 > n2 ? 1 : 0);
        if (op == "<=") return Valor::desdeEnt(n1 <= n2 ? 1 : 0);
        if (op == ">=") return Valor::desdeEnt(n1 >= n2 ? 1 : 0);
    } else {
        string s1 = v1.aTexto();
        string s2 = v2.aTexto();
        if (op == "==") return Valor::desdeEnt(s1 == s2 ? 1 : 0);
        if (op == "!=") return Valor::desdeEnt(s1 != s2 ? 1 : 0);
        if (op == "<") return Valor::desdeEnt(s1 < s2 ? 1 : 0);
        if (op == ">") return Valor::desdeEnt(s1 > s2 ? 1 : 0);
        if (op == "<=") return Valor::desdeEnt(s1 <= s2 ? 1 : 0);
        if (op == ">=") return Valor::desdeEnt(s1 >= s2 ? 1 : 0);
    }
    return Valor::desdeEnt(0);
}

// EJECUTOR Y API NATIVA

Valor evaluarExpr(NodoExpr* expr, map<string, Valor>& memoria, NodoPrograma* programa, bool forzarNumerico) {
    if (auto num = dynamic_cast<NodoNumero*>(expr)) {
        return num->valor;
    }
    if (auto cad = dynamic_cast<NodoCadena*>(expr)) {
        return Valor::desdeTexto(cad->valor);
    }
    if (auto var = dynamic_cast<NodoVariable*>(expr)) {
        return memoria.count(var->nombre) ? memoria[var->nombre] : Valor::desdeTexto(var->nombre);
    }
    if (auto un = dynamic_cast<NodoUnario*>(expr)) {
        Valor v = evaluarExpr(un->operando, memoria, programa, false);
        if (un->operador == "-") {
            if (v.tienePunto()) return Valor::desdeDec(-v.aDec());
            return Valor::desdeEnt(-v.aEnt());
        }
        if (un->operador == "!" || un->operador == "no") {
            return Valor::desdeEnt(v.esVerdadero() ? 0 : 1);
        }
        return v;
    }
    if (auto bin = dynamic_cast<NodoBinario*>(expr)) {
        if (bin->operador == "y") {
            Valor v1 = evaluarExpr(bin->izq, memoria, programa, false);
            if (!v1.esVerdadero()) return Valor::desdeEnt(0);
            Valor v2 = evaluarExpr(bin->der, memoria, programa, false);
            return Valor::desdeEnt(v2.esVerdadero() ? 1 : 0);
        }
        if (bin->operador == "o") {
            Valor v1 = evaluarExpr(bin->izq, memoria, programa, false);
            if (v1.esVerdadero()) return Valor::desdeEnt(1);
            Valor v2 = evaluarExpr(bin->der, memoria, programa, false);
            return Valor::desdeEnt(v2.esVerdadero() ? 1 : 0);
        }
        Valor v1 = evaluarExpr(bin->izq, memoria, programa, forzarNumerico);
        Valor v2 = evaluarExpr(bin->der, memoria, programa, forzarNumerico);
        return evaluarBinario(v1, bin->operador, v2, forzarNumerico);
    }
    if (auto llamada = dynamic_cast<NodoLlamadaExpr*>(expr)) {
        auto obtenerVal = [&](size_t idx) -> Valor {
            if (idx >= llamada->argumentos.size()) return Valor::desdeTexto("");
            return evaluarExpr(llamada->argumentos[idx], memoria, programa, false);
        };
        size_t puntoPos = llamada->nombreFuncion.find('.');

        if (llamada->nombreFuncion == "salida") {
            for (size_t i = 0; i < llamada->argumentos.size(); i++) {
                if (i > 0) cout << " ";
                cout << obtenerVal(i).aTexto();
            }
            cout << endl;
            return Valor::desdeTexto("");
        }
        if (llamada->nombreFuncion == "largo") {
            return Valor::desdeEnt((long long)obtenerVal(0).aTexto().length());
        }
        if (llamada->nombreFuncion == "letraen") {
            string txt = obtenerVal(0).aTexto();
            long long idx = obtenerVal(1).aEnt();
            if (idx >= 0 && idx < (long long)txt.length()) return Valor::desdeTexto(string(1, txt[(size_t)idx]));
            return Valor::desdeTexto("");
        }
        if (llamada->nombreFuncion == "codigoascii") {
            string txt = obtenerVal(0).aTexto();
            return Valor::desdeEnt(txt.empty() ? 0 : (long long)(unsigned char)txt[0]);
        }
        if (llamada->nombreFuncion == "aentero") {
            return Valor::desdeEnt(obtenerVal(0).aEnt());
        }
        if (llamada->nombreFuncion == "adecimal") {
            return Valor::desdeDec(obtenerVal(0).aDec());
        }
        if (llamada->nombreFuncion == "atexto") {
            return Valor::desdeTexto(obtenerVal(0).aTexto());
        }
        if (llamada->nombreFuncion == "leerarchivo") {
            if (modoAislado) {
                cerr << "[Sandbox KIWII]: Lectura de archivos bloqueada en modo --aislado" << endl;
                return Valor::desdeTexto("");
            }
            return Valor::desdeTexto(leerArchivo(obtenerVal(0).aTexto()));
        }
        if (puntoPos != string::npos) {
            string objNom = llamada->nombreFuncion.substr(0, puntoPos);
            string metNom = llamada->nombreFuncion.substr(puntoPos + 1);
            string claseKey = objNom + ".__clase__";
            if (memoria.count(claseKey) && programa->estructuras.count(memoria[claseKey].aTexto())) {
                NodoEstructura* estObj = programa->estructuras[memoria[claseKey].aTexto()];
                if (estObj->metodos.count(metNom)) {
                    if (++profundidadPila > LIMITE_PILA) {
                        profundidadPila--;
                        throw runtime_error("Desbordamiento de pila bloqueado (Proteccion DoS)");
                    }
                    NodoFuncion* funcObj = estObj->metodos[metNom];
                    map<string, Valor> memoriaLocal;
                    memoriaLocal["este.__clase__"] = Valor::desdeTexto(estObj->nombre);
                    for (auto attr : estObj->atributos) {
                        memoriaLocal["este." + attr->nombre] = memoria[objNom + "." + attr->nombre];
                    }
                    vector<string> refOriginal, refLocal;
                    for (size_t k = 0; k < funcObj->parametros.size() && k < llamada->argumentos.size(); k++) {
                        string param = funcObj->parametros[k];
                        Valor argVal = evaluarExpr(llamada->argumentos[k], memoria, programa, false);
                        if (param[0] == '&') {
                            string paramReal = param.substr(1);
                            memoriaLocal[paramReal] = argVal;
                            if (auto varArg = dynamic_cast<NodoVariable*>(llamada->argumentos[k])) {
                                refOriginal.push_back(varArg->nombre);
                                refLocal.push_back(paramReal);
                            }
                        } else {
                            memoriaLocal[param] = argVal;
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
                    if (memoriaLocal.count("__retorno__")) return memoriaLocal["__retorno__"];
                }
            }
            return Valor::desdeTexto("");
        }
        if (programa->funciones.count(llamada->nombreFuncion)) {
            if (++profundidadPila > LIMITE_PILA) {
                profundidadPila--;
                throw runtime_error("Desbordamiento de pila bloqueado (Proteccion DoS)");
            }
            NodoFuncion* funcObj = programa->funciones[llamada->nombreFuncion];
            map<string, Valor> memoriaLocal;
            vector<string> refOriginal, refLocal;
            for (size_t k = 0; k < funcObj->parametros.size() && k < llamada->argumentos.size(); k++) {
                string param = funcObj->parametros[k];
                Valor argVal = evaluarExpr(llamada->argumentos[k], memoria, programa, false);
                if (param[0] == '&') {
                    string paramReal = param.substr(1);
                    memoriaLocal[paramReal] = argVal;
                    if (auto varArg = dynamic_cast<NodoVariable*>(llamada->argumentos[k])) {
                        refOriginal.push_back(varArg->nombre);
                        refLocal.push_back(paramReal);
                    }
                } else {
                    memoriaLocal[param] = argVal;
                }
            }
            ejecutarInstrucciones(funcObj->bloque, memoriaLocal, programa);
            profundidadPila--;
            for (size_t k = 0; k < refOriginal.size(); k++) {
                memoria[refOriginal[k]] = memoriaLocal[refLocal[k]];
            }
            if (memoriaLocal.count("__retorno__")) return memoriaLocal["__retorno__"];
        }
        return Valor::desdeTexto("");
    }
    return Valor::desdeTexto("");
}

int ejecutarInstrucciones(const vector<NodoAST*>& instrucciones, map<string, Valor>& memoria, NodoPrograma* programa) {
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
                memoria[instancia->nombreVar + ".__clase__"] = Valor::desdeTexto(instancia->nombreEstructura);
                for (auto attr : estObj->atributos) {
                    bool esNum = (attr->tipoVariable == "ent" || attr->tipoVariable == "dec" || attr->tipoVariable == "dual");
                    Valor v = evaluarExpr(attr->expr, memoria, programa, esNum);
                    memoria[instancia->nombreVar + "." + attr->nombre] = aplicarTipoAsignacion(attr->tipoVariable, attr->nombre, v, memoria);
                }
            } else {
                cerr << "Error KIWII: La estructura '" << instancia->nombreEstructura << "' no existe." << endl;
            }
        }
        else if (auto asignacion = dynamic_cast<NodoAsignacion*>(nodo)) {
            bool forzarNum = (asignacion->tipoVariable == "ent" || asignacion->tipoVariable == "dec" || asignacion->tipoVariable == "dual");
            if (!forzarNum && asignacion->tipoVariable.empty() && memoria.count(asignacion->nombre)) {
                forzarNum = (memoria[asignacion->nombre].tipo != TipoVal::TEXTO);
            }
            Valor v = evaluarExpr(asignacion->expr, memoria, programa, forzarNum);
            memoria[asignacion->nombre] = aplicarTipoAsignacion(asignacion->tipoVariable, asignacion->nombre, v, memoria);
        }
        else if (auto ret = dynamic_cast<NodoRetorna*>(nodo)) {
            if (ret->expr) {
                memoria["__retorno__"] = evaluarExpr(ret->expr, memoria, programa, false);
            } else {
                memoria["__retorno__"] = Valor::desdeTexto("");
            }
            return 3;
        }
        else if (auto sentExpr = dynamic_cast<NodoSentenciaExpr*>(nodo)) {
            evaluarExpr(sentExpr->expr, memoria, programa, false);
        }
        else if (auto entrada = dynamic_cast<NodoEntrada*>(nodo)) {
            string valorIngresado; getline(cin, valorIngresado);
            memoria[entrada->nombreVariable] = Valor::desdeTexto(valorIngresado);
        }
        else if (auto nodoSi = dynamic_cast<NodoSi*>(nodo)) {
            if (evaluarExpr(nodoSi->condicion, memoria, programa, false).esVerdadero()) {
                int est = ejecutarInstrucciones(nodoSi->bloqueSi, memoria, programa);
                if (est != 0) return est;
            } else {
                bool ejecutado = false;
                for (auto& rama : nodoSi->ramasSino) {
                    if (evaluarExpr(rama.first, memoria, programa, false).esVerdadero()) {
                        int est = ejecutarInstrucciones(rama.second, memoria, programa);
                        if (est != 0) return est;
                        ejecutado = true;
                        break;
                    }
                }
                if (!ejecutado && !nodoSi->bloqueDemas.empty()) {
                    int est = ejecutarInstrucciones(nodoSi->bloqueDemas, memoria, programa);
                    if (est != 0) return est;
                }
            }
        }
        else if (auto nodoMientras = dynamic_cast<NodoMientras*>(nodo)) {
            int ciclos = 0;
            while (evaluarExpr(nodoMientras->condicion, memoria, programa, false).esVerdadero()) {
                if (++ciclos > LIMITE_CICLOS) throw runtime_error("Limite de iteraciones excedido (Proteccion DoS)");
                int est = ejecutarInstrucciones(nodoMientras->bloque, memoria, programa);
                if (est == 1) break;
                if (est == 2) continue;
                if (est == 3) return 3;
            }
        }
        else if (auto nodoPara = dynamic_cast<NodoPara*>(nodo)) {
            int ciclos = 0;
            if (nodoPara->inicializacion) {
                vector<NodoAST*> ini = {nodoPara->inicializacion};
                ejecutarInstrucciones(ini, memoria, programa);
            }
            while (evaluarExpr(nodoPara->condicion, memoria, programa, false).esVerdadero()) {
                if (++ciclos > LIMITE_CICLOS) throw runtime_error("Limite de iteraciones excedido (Proteccion DoS)");
                int est = ejecutarInstrucciones(nodoPara->bloque, memoria, programa);
                if (est == 1) break;
                if (est == 3) return 3;
                if (nodoPara->actualizacion) {
                    vector<NodoAST*> act = {nodoPara->actualizacion};
                    ejecutarInstrucciones(act, memoria, programa);
                }
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
        else if (dynamic_cast<NodoRompe*>(nodo)) return 1;
        else if (dynamic_cast<NodoSalta*>(nodo)) return 2;
    }
    return 0;
}

void ejecutar(NodoPrograma* arbol) {
    map<string, Valor> memoriaGlobal;
    if (arbol->funciones.count("principal")) {
        ejecutarInstrucciones(arbol->funciones["principal"]->bloque, memoriaGlobal, arbol);
    } else {
        cerr << "Error: KIWII no encontro la funcion 'principal()'." << endl;
    }
}