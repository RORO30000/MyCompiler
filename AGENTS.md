# MyCompiler — AGENTS.md

## Propósito

Compilador e intérprete educativo de un lenguaje de programación con sintaxis en español, acompañado de un IDE gráfico con depuración paso a paso y visualización de memoria RAM. Creado por Jose Chullo y Rodrigo Ramos.

## Arquitectura

Dos capas desacopladas:

1. **Núcleo del compilador** (C++17 puro, sin dependencias externas): preprocesador → lexer → parser (intérprete recursivo descendente, dos pasadas: registro de funciones → ejecución de `Principal`). Soporta entrada por `leer()` via `std::cin` (consola) o `inputHook` (GUI).
2. **Interfaz gráfica** (Qt6/Qt5): editor con gutter, panel de código expandido lateral, terminal emergente con HTML, panel de animación con tarjetas de variables y vista de arreglos, navegación paso a paso con historial de snapshots.

La comunicación entre capas ocurre via:
- **Cola de eventos**: el parser llena `std::vector<EventoPaso>`; la GUI lo reproduce paso a paso.
- **Redirección de `std::cout`**: la GUI captura la salida del parser mediante `stringstream`.
- **inputHook**: el parser usa un callback para `leer()`; la GUI inyecta `QInputDialog`.

## Estructura de carpetas

```
MyCompiler/
├── src/
│   ├── main.cpp                 # Entry point consola (test runner, 42 pruebas)
│   ├── main_gui.cpp             # Entry point GUI Qt
│   ├── core/
│   │   ├── lexer.hpp            # Token types (TipoToken enum, ~60 tokens), Token struct
│   │   ├── lexer.cpp            # Analizador léxico (tokenizar)
│   │   ├── parser.cpp           # Parser recursivo descendente + intérprete (~1420 líneas)
│   │   ├── semantic.hpp         # TablaVariables (scoped), TablaFunciones, Arreglo
│   │   ├── errors.hpp           # Mensajes de error/warning/success en español (~380 líneas)
│   │   ├── eventos.hpp          # TipoEvento enum (~25 tipos), EventoPaso struct
│   │   ├── preprocesador.hpp    # Declaración de preprocesarBibliotecas (2 sobrecargas)
│   │   └── preprocesador.cpp    # #incluir "archivo" → resolución recursiva + mapa líneas (~80 líneas)
│   └── gui/
│       ├── VentanaPrincipal.hpp # VentanaPrincipal, CodeEditor, LineNumberArea (~168 líneas)
│       ├── VentanaPrincipal.cpp # IDE gráfico completo (~1670 líneas)
│       ├── syntax_highlighter.hpp # Coloreado sintáctico header-only sin Q_OBJECT
│       └── theme.hpp            # Tema centralizado: ~50 colores como inline QString (mutable runtime)
├── librerias/                   # Biblioteca estándar para #incluir
│   ├── matematica.txt           # Funciones matemáticas básicas
│   ├── matematica_avanzada.txt  # Funciones numéricas adicionales
│   └── texto.txt                # Funciones de manipulación de cadenas
├── programas/                   # Carpeta de programas del usuario
├── Makefile                     # Build adaptativo (Qt6/Qt5 auto-detect)
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
- **Estado global del parser**: variables estáticas `pos`, `tkns`, `tabla`, `tablaFunciones`, `tipoFuncionActual`

## Sintaxis del lenguaje

### Tipos de datos

| Tipo | Descripción | Ejemplo |
|---|---|---|
| `entero` | Número entero | `entero x = 42;` |
| `decimal` | Número con punto flotante | `decimal pi = 3.14;` |
| `cadena` | Texto entre comillas dobles | `cadena s = "Hola";` |
| `booleano` | `verdadero` o `falso` | `booleano ok = verdadero;` |
| `caracter` | Un carácter entre comillas simples | `caracter c = 'A';` |
| `puntero` | Dirección de memoria (ej. `entero* p`) | `entero* p = &x;` |

### Keywords

`entero`, `decimal`, `cadena`, `booleano`, `caracter`, `vacio`, `funcion`, `retornar`, `arreglo`,
`si`, `sino`, `fin_si`, `entonces`, `hacer`, `mientras`, `fin_mientras`, `para`, `fin_para`,
`elegir`, `caso`, `defecto`, `parar`, `mostrar`, `leer`, `romper`, `continuar`, `no`,
`Principal`, `verdadero`, `falso`.

### Operadores

| Categoría | Operadores |
|---|---|
| Aritméticos | `+`, `-`, `*`, `/`, `%` |
| Comparación | `==`, `!=`, `<`, `>`, `<=`, `>=` |
| Lógicos | `&&`, `\|\|`, `!`, `no` (alias de `!`) |
| Asignación | `=`, `+=`, `-=`, `*=` |
| Incremento/Decremento | `++`, `--` |
| Ternario | `si (cond) entonces v sino f` |

### Tipado estricto

El compilador valida tipos en:
- **Declaraciones**: `entero x = "texto"` → error (cadena no cabe en entero)
- **Asignaciones**: `x = "texto"` → error si `x` es entero
- **Retorno de función**: `entero fn() { retornar "texto" }` → error
- **Parámetros**: `fn("texto")` donde el parámetro espera `entero` → error
- **Arreglos**: asignar un valor de tipo incorrecto a una celda → error

Reglas de coerción implícita:
- `decimal` acepta valores `entero` (promoción)
- `cadena` acepta cualquier tipo (conversión textual)
- `entero`, `booleano`, `caracter` solo aceptan su propio tipo

### Estructuras de control

```
si (cond) { ... } sino si (cond) { ... } sino { ... } fin_si

