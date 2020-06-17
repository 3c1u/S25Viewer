#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include <QSurfaceFormat>

#include "S25LayerModel.h"
#include "s25image.h"

int main(int argc, char *argv[]) {
  QSurfaceFormat fmt;
  fmt.setVersion(4, 0);
  fmt.setProfile(QSurfaceFormat::CoreProfile);
  QSurfaceFormat::setDefaultFormat(fmt);

  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QGuiApplication app(argc, argv);

  // register
  qmlRegisterType<S25LayerModel>("S25Viewer", 1, 0, "LayerModel");
  qmlRegisterType<S25Image>("S25Viewer", 1, 0, "Image");

  QQmlApplicationEngine engine;
  const QUrl            url(QStringLiteral("qrc:/viewer.qml"));
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreated, &app,
      [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
          QCoreApplication::exit(-1);
      },
      Qt::QueuedConnection);
  engine.load(url);

  return app.exec();
}
