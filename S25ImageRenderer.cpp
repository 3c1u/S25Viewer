#include <QDebug>
#include <QDrag>
#include <QDropEvent>
#include <QMimeData>
#include <QOpenGLFunctions>

#include "S25DecoderWrapper.h"
#include "S25ImageRenderer.h"

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

S25ImageRenderer::S25ImageRenderer(QQuickWindow *window)
    : m_window{window}, m_archive{std::nullopt}, m_images{}, m_imageEntries{},
      m_textures{}, m_isInitialized{false}, m_viewportWidth{0},
      m_viewportHeight{0} {}

int S25ImageRenderer::getTotalLayers() const {
  if (m_archive) {
    return m_archive->getTotalLayers();
  }

  return 0;
}

int S25ImageRenderer::getPictLayerFor(unsigned long layer) const {
  if (m_archive && layer < m_imageEntries.size()) {
    return m_imageEntries[layer];
  }

  return -1;
}

bool S25ImageRenderer::getPictLayerIsValid(unsigned long layer) const {
  if (m_archive && layer < m_images.size()) {
    return !!m_images[layer] || m_imageEntries[layer] == -1;
  }

  return false;
}

void S25ImageRenderer::setPictLayer(unsigned long layer, int pictLayer) {
  if (m_archive && layer < m_images.size()) {
    m_imageEntries[layer] = pictLayer;

    loadImagesToTexture();
    loadVertexBuffers();

    emit update();
  }
}

void S25ImageRenderer::init() {
  if (m_isInitialized) {
    return;
  }

  m_isInitialized = true;

  QSGRendererInterface *rif = m_window->rendererInterface();
  qDebug() << "api: " << rif->graphicsApi();

  Q_ASSERT(rif->graphicsApi() == QSGRendererInterface::OpenGL ||
           rif->graphicsApi() == QSGRendererInterface::OpenGLRhi);

  initializeOpenGLFunctions();

  glEnable(GL_FRAMEBUFFER_SRGB);

  // create shader program

  auto vShader = glCreateShader(GL_VERTEX_SHADER);

  glShaderSource(vShader, 1, &vertShader, nullptr);
  glCompileShader(vShader);

  GLint result;
  GLint log_length;

  glGetShaderiv(vShader, GL_COMPILE_STATUS, &result);
  glGetShaderiv(vShader, GL_INFO_LOG_LENGTH, &log_length);

  std::vector<char> errMessage(log_length);
  glGetShaderInfoLog(vShader, log_length, NULL, errMessage.data());
  qDebug() << "vertex shader status: " << errMessage.data();

  auto fShader = glCreateShader(GL_FRAGMENT_SHADER);

  glShaderSource(fShader, 1, &fragShader, nullptr);
  glCompileShader(fShader);

  glGetShaderiv(fShader, GL_COMPILE_STATUS, &result);
  glGetShaderiv(fShader, GL_INFO_LOG_LENGTH, &log_length);

  errMessage.resize(log_length, 0);
  glGetShaderInfoLog(fShader, log_length, NULL, errMessage.data());
  qDebug() << "frag shader status: " << errMessage.data();

  auto program = glCreateProgram();
  glAttachShader(program, vShader);
  glAttachShader(program, fShader);
  glLinkProgram(program);

  glGetProgramiv(program, GL_LINK_STATUS, &result);
  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);

  errMessage.resize(log_length, 0);
  glGetProgramInfoLog(program, log_length, NULL, errMessage.data());
  qDebug() << "link status: " << errMessage.data();

  glDeleteShader(vShader);
  glDeleteShader(fShader);

  m_program = program;

  // m_vao.create();
  // m_vao.bind();

  glGenBuffers(1, &m_uvBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, m_uvBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(uvBuffer), uvBuffer, GL_STATIC_DRAW);
}

void S25ImageRenderer::paint() {
  m_window->beginExternalCommands();

  // m_vao.bind();

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // guard empty S25 archive
  if (!m_archive) {
    m_window->resetOpenGLState();
    m_window->endExternalCommands();
    return;
  }

  qDebug() << "paintGL call";

  auto &archive = *m_archive;
  auto  entries = archive.getTotalLayers();

  glUseProgram(m_program);
  glUniform2f(0, m_viewportWidth, m_viewportHeight);

  auto pixelRatio = m_window->devicePixelRatio();
  glViewport(0, 0, m_viewportWidth * pixelRatio, m_viewportHeight * pixelRatio);

  glDisable(GL_DEPTH_TEST);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, m_uvBuffer);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);

  for (size_t i = 0; i < entries; i++) {
    if (!m_images[i]) {
      continue;
    }

    qDebug() << "paint entry: " << i;

    auto tex = m_textures[i];
    auto vtx = m_vertexBuffers[i];

    glBindTexture(GL_TEXTURE_2D, tex);

    glBindBuffer(GL_ARRAY_BUFFER, vtx);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);

    glDrawArrays(GL_TRIANGLES, 0, 6);
  }

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);

  glUseProgram(0);

  m_window->resetOpenGLState();
  m_window->endExternalCommands();
}

void S25ImageRenderer::resize(int width, int height) {
  m_viewportWidth  = width;
  m_viewportHeight = height;
}

void S25ImageRenderer::loadImage(QUrl const &theUrl) {
  qDebug() << "s25 drop event";

  if (loadArchive(theUrl.path())) {
    emit imageLoaded(theUrl);
  }

  // load S25 into texture
  loadImagesToTexture();
  loadVertexBuffers();

  // force update
  emit update();
}

bool S25ImageRenderer::loadArchive(QString const &path) {
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

void S25ImageRenderer::loadVertexBuffers() {
  m_window->beginExternalCommands();

  // guard empty S25 archive
  if (!m_archive) {
    return;
  }

  qDebug() << "load vertex buffers";

  auto &archive = *m_archive;
  auto  entries = archive.getTotalLayers();

  // clear vertex buffers
  glDeleteBuffers(m_vertexBuffers.size(), m_vertexBuffers.data());

  m_vertexBuffers.resize(entries, 0);
  glGenBuffers(entries, m_vertexBuffers.data());

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

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffers[i]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(buf), buf, GL_STATIC_DRAW);
  }

  m_window->resetOpenGLState();
  m_window->endExternalCommands();
}

void S25ImageRenderer::loadImagesToTexture() {
  m_window->beginExternalCommands();

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
  glDeleteTextures(m_textures.size(), m_textures.data());

  // generate textures
  m_textures.resize(entries, 0);
  glGenTextures(entries, m_textures.data());

  for (size_t i = 0; i < entries; i++) {
    if (!m_images[i]) {
      continue;
    }

    auto        tex = m_textures[i];
    const auto &img = *m_images[i];

    qDebug() << "load entry " << i << "; (w, h) = " << img.getWidth() << ", "
             << img.getHeight();

    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, img.getWidth(),
                 img.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 img.getRGBABuffer(nullptr));

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  }

  m_window->resetOpenGLState();
  m_window->endExternalCommands();
}
