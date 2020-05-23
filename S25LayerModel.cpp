#include "S25LayerModel.h"

#include <QColor>
#include <QDebug>

S25LayerModel::S25LayerModel(QObject *parent, S25ImageView *view)
    : QAbstractTableModel(parent), m_view{view} {}

void S25LayerModel::updateModel() { emit layoutChanged(); }

int S25LayerModel::rowCount(const QModelIndex &parent) const {
  Q_UNUSED(parent)

  if (m_view) {
    return m_view->getTotalLayers();
  } else {
    return 0;
  }
}

int S25LayerModel::columnCount(const QModelIndex &parent) const {
  Q_UNUSED(parent)

  return 2;
}

void S25LayerModel::bindView(S25ImageView *view) { m_view = view; }

Qt::ItemFlags S25LayerModel::flags(const QModelIndex &index) const {
  switch (index.column()) {
  case kS25LayerModelLayerNumber:
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    break;
  case kS25LayerModelPictLayerNumber:
    return Qt::ItemIsEnabled | Qt::ItemIsEditable;
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

  switch (index.column()) {
  case kS25LayerModelLayerNumber:
    break;
  case kS25LayerModelPictLayerNumber:
    m_view->setPictLayer(index.row(), val);
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
  if (!index.isValid()) {
    return QVariant{};
  }

  switch (index.column()) {
  case kS25LayerModelLayerNumber:
    if (role == Qt::DisplayRole)
      return QString("Layer %1").arg(index.row() + 1);

    break;
  case kS25LayerModelPictLayerNumber:
    if (role == Qt::DisplayRole || role == Qt::EditRole)
      return m_view->getPictLayerFor(index.row());
    else if (role == Qt::ForegroundRole)
      if (!m_view->getPictLayerIsValid(index.row()))
        return QColor::fromRgb(255, 0, 0);
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

QVariant S25LayerModel::headerData(int section, Qt::Orientation orientation,
                                   int role) const {
  if (orientation == Qt::Vertical || role != Qt::DisplayRole) {
    return QVariant{};
  }

  switch (section) {
  case kS25LayerModelLayerNumber:
    return tr("Layer");
    break;
  case kS25LayerModelPictLayerNumber:
    return tr("Pict Layer");
    break;
  case kS25LayerModelVisibilityFlag:
    return tr("Visible");
    break;
  default:
    break;
  }

  return QVariant{};
}
