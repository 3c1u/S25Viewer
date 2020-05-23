#ifndef S25LAYERMODEL_H
#define S25LAYERMODEL_H

#include "S25ImageRenderer.h"
#include "s25image.h"

#include <QAbstractTableModel>

class S25LayerModel : public QAbstractTableModel {
  Q_OBJECT
  Q_PROPERTY(S25Image *image READ image WRITE setImage NOTIFY imageChanged)

public:
  S25LayerModel(QObject *parent = nullptr, S25ImageRenderer *view = nullptr);

  int      rowCount(const QModelIndex &parent) const override;
  int      columnCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  void          bindView(S25ImageRenderer *view);

  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole) override;

  S25Image *image() const { return m_image; }

  void setImage(S25Image *image) {
    m_image = image;
    emit imageChanged();
  }
public slots:
  void updateModel();

signals:
  void imageChanged();

private:
  enum S25LayerModelRole {
    kS25LayerModelLayerNumber = 0,
    kS25LayerModelPictLayerNumber,
    kS25LayerModelVisibilityFlag,
  };

  S25ImageRenderer *m_view;
  S25Image *        m_image;
};

#endif // S25LAYERMODEL_H
