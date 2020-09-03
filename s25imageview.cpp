#include <QDebug>
#include <QDrag>
#include <QDropEvent>
#include <QMimeData>
#include <QOpenGLFunctions>

#include "S25DecoderWrapper.h"
#include "s25imageview.h"

static const char *vertShader =
    "#version 330\n"
    "layout(location = 0) in"
    "          vec2  vert;\n"
    "layout(location = 1) in"
    "          vec2  uv_;\n"
    "out       vec2  uv;\n"
    "uniform   vec2  viewport;\n"
    "uniform   mat4  transform;\n"
    "\n"
    "void main() {\n"
    "  uv = uv_;\n"
    "  gl_Position = transform * vec4(vert, 0, 1);\n"
    "}";

static const char *fragShader = "#version 330\n"
                                "uniform sampler2D u_image;"
                                "in      vec2      uv;"
                                "out     vec4      f_color;"
                                "\n"
                                "void main() {\n"
                                "  f_color = texture(u_image, uv);\n"
                                "}";

static float uvBuffer[] = {
    0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0, 1.0,
};

S25ImageView::S25ImageView(QWidget *parent)
    : QOpenGLWidget(parent), m_archive{std::nullopt}, m_images{},
      m_imageEntries{}, m_textures{}, m_viewportWidth{0},
      m_currentScale{1}, m_scale{1} {
  grabGesture(Qt::PanGesture);
  grabGesture(Qt::PinchGesture);
}

bool S25ImageView::event(QEvent *event) {
  if (event->type() == QEvent::Gesture) {
    event->accept();
    gestureEvent(dynamic_cast<QGestureEvent *>(event));
  }

  return QWidget::event(event);
}

void S25ImageView::gestureEvent(QGestureEvent *event) {
  if (!event) {
    return;
  }

  if (auto gesture =
          reinterpret_cast<QPinchGesture *>(event->gesture(Qt::PinchGesture))) {
    auto changeFlags = gesture->changeFlags();

    if (changeFlags & QPinchGesture::ScaleFactorChanged) {
      m_currentScale = gesture->totalScaleFactor();
      qDebug() << "scale: " << m_currentScale * m_scale;
    }

    if (gesture->state() == Qt::GestureFinished) {
      m_scale *= m_currentScale;
      m_currentScale = 1.0;
    }
  }

  if (auto gesture =
          reinterpret_cast<QPanGesture *>(event->gesture(Qt::PanGesture))) {
    qDebug() << "pan: " << gesture->delta();
  }

  // redraw
  update();
}

void S25ImageView::wheelEvent(QWheelEvent *event) {
  m_offset += event->pixelDelta();
  update();
}

int S25ImageView::getTotalLayers() const {
  if (m_archive) {
    return m_archive->getTotalLayers();
  }

  return 0;
}

int S25ImageView::getPictLayerFor(unsigned long layer) const {
  if (m_archive && layer < m_imageEntries.size()) {
    return m_imageEntries[layer];
  }

  return -1;
}

bool S25ImageView::getPictLayerIsValid(unsigned long layer) const {
  if (m_archive && layer < m_images.size()) {
    return !!m_images[layer] || m_imageEntries[layer] == -1;
  }

  return false;
}

void S25ImageView::setPictLayer(unsigned long layer, int pictLayer) {
  if (m_archive && layer < m_images.size()) {
    m_imageEntries[layer] = pictLayer;

    loadImagesToTexture();
    loadVertexBuffers();

    update();
  }
}