mientras (cond) { ... } fin_mientras

hacer { ... } mientras (cond) fin_mientras

para (inic; cond; incr) { ... } fin_para

elegir (expr) {
    caso valor1: ... parar;
    caso valor2: ... parar;
    defecto: ...
}
```

- `romper` / `parar` sale del bucle o switch actual
- `continuar` salta a la siguiente iteración del bucle

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

Sintaxis legacy (funcion ... retornar):
```
funcion sumar(entero a, entero b) retornar entero {
    retornar a + b;
}
```

Punto de entrada (`Principal`):
```
Principal() {
    mostrar("Hola Mundo");
}
```

### Expresiones

- **Concatenación de cadenas**: `"Hola" + " Mundo"` → `"Hola Mundo"` (si algún operando no es numérico)
- **Operador ternario**: `entero max = si (a > b) entonces a sino b;`
- **Agrupación**: `(1 + 2) * 3`

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
       ├─ Split horizontal: editor original | panel de funciones de biblioteca
       ├─ botón "Compilar"
       │    └─ manejarEjecucion()
       │         ├─ preprocesarBibliotecas(src, expanded, mapaLineas)
       │         ├─ Si hay #incluir en src:
       │         │   ├─ Extraer funciones de bibliotecas referenciadas (estático)
       │         │   ├─ Construir vista solo-funciones (concatenación)
       │         │   ├─ Construir mapa de líneas expanded→vista-librería
       │         │   └─ Mostrar panel derecho con solo las funciones llamadas
       │         ├─ tokenizar(expanded)
       │         ├─ parsear(tokens, &pasos)   → llena cola de eventos
       │         │   └─ leer() → inputHook → QInputDialog (modal)
       │         ├─ Colapsar eventos consecutivos misma línea (merge)
       │         ├─ mostrarTerminal()          → diálogo emergente con salida
       │         └─ activa botones Siguiente/Anterior
       ├─ navegación paso a paso
       │    ├─ avanzarPaso() → aplicarEvento() + guardarSnapshot()
       │    └─ retrocederPaso() → restaurarSnapshot()
       ├─ resaltarLinea() sincroniza ambos paneles
       │    ├─ Panel izquierdo: mapaLineas[lineaEvento] → línea original
       │    └─ Panel derecho:  mapaLineasLibreria[lineaEvento] → línea en vista solo-funciones
       │         La flecha aparece solo cuando la línea activa está dentro de una
       │         función de biblioteca; caso contrario el panel no muestra ninguna
       │         flecha (solo las funciones leídas quedan visibles).
       └─ panel derecho de animación
            ├─ Tarjetas de variables (QGraphicsProxyWidget animado)
            └─ Vista de arreglo (celdas con índice activo)
```

