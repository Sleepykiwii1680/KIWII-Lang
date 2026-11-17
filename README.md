# KIWII Language (Kernel Interfaced Word Instruction Infrastructure)

**KIWII** es un lenguaje de programación interpretado en español, diseñado desde cero en C++. Combina la estructura clásica y manejo de referencias de C++ con la legibilidad y flexibilidad de Python.

## Características del Kernel v1.0
* **Sintaxis hispana intuitiva:** Tipos de datos (`ent`, `texto`, `dual`, `dec`, `vacio`) y control de flujo (`si`, `demas`, `mientras`, `para`, `rompe`, `salta`).
* **Modularidad y Preprocesador:** Soporte nativo para importar librerías escritas en KIWII mediante `#incluir "archivo.kiw"`.
* **Gestión de Memoria y Referencias:** Ámbitos de memoria local por función y paso por referencia utilizando `&`.
* **Estructuras de Datos (POO base):** Definición de moldes con `estructura` e instanciación dinámica mediante `nuevo`.
* **Manejo de Excepciones:** Bloques seguros `intenta` / `excepto`.
* **Operaciones de cadenas estilo Python:** Multiplicación y repetición directa de textos (`"ja" * 5`).

## Ejemplo de Código (`prueba.kiw`)

```cpp
#incluir <consola>

estructura Jugador {
    texto nombre = "Heroe";
    ent vida = 100;
}

vacio curar(&salud, puntos) {
    salud = salud + puntos;
}

ent principal() {
    ent j1 = nuevo Jugador();
    j1.nombre = "Kiwi";
    
    curar(j1.vida, 50);
    salida("Salud actual de " + j1.nombre);
    salida(j1.vida);
    retorna 0;
}