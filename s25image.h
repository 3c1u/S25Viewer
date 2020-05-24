#ifndef S25IMAGE_H
#define S25IMAGE_H

#include <QQuickItem>
#include <QUrl>

#include "S25ImageRenderer.h"

class S25Image : public QQuickItem
{
  Q_OBJECT
  Q_PROPERTY(QUrl url WRITE setUrl)
public:
  S25Image();
  ~S25Image();
  void setUrl(QUrl const &theUrl);
  void sync();
  void cleanup();

  int  getTotalLayers() const;
  int  getPictLayerFor(unsigned long layer) const;
  bool getPictLayerIsValid(unsigned long layer) const;

  void setPictLayer(unsigned long layer, int pictLayer);
signals:
  void imageLoaded(QUrl theUrl);
  void update();

private slots:
  void handleWindowChanged(QQuickWindow *win);
  void handleUpdate();
  void handleImageLoaded(QUrl theUrl);

private:
  S25ImageRenderer *m_renderer;
};

#endif // S25IMAGE_H
