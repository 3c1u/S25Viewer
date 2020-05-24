#ifndef S25LAYERMODEL_H
#define S25LAYERMODEL_H

#include "S25ImageRenderer.h"
#include "s25image.h"

#include <QAbstractTableModel>
#include <QHash>
#include <QQmlApplicationEngine>

class S25LayerModel : public QAbstractTableModel {
  Q_OBJECT
  Q_PROPERTY(S25Image *image READ image WRITE setImage NOTIFY imageChanged)

public:
  S25LayerModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;

  QVariant      data(const QModelIndex &index, int role) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  bool          setData(const QModelIndex &index, const QVariant &value,
                        int role = Qt::EditRole) override;

  QHash<int, QByteArray> roleNames() const override;

  S25Image *image() const;
  void      setImage(S25Image *image);
public slots:
  void updateModel();

signals:
  void imageChanged();

private:
  enum S25LayerModelRole {
    kS25LayerModelLayerNumber = Qt::UserRole + 1,
    kS25LayerModelPictLayerNumber,
    kS25LayerModelVisibilityFlag,
  };

  S25Image *        m_image;
};

#endif // S25LAYERMODEL_H