## Bibliotecas estándar

Las bibliotecas se almacenan en `librerias/` y se incluyen con `#incluir "nombre.txt"`.
El preprocesador busca primero en el directorio actual y luego en `librerias/`.

### `matematica.txt`
```c
entero calcularPotencia(entero base, entero exponente)
entero calcularCuadrado(entero n)
```

### `matematica_avanzada.txt`
```c
entero   absoluto(entero x)
entero   maximo(entero a, entero b)
entero   minimo(entero a, entero b)
decimal  raiz(decimal x)
entero   azar(entero maximo)
entero   factorial(entero n)
entero   potencia(entero base, entero exponente)
booleano esPrimo(entero n)
```

### `texto.txt`
```c
booleano esVacia(cadena s)
cadena   repetir(cadena s, entero n)
```

## Archivos críticos

| Archivo | Líneas | Rol |
|---|---|---|
| `src/core/parser.cpp` | 1417 | Corazón del compilador: gramática, ejecución, tipado estricto, generación de eventos, `leer()` con inputHook |
| `src/gui/VentanaPrincipal.cpp` | 1671 | IDE completo: split editores, terminal emergente, animación, snapshots, navegación |
| `src/gui/syntax_highlighter.hpp` | 161 | Coloreado sintáctico header-only sin Q_OBJECT |
| `src/gui/VentanaPrincipal.hpp` | 168 | VentanaPrincipal (QMainWindow), CodeEditor, LineNumberArea |
| `src/core/semantic.hpp` | 210 | Tabla de símbolos con ámbitos anidados, arreglos, funciones |
| `src/core/lexer.cpp` | 219 | Tokenización: palabras reservadas, operadores, comentarios, cadenas (~29 keywords), escapes `\n`/`\t`/etc. |
| `src/core/lexer.hpp` | 52 | Tipos de token (TipoToken enum, ~60 tokens) y estructura Token |
| `src/core/errors.hpp` | 376 | Catálogo completo de mensajes de error/advertencia en español |
| `src/core/eventos.hpp` | 45 | Contrato de eventos entre parser y GUI |
| `src/main_gui.cpp` | 14 | Entry point GUI Qt |
| `src/core/preprocesador.cpp` | 78 | Resolución recursiva de `#incluir` + generación de mapa línea-expandida→original. Busca en `./` y `librerias/`. |
| `src/core/preprocesador.hpp` | 7 | Declaración de `preprocesarBibliotecas` (2 sobrecargas) |
| `src/main.cpp` | 401 | Test runner con 42 pruebas + helpers |
| `Makefile` | 80 | Build adaptativo con detección automática de Qt |
| `librerias/matematica.txt` | 13 | Biblioteca de ejemplo (calcularPotencia, calcularCuadrado) |
| `librerias/matematica_avanzada.txt` | 114 | Funciones matemáticas adicionales |
| `librerias/texto.txt` | 32 | Funciones de manipulación de cadenas |

Los `#include` usan rutas relativas a `src/` gracias a la bandera `-Isrc` del compilador (ej. `#include "core/lexer.hpp"`).

## Split view inteligente (solo funciones de biblioteca)

Cuando el código fuente contiene `#incluir`, el panel derecho muestra **únicamente las funciones de biblioteca que son invocadas** desde el programa principal, no el contenido completo de las librerías. Esto es un análisis estático: se examina el código expandido, se identifican qué funciones de bibliotecas se llaman, y se extraen solo sus cuerpos.

### Flujo

1. `preprocesarBibliotecas()` resuelve los `#incluir` y produce el código expandido (`expanded`) y el mapa de líneas expandida→original (`mapaLineas`).
2. Se escanea `expanded` para detectar llamadas a funciones que pertenecen a bibliotecas (líneas donde `mapaLineas[k] == 0`).
3. Se extrae el cuerpo de cada función de biblioteca invocada (manejando `{}` anidadas).
4. Se construye una **vista solo-funciones**: concatenación de los cuerpos extraídos, separados por doble salto de línea.
5. Se construye un **mapa de líneas expandida→vista-librería**: para cada línea de `expanded` que está dentro del cuerpo de una función de biblioteca, se calcula su línea correspondiente en la vista solo-funciones.

