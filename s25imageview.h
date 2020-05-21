#ifndef S25IMAGEVIEW_H
#define S25IMAGEVIEW_H

#include <memory>
#include <optional>
#include <vector>

#include <QWidget>
#include <QOpenGLWidget>
#include <QOpenGLVertexArrayObject>

#include "S25DecoderWrapper.h"

class S25ImageView : public QOpenGLWidget
{
  Q_OBJECT
public:
  S25ImageView(QWidget *parent);

  // override functions
  virtual void initializeGL() override;
  virtual void paintGL() override;
  virtual void resizeGL(int w, int h) override;

  // handle drop event
  virtual void dropEvent(QDropEvent *theEvent) override;
  virtual void dragEnterEvent(QDragEnterEvent* event) override;

private:
  std::optional<S25pArchive>                  m_archive;
  std::vector<std::optional<S25pImage>>       m_images;
  std::vector<int32_t>                        m_imageEntries;

  std::vector<GLuint> m_textures;
  std::vector<GLuint> m_vertexBuffers;

  GLuint m_uvBuffer;

  QOpenGLVertexArrayObject m_vao;
  GLuint m_program;

  int m_viewportWidth;
  int m_viewportHeight;

  void loadArchive(QString const& path);
  void loadImagesToTexture();
  void loadVertexBuffers();
};

#endif // S25IMAGEVIEW_H