void S25ImageView::initializeGL() {
  auto f = QOpenGLContext::currentContext()->functions();

  f->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  f->glClearDepthf(1.0f);

  f->glEnable(GL_FRAMEBUFFER_SRGB);

  // create shader program

  auto vShader = f->glCreateShader(GL_VERTEX_SHADER);

  f->glShaderSource(vShader, 1, &vertShader, nullptr);
  f->glCompileShader(vShader);

  GLint result;
  GLint log_length;

  f->glGetShaderiv(vShader, GL_COMPILE_STATUS, &result);
  f->glGetShaderiv(vShader, GL_INFO_LOG_LENGTH, &log_length);

  std::vector<char> errMessage(log_length);
  f->glGetShaderInfoLog(vShader, log_length, NULL, errMessage.data());
  qDebug() << "vertex shader status: " << errMessage.data();

  auto fShader = f->glCreateShader(GL_FRAGMENT_SHADER);

  f->glShaderSource(fShader, 1, &fragShader, nullptr);
  f->glCompileShader(fShader);

  f->glGetShaderiv(fShader, GL_COMPILE_STATUS, &result);
  f->glGetShaderiv(fShader, GL_INFO_LOG_LENGTH, &log_length);

  errMessage.resize(log_length, 0);
  f->glGetShaderInfoLog(fShader, log_length, NULL, errMessage.data());
  qDebug() << "frag shader status: " << errMessage.data();

  auto program = f->glCreateProgram();
  f->glAttachShader(program, vShader);
  f->glAttachShader(program, fShader);
  f->glLinkProgram(program);

  f->glGetProgramiv(program, GL_LINK_STATUS, &result);
  f->glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);

  errMessage.resize(log_length, 0);
  f->glGetProgramInfoLog(program, log_length, NULL, errMessage.data());
  qDebug() << "link status: " << errMessage.data();

  f->glDeleteShader(vShader);
  f->glDeleteShader(fShader);

  m_program = program;

  m_vao.create();
  m_vao.bind();

  f->glGenBuffers(1, &m_uvBuffer);
  f->glBindBuffer(GL_ARRAY_BUFFER, m_uvBuffer);
  f->glBufferData(GL_ARRAY_BUFFER, sizeof(uvBuffer), uvBuffer, GL_STATIC_DRAW);

  m_viewport  = f->glGetUniformLocation(program, "viewport");
  m_transform = f->glGetUniformLocation(program, "transform");
}

void S25ImageView::paintGL() {
  auto f = QOpenGLContext::currentContext()->functions();

  // clear
  f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  m_vao.bind();

  // guard empty S25 archive
  if (!m_archive) {
    f->glFinish();
    return;
  }

  qDebug() << "paintGL call";

  auto &archive = *m_archive;
  auto  entries = archive.getTotalLayers();

  f->glUseProgram(m_program);
  f->glUniform2f(m_viewport, m_viewportWidth, m_viewportHeight);

  // create transform
  auto const tr = QTransform()
                      .scale(1.0 / m_viewportWidth, -1.0 / m_viewportHeight)
                      .scale(m_scale * m_currentScale, m_scale * m_currentScale)
                      .translate(m_offset.x(), m_offset.y());

  GLfloat mat[16] = {
      static_cast<float>(tr.m11()),
      static_cast<float>(tr.m12()),
      0,
      static_cast<float>(tr.m13()), //
      static_cast<float>(tr.m21()),
      static_cast<float>(tr.m22()),
      0,
      static_cast<float>(tr.m23()), //
      0,
      0,
      1,
      0, //
      static_cast<float>(tr.m31()),
      static_cast<float>(tr.m32()),
      0,
      static_cast<float>(tr.m33()), //
  };

  f->glUniformMatrix4fv(m_transform, 1, GL_FALSE, mat);

  f->glEnable(GL_BLEND);
  f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  f->glEnableVertexAttribArray(0);
  f->glEnableVertexAttribArray(1);

  f->glBindBuffer(GL_ARRAY_BUFFER, m_uvBuffer);
  f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);

  for (size_t i = 0; i < entries; i++) {
    if (!m_images[i]) {
      continue;
    }

    qDebug() << "paint entry: " << i;

    auto tex = m_textures[i];
    auto vtx = m_vertexBuffers[i];

    f->glBindTexture(GL_TEXTURE_2D, tex);

    f->glBindBuffer(GL_ARRAY_BUFFER, vtx);
    f->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);

    f->glDrawArrays(GL_TRIANGLES, 0, 6);
  }

  f->glFinish();
}

void S25ImageView::resizeGL(int width, int height) {
  m_viewportWidth  = width;
  m_viewportHeight = height;
}

void S25ImageView::dragEnterEvent(QDragEnterEvent *event) {
  event->acceptProposedAction();
}

