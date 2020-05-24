#include "S25LayerModel.h"

#include <QColor>
#include <QDebug>

S25LayerModel::S25LayerModel(QObject *parent) : QAbstractTableModel(parent) {}

void S25LayerModel::updateModel() { emit layoutChanged(); }

int S25LayerModel::rowCount(const QModelIndex &parent) const {
  Q_UNUSED(parent)
  qDebug() << "S25LayerModel::rowCount";

  if (m_image) {
    return m_image->getTotalLayers();
  } else {
    return 0;
  }
}

int S25LayerModel::columnCount(const QModelIndex &parent) const {
  Q_UNUSED(parent)
  qDebug() << "S25LayerModel::columnCount";

  return 2;
}

Qt::ItemFlags S25LayerModel::flags(const QModelIndex &index) const {
  switch (index.column()) {
  case kS25LayerModelLayerNumber:
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    break;
  case kS25LayerModelPictLayerNumber:
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    break;
  /* case kS25LayerModelVisibilityFlag:
   */
  default:
    qDebug() << "out of index";
    break;
  }

  return Qt::NoItemFlags;
}

bool S25LayerModel::setData(const QModelIndex &index, const QVariant &value,
                            int role) {
  auto val = value.toInt();

  switch (role) {
  case kS25LayerModelLayerNumber:
    break;
  case kS25LayerModelPictLayerNumber:
    m_image->setPictLayer(index.row(), val);
    emit dataChanged(index, index, QVector{role});
    return true;
    break;
  /* case kS25LayerModelVisibilityFlag:
    if (role == Qt::CheckStateRole || role == Qt::EditRole)
      return Qt::Checked;
    break; */
  default:
    qDebug() << "out of index";
    break;
  }

  return false;
}

QVariant S25LayerModel::data(const QModelIndex &index, int role) const {
  qDebug() << "S25LayerModel::data";

  if (!index.isValid()) {
    return QVariant{};
  }

  switch (role) {
  case kS25LayerModelLayerNumber:
    return QString("Layer %1").arg(index.row() + 1);
    break;
  case kS25LayerModelPictLayerNumber:
    return m_image->getPictLayerFor(index.row());
    break;
  /* case kS25LayerModelVisibilityFlag:
    if (role == Qt::CheckStateRole || role == Qt::EditRole)
      return Qt::Checked;
    break; */
  default:
    qDebug() << "out of index";
    break;
  }

  return QVariant{};
}

S25Image *S25LayerModel::image() const { return m_image; }

void S25LayerModel::setImage(S25Image *image) {
  m_image = image;

  connect(m_image, &S25Image::update, this, &S25LayerModel::updateModel);

  emit imageChanged();
}

QHash<int, QByteArray> S25LayerModel::roleNames() const {
  qDebug() << "S25LayerModel::roleNames";
  return {
      {kS25LayerModelLayerNumber, "layerNo"},
      {kS25LayerModelPictLayerNumber, "pictLayer"},
  };
}
