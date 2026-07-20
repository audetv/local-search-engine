#include <QApplication>
#include "main_window.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Local Search Engine");
    app.setApplicationVersion("1.0.0");

    MainWindow window;
    window.setWindowTitle("Local Search Engine");
    window.resize(900, 700);
    window.show();

    return app.exec();
}