void S25ImageView::dropEvent(QDropEvent *theEvent) {
  qDebug() << "s25 drop event";

  // load S25 image
  if (!theEvent->mimeData()->hasUrls()) {
    return;
  }

  const auto url = theEvent->mimeData()->urls().first();
  if (loadArchive(url.toLocalFile())) {
    emit imageLoaded(url);
  }

  m_offset       = QPoint();
  m_currentScale = 1.0;
  m_scale        = 1.0;

  // load S25 into texture
  loadImagesToTexture();
  loadVertexBuffers();

  // force update
  update();
}

bool S25ImageView::loadArchive(QString const &path) {
  auto pathAsUtf8 = path.toUtf8();
  auto arc        = S25pArchive(pathAsUtf8);

  if (!arc) {
    // archive load failed
    qDebug() << "failed to load archive";
    return false;
  }

  qDebug() << "image loaded: " << path << ", with " << arc.getTotalEntries()
           << "entries";

  // select nothing
  m_imageEntries.resize(arc.getTotalLayers(), -1);

  m_archive = std::make_optional(std::move(arc));

  return true;
}

void S25ImageView::loadVertexBuffers() {
  auto f = QOpenGLContext::currentContext()->functions();

  // guard empty S25 archive
  if (!m_archive) {
    return;
  }

  qDebug() << "load vertex buffers";

  auto &archive = *m_archive;
  auto  entries = archive.getTotalLayers();

  // clear vertex buffers
  f->glDeleteBuffers(m_vertexBuffers.size(), m_vertexBuffers.data());

  m_vertexBuffers.resize(entries, 0);
  f->glGenBuffers(entries, m_vertexBuffers.data());

  float max_width  = 0;
  float max_height = 0;

  float max_oX = 0;
  float max_oY = 0;

  for (size_t i = 0; i < entries; i++) {
    if (!m_images[i]) {
      continue;
    }

    const auto &img = *m_images[i];

    if (max_width < img.getWidth()) {
      max_width = img.getWidth();
      max_oX    = img.getOffsetX();
    }

    if (max_height < img.getHeight()) {
      max_height = img.getHeight();
      max_oY     = img.getOffsetY();
    }
  }

  for (size_t i = 0; i < entries; i++) {
    if (!m_images[i]) {
      continue;
    }

    const auto &img = *m_images[i];

    auto x1 = (float)img.getOffsetX() - max_width * 0.5f - max_oX;
    auto y1 = (float)img.getOffsetY() - max_height * 0.5f - max_oY;
    auto x2 = x1 + (float)img.getWidth();
    auto y2 = y1 + (float)img.getHeight();

    float buf[] = {
        x1, y1, x2, y1, x1, y2, x1, y2, x2, y1, x2, y2,
    };

    f->glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffers[i]);
    f->glBufferData(GL_ARRAY_BUFFER, sizeof(buf), buf, GL_STATIC_DRAW);
  }
}

void S25ImageView::loadImagesToTexture() {
  auto f = QOpenGLContext::currentContext()->functions();

  // guard empty S25 archive
  if (!m_archive) {
    return;
  }

  auto &archive = *m_archive;
  auto  entries = archive.getTotalLayers();

  // load S25 images
  m_images.clear();

  for (size_t i = 0; i < entries; i++) {
    auto entry = m_imageEntries[i];

    // empty image
    if (entry == -1) {
      m_images.push_back(std::nullopt);
      continue;
    }

    m_images.push_back(archive.getImage(entry + 100 * i));
  }

  // clear textures
  f->glDeleteTextures(m_textures.size(), m_textures.data());

  // generate textures
  m_textures.resize(entries, 0);
  f->glGenTextures(entries, m_textures.data());

  for (size_t i = 0; i < entries; i++) {
    if (!m_images[i]) {
      continue;
    }

    auto        tex = m_textures[i];
    const auto &img = *m_images[i];

    qDebug() << "load entry " << i << "; (w, h) = " << img.getWidth() << ", "
             << img.getHeight();

    f->glBindTexture(GL_TEXTURE_2D, tex);
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.getWidth(), img.getHeight(),
                    0, GL_RGBA, GL_UNSIGNED_BYTE, img.getRGBABuffer(nullptr));

    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  }
}
