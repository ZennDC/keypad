#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include "controller.hpp"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    QQuickStyle::setStyle("Material");

    QQmlApplicationEngine engine;

    // The QML engine automatically manages and instantiates 
    // the DeviceController singleton via QML_SINGLETON in controller.hpp.
    engine.loadFromModule("Configurator", "Main");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
