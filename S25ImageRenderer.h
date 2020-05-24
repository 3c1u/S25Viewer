#ifndef S25IMAGEVIEW_H
#define S25IMAGEVIEW_H

#include <memory>
#include <optional>
#include <vector>

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QQuickWindow>
#include <QUrl>

#include "S25DecoderWrapper.h"

class S25ImageRenderer : public QObject, protected QOpenGLFunctions {
  Q_OBJECT
public:
  S25ImageRenderer(QQuickWindow *window);

  void resize(int w, int h);

  // for S25Image
  void loadImage(QUrl const &path);

  // for S25LayerModel
  int  getTotalLayers() const;
  int  getPictLayerFor(unsigned long layer) const;
  bool getPictLayerIsValid(unsigned long layer) const;

  void setPictLayer(unsigned long layer, int pictLayer);

signals:
  void imageLoaded(QUrl const &theUrl);
  void update();

public slots:
  void init();
  void paint();

private:
  QQuickWindow *m_window;

  std::optional<S25pArchive>            m_archive;
  std::vector<std::optional<S25pImage>> m_images;
  std::vector<int32_t>                  m_imageEntries;

  std::vector<GLuint> m_textures;
  std::vector<GLuint> m_vertexBuffers;

  GLuint m_uvBuffer;

  QOpenGLVertexArrayObject m_vao;
  GLuint                   m_program;

  bool m_isInitialized;

  int m_viewportWidth;
  int m_viewportHeight;

  bool loadArchive(QString const &path);
  void loadImagesToTexture();
  void loadVertexBuffers();
};

#endif // S25IMAGEVIEW_H
