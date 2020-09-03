#ifndef S25IMAGEVIEW_H
#define S25IMAGEVIEW_H

#include <memory>
#include <optional>
#include <vector>

#include <QGestureEvent>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QUrl>
#include <QWidget>

#include "S25DecoderWrapper.h"

class S25ImageView : public QOpenGLWidget {
  Q_OBJECT
public:
  S25ImageView(QWidget *parent);

  // override functions
  virtual void initializeGL() override;
  virtual void paintGL() override;
  virtual void resizeGL(int w, int h) override;

  // handle drop event
  virtual void dropEvent(QDropEvent *theEvent) override;
  virtual void dragEnterEvent(QDragEnterEvent *event) override;

  // handle touch events
  virtual bool event(QEvent *event) override;
  void         gestureEvent(QGestureEvent *event);

  // for S25LayerModel
  int  getTotalLayers() const;
  int  getPictLayerFor(unsigned long layer) const;
  bool getPictLayerIsValid(unsigned long layer) const;

  void setPictLayer(unsigned long layer, int pictLayer);

signals:
  void imageLoaded(QUrl theUrl);

private:
  std::optional<S25pArchive>            m_archive;
  std::vector<std::optional<S25pImage>> m_images;
  std::vector<int32_t>                  m_imageEntries;

  std::vector<GLuint> m_textures;
  std::vector<GLuint> m_vertexBuffers;

  GLuint m_uvBuffer;
  GLuint m_transform;
  GLuint m_viewport;

  QOpenGLVertexArrayObject m_vao;
  GLuint                   m_program;

  int m_viewportWidth;
  int m_viewportHeight;

  qreal m_currentScale, m_scale;

  bool loadArchive(QString const &path);
  void loadImagesToTexture();
  void loadVertexBuffers();
};

#endif // S25IMAGEVIEW_H
