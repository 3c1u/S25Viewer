#include "widget.h"
#include "./ui_widget.h"

Widget::Widget(QWidget *parent) : QWidget(parent), ui(new Ui::Widget) {
  ui->setupUi(this);

  // Set model.
  // > The view does not take ownership of the model unless it is the model's
  // parent object ... >
  // (https://doc.qt.io/qt-5/qabstractitemview.html#setModel)
  m_model = new S25LayerModel(ui->tableView, ui->openGLWidget);
  ui->tableView->setModel(m_model);

  connect(ui->openGLWidget, SIGNAL(imageLoaded()), m_model,
          SLOT(updateModel()));
}

Widget::~Widget() { delete ui; }
