#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include "device_controller.hpp"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    QQuickStyle::setStyle("Material");

    QQmlApplicationEngine engine;
    engine.loadFromModule("Configurator", "Main");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}