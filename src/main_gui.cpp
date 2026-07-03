#include <QApplication>
#include "gui/VentanaPrincipal.hpp"

int main(int argc, char *argv[]) {
    // Inicializador obligatorio del motor de interfaz gráfica de Qt
    QApplication app(argc, argv);

    // Instanciar y mostrar nuestra ventana modificable
    VentanaPrincipal ventana;
    ventana.show();

    // Cede el control del hilo principal a la interfaz para que responda a clics y rediseños
    return app.exec();
}
