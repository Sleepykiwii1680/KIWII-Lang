# KIWII Language (Kernel Interfaced Word Instruction Infrastructure)

**KIWII** es un lenguaje de programación interpretado en español con aislamiento de memoria (*Sandboxing*), diseñado desde cero en C++. Combina la estructura clásica y el manejo de referencias de C++ con la legibilidad y flexibilidad de Python.

## Características del Kernel v1.5
- **Sintaxis hispana intuitiva:** Tipos de datos (`ent`, `texto`, `dual`, `dec`, `vacio`) y control de flujo (`si`, `demas`, `mientras`, `para`, `rompe`, `salta`).
- **Seguridad y Sandboxing (Mitigación CWE):**
  - **Modo Aislado (`--aislado` / CWE-250):** Ejecución de scripts no confiables con bloqueo total de lectura en disco.
  - **Filtro Anti Path Traversal (CWE-22):** Bloqueo nativo de rutas relativas o absolutas (`../`, `C:/`, `/etc/`) fuera del directorio de trabajo.
  - **Protección contra Denegación de Servicio (CWE-835 y CWE-674):** Límite automático de iteraciones en ciclos y tope de profundidad en pila recursiva.
- **Gestión de Memoria y Referencias:** Ámbitos de memoria local por función y paso por referencia utilizando `&`.
- **Estructuras de Datos y Excepciones:** Definición de moldes con `estructura`, instanciación con `nuevo` y bloques seguros `intenta` / `excepto`.
- **Librería Estándar (`estandar.kiw`):** Módulos nativos para manipulación de textos (`cadenas.kiw`), aritmética (`matematica.kiw`), colecciones dinámicas (`listas.kiw`), diccionarios clave-valor (`mapas.kiw`) y renderizado de tablas (`formato.kiw`).
- **Bootstrapping y Máquina Virtual (`motor.kiw`):** Analizador léxico (`lexer.kiw`) e intérprete (`evaluador.kiw`) escritos 100% en KIWII capaces de tokenizar y ejecutar otros archivos `.kiw` en memoria virtual aislada.

## Demostración y Uso

El código de prueba principal del lenguaje se encuentra en **`demokiwii.kiw`** (17 secciones que validan tipos, recursión, referencias, estructuras, librerías, máquina virtual y blindajes de seguridad).

```powershell
# Ejecución estándar de la suite completa
kiwii demokiwii.kiw

# Ejecución con métricas del compilador (archivos fusionados, tokens y memoria)
kiwii demokiwii.kiw --debug

# Ejecución en entorno Sandbox (bloquea accesos al sistema de archivos)
ki