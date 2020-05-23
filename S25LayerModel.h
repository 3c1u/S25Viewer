#ifndef S25LAYERMODEL_H
#define S25LAYERMODEL_H

#include "s25imageview.h"
#include <QAbstractTableModel>

class S25LayerModel : public QAbstractTableModel {
  Q_OBJECT
public:
  S25LayerModel(QObject *parent = nullptr, S25ImageView *view = nullptr);

  int      rowCount(const QModelIndex &parent) const override;
  int      columnCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  void          bindView(S25ImageView *view);

  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole) override;
public slots:
  void updateModel();

private:
  enum S25LayerModelRole {
    kS25LayerModelLayerNumber = 0,
    kS25LayerModelPictLayerNumber,
    kS25LayerModelVisibilityFlag,
  };

  S25ImageView *m_view;
};

#endif // S25LAYERMODEL_H
