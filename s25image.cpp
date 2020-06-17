#include "s25image.h"
#include <QQuickWindow>
#include <QSize>

S25Image::S25Image() : m_renderer(nullptr) {
  connect(this, &QQuickItem::windowChanged, this,
          &S25Image::handleWindowChanged, Qt::DirectConnection);
  setFlag(QQuickItem::ItemHasContents, true);
}

S25Image::~S25Image() {
  if (m_renderer) {
    delete m_renderer;
  }
}

void S25Image::setUrl(QUrl const &path) {
  if (m_renderer) {
    m_renderer->loadImage(path);
  }
}

void S25Image::handleUpdate() {
  QQuickItem::update();
  qDebug() << "update called";
  emit update();
}

void S25Image::handleImageLoaded(QUrl theUrl) { emit imageLoaded(theUrl); }

void S25Image::handleWindowChanged(QQuickWindow *win) {
  if (win) {
    connect(win, &QQuickWindow::beforeSynchronizing, this, &S25Image::sync,
            Qt::DirectConnection);
    connect(win, &QQuickWindow::sceneGraphInvalidated, this, &S25Image::cleanup,
            Qt::DirectConnection);
  }
}

void S25Image::sync() {
  if (!m_renderer) {
    m_renderer = new S25ImageRenderer(window());

    connect(window(), &QQuickWindow::beforeRendering, m_renderer,
            &S25ImageRenderer::init, Qt::DirectConnection);
    connect(window(), &QQuickWindow::beforeRenderPassRecording, m_renderer,
            &S25ImageRenderer::paint, Qt::DirectConnection);

    connect(m_renderer, &S25ImageRenderer::imageLoaded, this,
            &S25Image::handleImageLoaded, Qt::DirectConnection);
    connect(m_renderer, &S25ImageRenderer::update, this,
            &S25Image::handleUpdate, Qt::DirectConnection);
  }

  auto const size = window()->size();
  m_renderer->resize(size.width(), size.height());
}

void S25Image::cleanup() {
  delete m_renderer;
  m_renderer = nullptr;
}

int S25Image::getTotalLayers() const {
  if (m_renderer) {
    return m_renderer->getTotalLayers();
  }

  return 0;
}

int S25Image::getPictLayerFor(unsigned long layer) const {
  if (m_renderer) {
    return m_renderer->getPictLayerFor(layer);
  }

  return -1;
}

bool S25Image::getPictLayerIsValid(unsigned long layer) const {
  if (m_renderer) {
    return m_renderer->getPictLayerIsValid(layer);
  }

  return false;
}

void S25Image::setPictLayer(unsigned long layer, int pictLayer) {
  if (m_renderer) {
    m_renderer->setPictLayer(layer, pictLayer);
  }
}
