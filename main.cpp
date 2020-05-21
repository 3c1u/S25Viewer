#include "widget.h"

#include <QApplication>
#include <QSurfaceFormat>

int main(int argc, char *argv[])
{
  QSurfaceFormat fmt;
  fmt.setVersion(4,0);
  fmt.setProfile(QSurfaceFormat::CoreProfile);
  QSurfaceFormat::setDefaultFormat(fmt);

  QApplication a(argc, argv);
  Widget w;
  w.show();
  return a.exec();
}