### Seguimiento de flecha

- Cuando la línea activa del evento (`ev.linea`) corresponde a código de biblioteca (`mapaLineas[ev.linea] == 0`), la flecha se muestra en el **panel derecho** (vista solo-funciones), en la línea correspondiente según el mapa `expanded→vista-librería`.
- Cuando la línea activa corresponde a código del usuario (`mapaLineas[ev.linea] > 0`), la flecha se muestra en el **panel izquierdo** (editor original), y el panel derecho solo muestra las funciones (sin flecha).
- Si no hay función de biblioteca activa en ese momento, el panel derecho no muestra ninguna flecha.

### Consideraciones

- El análisis es **estático**: todas las funciones llamadas en algún punto del programa se muestran desde el inicio.
- Funciones de biblioteca que llaman a otras funciones de biblioteca: ambas se incluyen en la vista.
- Múltiples llamadas a la misma función: la función aparece una sola vez en la vista.
- Si una biblioteca no se encuentra (`#incluir "no_existe.txt"`): se muestra un aviso en la terminal y la línea se agrega como comentario en el código expandido, pero no genera entrada en la vista solo-funciones (no hay función que extraer).

## Historial de features

| Feature | Commit/Añadido |
|---|---|---|
| si/sino/sino si/fin_si, mientras, para, ++/--, +=/-=/*= | Original |
| Tipo `cadena`, `booleano`, `caracter` | Original |
| Funciones con retorno estilo C++ | Original |
| `romper`/`continuar` (break/continue) | 630ff71 |
| `no` como alias de `!` | 62a730b |
| Concatenación de cadenas con `+`, `hacer-mientras` | 3aca8d1 |
| Operador ternario `si ... entonces ... sino` | 3337956 |
| `elegir`/`caso`/`defecto`, `parar` como break | 4c27a3f |
| Tipado estricto (validar tipos en declaración/asignación/retorno/parámetros/arreglos) | 937df6b |
| Fix `formatearNumero` para locale con coma decimal | 937df6b |
| Syntax highlighting en el editor (keywords, strings, números, comentarios, directivas) | 97ec8a1 |
| Fix `parseSi` saltarBloqueLlaves (sino brace-skipping con `solicitudRetorno`) | 97ec8a1 |
| Agrupar eventos por línea (post-procesado colapsa consecutivos misma línea) | Última sesión |
| Play/Pause + slider de velocidad (auto-play) | Última sesión |
| Flecha animada en el gutter con QPropertyAnimation | Última sesión |
| Fix `BUCLE_FIN`: usar línea de `fin_mientras`/`fin_para` en vez de `mientras`/`para` | Última sesión |
| Split view: panel expandido derecho al compilar con `#incluir` | Última sesión |
| Mapa de líneas expandida→original para sincronizar flechas entre paneles | Última sesión |
| Preprocesador: búsqueda automática en `./` y `librerias/` | Última sesión |
| Biblioteca `matematica.txt` migrada a sintaxis actual | Última sesión |
| Nuevas bibliotecas: `matematica_avanzada.txt`, `texto.txt` | Última sesión |
| Tema centralizado: `theme.hpp` con ~50 colores, toggle oscuro/claro, namespace `Theme` con `setDark()`/`setLight()` | Pendiente |
| Barra de menú funcional: Undo/Redo/Copy/Paste/Cut/SelectAll, atajos, toggle tema, aumentar/reducir fuente, Acerca de | Pendiente |
| Eventos nuevos: `ELEGIR_CASO`, `ROMPER`, `CONTINUAR` en parser y GUI | Pendiente |
| Warning por `#incluir` de biblioteca no encontrada (no interrumpe compilación) | Pendiente |
| Panel derecho inteligente: muestra SOLO las funciones de biblioteca invocadas (estático), con mapa de líneas expanded→vista-librería y seguimiento de flecha | Pendiente |
