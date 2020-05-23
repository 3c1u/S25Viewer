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
    "\n"
    "void main() {\n"
    "  vec2 pos = vec2(vert.x / viewport.x, - vert.y / viewport.y);"
    "  uv = uv_;\n"
    "  gl_Position = vec4(pos, 0, 1);\n"
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
      m_imageEntries{}, m_textures{}, m_viewportWidth{0}, m_viewportHeight{0} {}

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
  f->glUniform2f(0, m_viewportWidth, m_viewportHeight);

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
  this->loadArchive(url.path());

  // load S25 into texture
  loadImagesToTexture();
  loadVertexBuffers();

  emit imageLoaded();

  // force update
  update();
}

void S25ImageView::loadArchive(QString const &path) {
  auto pathAsUtf8 = path.toUtf8();
  auto arc        = S25pArchive(pathAsUtf8);

  qDebug() << "image loaded: " << path << ", with " << arc.getTotalEntries()
           << "entries";

  // select nothing
  m_imageEntries.resize(arc.getTotalLayers(), -1);

  m_archive = std::make_optional(std::move(arc));
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

  for (size_t i = 0; i < entries; i++) {
    if (!m_images[i]) {
      continue;
    }

    const auto &img = *m_images[i];

    max_width  = fmax(img.getWidth(), max_width);
    max_height = fmax(img.getHeight(), max_height);
  }

  for (size_t i = 0; i < entries; i++) {
    if (!m_images[i]) {
      continue;
    }

    const auto &img = *m_images[i];

    auto x1 = (float)img.getOffsetX() - max_width * 0.5f;
    auto y1 = (float)img.getOffsetY() - max_height * 0.5f;
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
