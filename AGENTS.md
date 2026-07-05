# MyCompiler — AGENTS.md

## Propósito

Compilador e intérprete educativo de un lenguaje de programación con sintaxis en español, acompañado de un IDE gráfico con depuración paso a paso y visualización de memoria RAM. Creado por Jose Chullo y Rodrigo Ramos.

## Arquitectura

Dos capas desacopladas:

1. **Núcleo del compilador** (C++17 puro, sin dependencias externas): preprocesador → lexer → parser (intérprete recursivo descendente, dos pasadas: registro de funciones → ejecución de `Principal`). Soporta entrada por `leer()` via `std::cin` (consola) o `inputHook` (GUI).
2. **Interfaz gráfica** (Qt6/Qt5): editor con gutter, terminal emergente con HTML, panel de animación con tarjetas de variables y vista de arreglos, navegación paso a paso con historial de snapshots.

La comunicación entre capas ocurre via:
- **Cola de eventos**: el parser llena `std::vector<EventoPaso>`; la GUI lo reproduce paso a paso.
- **Redirección de `std::cout`**: la GUI captura la salida del parser mediante `stringstream`.
- **inputHook**: el parser usa un callback para `leer()`; la GUI inyecta `QInputDialog`.

## Estructura de carpetas

```
MyCompiler/
├── src/
│   ├── main.cpp                 # Entry point consola (test runner, 10 pruebas)
│   ├── main_gui.cpp             # Entry point GUI Qt
│   ├── core/
│   │   ├── lexer.hpp            # Token types (TipoToken enum, ~40 tokens), Token struct
│   │   ├── lexer.cpp            # Analizador léxico (tokenizar)
│   │   ├── parser.cpp           # Parser recursivo descendente + intérprete (~966 líneas)
│   │   ├── semantic.hpp         # TablaVariables (scoped), TablaFunciones, Arreglo
│   │   ├── errors.hpp           # Mensajes de error/warning/success en español
│   │   ├── eventos.hpp          # TipoEvento enum, EventoPaso struct
│   │   ├── preprocesador.hpp    # Declaración de preprocesarBibliotecas
│   │   └── preprocesador.cpp    # #incluir "archivo" → resolución recursiva
│   └── gui/
│       ├── VentanaPrincipal.hpp # VentanaPrincipal, CodeEditor, LineNumberArea
│       └── VentanaPrincipal.cpp # IDE gráfico completo (~1190 líneas)
├── programas/                   # Carpeta de programas del usuario
├── Makefile                     # Build adaptativo (Qt6/Qt5 auto-detect)
├── matematica.txt               # Biblioteca de ejemplo para #incluir
├── README.md                    # Documentación en español
├── README_MyCompiler.txt        # Guía rápida
├── LICENSE                      # MIT
├── AGENTS.md
└── .gitignore
```

## Dependencias

- **Compilador**: g++ con -std=c++17
- **Librerías**: Qt6Widgets o Qt5Widgets (detectado automáticamente por pkg-config)
- **Build**: Make + pkg-config + MOC (Meta-Object Compiler)
- **Sin dependencias externas para el núcleo del compilador** (solo STL)

## Convenciones

- **Lenguaje**: C++17
- **Estilo de código**: indentación 4 espacios, llaves estilo K&R, `PascalCase` para clases, `camelCase` para métodos, `snake_case` para funciones libres
- **Comentarios**: bloques con líneas `// ─── ... ───` para separar secciones
- **Headers**: `#pragma once`, sin namespaces (todo global)
- **Errores**: se lanzan como `std::runtime_error` con mensajes desde `errors.hpp`
- **Estado global del parser**: variables estáticas `pos`, `tkns`, `tabla`, `tablaFunciones`

## Sintaxis del lenguaje

### Tipos de datos

| Tipo | Descripción | Ejemplo |
|---|---|---|
| `entero` | Número entero | `entero x = 42;` |
| `decimal` | Número con punto flotante | `decimal pi = 3.14;` |
| `cadena` | Texto entre comillas dobles | `cadena s = "Hola";` |
| `booleano` | `verdadero` o `falso` | `booleano ok = verdadero;` |
| `caracter` | Un carácter entre comillas simples | `caracter c = 'A';` |

