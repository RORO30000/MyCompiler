# MyCompiler — AGENTS.md

## Propósito

Compilador e intérprete educativo de un lenguaje de programación con sintaxis en español, acompañado de un IDE gráfico con depuración paso a paso y visualización de memoria RAM. Creado por Jose Chullo y Rodrigo Ramos.

## Arquitectura

Dos capas desacopladas:

1. **Núcleo del compilador** (C++17 puro, sin dependencias externas): preprocesador → lexer → parser (intérprete recursivo descendente, dos pasadas: registro de funciones → ejecución de `principal`). Soporta entrada por `leer()` via `std::cin` (consola) o `inputHook` (GUI).
2. **Interfaz gráfica** (Qt6/Qt5): editor con gutter, terminal emergente con HTML, panel de animación con tarjetas de variables y vista de arreglos, navegación paso a paso con historial de snapshots.

La comunicación entre capas ocurre via:
- **Cola de eventos**: el parser llena `std::vector<EventoPaso>`; la GUI lo reproduce paso a paso.
- **Redirección de `std::cout`**: la GUI captura la salida del parser mediante `stringstream`.
- **inputHook**: el parser usa un callback para `leer()`; la GUI inyecta `QInputDialog`.

## Estructura de carpetas

```
MyCompiler/
├── src/
│   ├── main.cpp                 # Entry point consola (test runner, 12 pruebas)
│   ├── main_gui.cpp             # Entry point GUI Qt
│   ├── core/
│   │   ├── lexer.hpp            # Token types (TipoToken enum), Token struct
│   │   ├── lexer.cpp            # Analizador léxico (tokenizar)
│   │   ├── parser.cpp           # Parser recursivo descendente + intérprete
│   │   ├── semantic.hpp         # TablaVariables (scoped), TablaFunciones, Arreglo
│   │   ├── errors.hpp           # Mensajes de error/warning/success en español
│   │   ├── eventos.hpp          # TipoEvento enum, EventoPaso struct
│   │   └── preprocesador.cpp    # #incluir "archivo" → resolución recursiva
│   └── gui/
│       ├── VentanaPrincipal.hpp # VentanaPrincipal, CodeEditor, LineNumberArea
│       └── VentanaPrincipal.cpp # IDE gráfico completo (878 líneas)
├── programas/                   # Carpeta de programas del usuario (generada)
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
- **El lenguaje del compilador usa keywords en español** (`numero`, `vacio`, `funcion`, `si`, `mientras`, `mostrar`, `leer`, `arreglo`, etc.)

## Flujo de ejecución

### Modo consola (`main.cpp`)
```
main()
  └─ compilarYEjecutar(codigo)
       ├─ preprocesarBibliotecas()   → resuelve #incluir
       ├─ tokenizar()                → produce vector<Token>
       └─ parsear(tokens)            → dos pasadas:
            ├─ Fase 1: registra todas las funciones (ejecutar=false)
            └─ Fase 2: ejecuta principal() (o modo secuencial si no existe)
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
| `parser.cpp` | 555 | Corazón del compilador: gramática, ejecución, generación de eventos, `leer()` con inputHook |
| `VentanaPrincipal.cpp` | 940 | IDE completo: UI, terminal emergente, animación, snapshots, navegación |
| `semantic.hpp` | 163 | Tabla de símbolos con ámbitos anidados, arreglos, funciones |
| `lexer.cpp` | 146 | Tokenización: palabras reservadas, operadores, comentarios, cadenas |
| `errors.hpp` | 147 | Catálogo completo de mensajes de error/advertencia en español |
| `eventos.hpp` | 42 | Contrato de eventos entre parser y GUI |
| `src/core/preprocesador.cpp` | 42 | Resolución recursiva de `#incluir` |
| `Makefile` | 75 | Build adaptativo con detección automática de Qt |

Los `#include` usan rutas relativas a `src/` gracias a la bandera `-Isrc` del compilador (ej. `#include "core/lexer.hpp"`).
