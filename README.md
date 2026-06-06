# MyCompiler

Somos un grupo de desarrollo enfocado en la creación de soluciones tecnológicas y proyectos de software utilizando C++. Nuestro objetivo es aprender, innovar y construir herramientas eficientes que combinen lógica, rendimiento y buenas prácticas de programación.

Este proyecto nace como una iniciativa colaborativa orientada al desarrollo de compiladores y sistemas relacionados con el análisis de lenguajes, aplicando conceptos de estructuras de datos, algoritmos y diseño de software.

Buscamos trabajar con una visión profesional, organizando el proyecto como un entorno real de desarrollo, fomentando el trabajo en equipo, el uso de GitHub y la colaboración continua.

---

## Estado y Estructura Actual del Sistema

El proyecto cuenta con un motor analítico en C++ y un entorno de desarrollo interactivo (IDE) de escritorio. El sistema se encuentra desacoplado en dos capas principales: el núcleo del compilador y la interfaz gráfica de usuario basada en el framework Qt.

### 1. Núcleo del Compilador
* **Analizador Léxico (lexer.cpp):** Transforma el código fuente en un flujo de tokens estructurados.
* **Analizador Sintáctico y Semántico (parser.cpp):** Aplica las reglas gramaticales, gestiona las palabras reservadas en español (como numero, vacio, mientras, si, sino) y ejecuta las operaciones lógicas.
* **Tabla de Símbolos (semantic.hpp):** Administra los ámbitos de memoria (scopes), el almacenamiento de variables y el registro de funciones o subrutinas globales.
* **Sistema de Inclusión (Preprocesador):** Permite la modularidad del código mediante la importación de bibliotecas externas con la directiva #incluir.
* **Gestión de Errores (errors.hpp):** Sistema centralizado que captura fallos mediante excepciones estructurales.

### 2. Entorno Gráfico e Interfaz de Usuario
La interfaz organiza el espacio de trabajo de manera elástica utilizando un contenedor divisor (QSplitter) que permite redimensionar los paneles con el ratón:
* **Editor de Código (QTextEdit):** Espacio principal con tipografía monoespaciada configurada para la escritura de código fuente.
* **Consola de Salida (QTextEdit):** Terminal embebida de solo lectura potenciada con renderizado HTML/CSS para destacar salidas en verde y errores en rojo.
* **Sistema de Plantillas (QTabWidget):** Estructura por pestañas en el panel derecho que segmenta las herramientas de visualización. Actualmente cuenta con un visualizador de variables (Memoria RAM) y un panel preparado para la gestión de arreglos.

---

## Conectividad y Animación Gráfica

La comunicación entre el código escrito por el usuario y las funciones en C++ se procesa mediante dos mecanismos internos:

### Sistema de Eventos
La interfaz utiliza el patrón Signals and Slots de Qt. El preprocesador MOC (Meta-Object Compiler) analiza la macro Q_OBJECT en el archivo VentanaPrincipal.hpp y genera de manera automática los metadatos necesarios (moc_VentanaPrincipal.cpp) para conectar los clics de los botones con la ejecución del compilador.

### Redirección de Flujo (std::cout)
Para mostrar los resultados en la pantalla gráfica sin alterar el código analítico original, el sistema redirige temporalmente el buffer de salida estándar hacia un buffer de memoria (std::stringstream). Al finalizar el análisis, Qt captura esta información y la vuelca en la consola del IDE. 

### Motor de Animación 2D
El panel de memoria RAM funciona a través de un lienzo interactivo (QGraphicsScene) y un visor (QGraphicsView). Al procesar las instrucciones, el entorno genera tarjetas gráficas con bordes redondeados (mediante QGraphicsProxyWidget) que se introducen en el lienzo con un efecto de movimiento y rebote elástico (QPropertyAnimation con la curva OutBack), simulando visualmente la asignación de variables en la memoria física.

---

## Guía de Compilación y Automatización

El proyecto incluye un archivo Makefile adaptativo que gestiona todo el proceso de construcción, interactuando con pkg-config para detectar de forma automática las dependencias de Qt6 o Qt5 instaladas en el sistema.

### Instalación de Dependencias (Linux - Ubuntu/Debian)
Para instalar el compilador, las herramientas de desarrollo y las librerías gráficas necesarias, ejecute en su terminal:

```bash
sudo apt update
sudo apt install qt6-base-dev qt6-base-dev-tools pkg-config g++ make