### Keywords

`entero`, `decimal`, `cadena`, `booleano`, `caracter`, `vacio`, `si`, `sino`, `fin_si`, `mientras`, `fin_mientras`, `para`, `fin_para`, `mostrar`, `leer`, `retornar`, `arreglo`, `Principal`, `verdadero`, `falso`.

### Operadores

| Categoría | Operadores |
|---|---|
| Aritméticos | `+`, `-`, `*`, `/`, `%` |
| Comparación | `==`, `!=`, `<`, `>`, `<=`, `>=` |
| Lógicos | `&&`, `\|\|`, `!` |
| Asignación | `=`, `+=`, `-=`, `*=` |
| Incremento/Decremento | `++`, `--` |

### Estructuras de control

```
si (cond) { ... } sino si (cond) { ... } sino { ... } fin_si

mientras (cond) { ... } fin_mientras

para (inic; cond; incr) { ... } fin_para
```

### Funciones

Con tipo de retorno (C++ style):
```
entero sumar(entero a, entero b) {
    retornar a + b;
}
```

Sin retorno (`vacio`):
```
vacio saludar() {
    mostrar("Hola");
}
```

Punto de entrada (`Principal`):
```
Principal() {
    mostrar("Hola Mundo");
}
```

## Flujo de ejecución

### Modo consola (`main.cpp`)
```
main()
  └─ compilarYEjecutar(codigo)
       ├─ preprocesarBibliotecas()   → resuelve #incluir
       ├─ tokenizar()                → produce vector<Token>
       └─ parsear(tokens)            → dos pasadas:
            ├─ Fase 1: registra funciones + ejecuta declaraciones globales (ejecutar=true)
            └─ Fase 2: ejecuta Principal() (o modo secuencial si no existe)
```

### Modo GUI (`main_gui.cpp`)
```
main()
  └─ VentanaPrincipal (QMainWindow)
       ├─ Editor de código (CodeEditor con LineNumberArea)
       ├─ botón "Compilar"
       │    └─ manejarEjecucion()
       │         ├─ preprocesarBibliotecas()
       │         ├─ tokenizar()
       │         ├─ parsear(tokens, &pasos)   → llena cola de eventos
       │         │   └─ leer() → inputHook → QInputDialog (modal)
       │         ├─ mostrarTerminal()          → diálogo emergente con salida
       │         └─ activa botones Siguiente/Anterior
       ├─ navegación paso a paso
       │    ├─ avanzarPaso() → aplicarEvento() + guardarSnapshot()
       │    └─ retrocederPaso() → restaurarSnapshot()
       └─ panel derecho
            ├─ Tarjetas de variables (QGraphicsProxyWidget animado)
            └─ Vista de arreglo (celdas con índice activo)
```

## Archivos críticos

| Archivo | Líneas | Rol |
|---|---|---|
| `src/core/parser.cpp` | 966 | Corazón del compilador: gramática, ejecución, generación de eventos, `leer()` con inputHook |
| `src/gui/VentanaPrincipal.cpp` | 1190 | IDE completo: UI, terminal emergente, animación, snapshots, navegación |
| `src/core/semantic.hpp` | 163 | Tabla de símbolos con ámbitos anidados, arreglos, funciones |
| `src/core/lexer.cpp` | 178 | Tokenización: palabras reservadas, operadores, comentarios, cadenas |
| `src/core/lexer.hpp` | 41 | Tipos de token (TipoToken) y estructura Token |
| `src/core/errors.hpp` | 147 | Catálogo completo de mensajes de error/advertencia en español |
| `src/core/eventos.hpp` | 43 | Contrato de eventos entre parser y GUI |
| `src/core/preprocesador.cpp` | 42 | Resolución recursiva de `#incluir` |
| `src/core/preprocesador.hpp` | 4 | Declaración de `preprocesarBibliotecas` |
| `src/main.cpp` | 150 | Test runner con 10 pruebas de la gramática |
| `Makefile` | 80 | Build adaptativo con detección automática de Qt |

Los `#include` usan rutas relativas a `src/` gracias a la bandera `-Isrc` del compilador (ej. `#include "core/lexer.hpp"`).
