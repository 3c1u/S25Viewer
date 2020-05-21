#ifndef S25DECODERWRAPPER_HPP
#define S25DECODERWRAPPER_HPP

#include "s25decoder/S25Decoder.h"
#include <optional>

class S25pImage {
public:
  S25pImage(S25Image *inner)
      : m_inner(inner) {
    S25ImageGetSize(m_inner, &m_width, &m_height);
    S25ImageGetOffset(m_inner, &m_x, &m_y);
  }

  ~S25pImage() {
    S25ImageRelease(m_inner);
  }

  S25pImage(S25pImage const&) = delete;
  S25pImage &operator=(S25pImage const &image) = delete;

  S25pImage(S25pImage &&image) {
    if (this == &image) {
      return;
    }

    this->m_inner = image.m_inner;
    image.m_inner = nullptr;

    S25ImageGetSize(m_inner, &m_width, &m_height);
    S25ImageGetOffset(m_inner, &m_x, &m_y);
  }

  S25pImage &operator=(S25pImage &&image) {
    if (this == &image) {
      return *this;
    }

    this->m_inner = image.m_inner;
    image.m_inner = nullptr;

    S25ImageGetSize(m_inner, &m_width, &m_height);
    S25ImageGetOffset(m_inner, &m_x, &m_y);

    return *this;
  }

  const uint8_t *getRGBABuffer(size_t *bufferSize) const {
    return S25ImageGetRGBABufferView(m_inner, bufferSize);
  }

  int getWidth() const {
    return m_width;
  }

  int getHeight() const {
    return m_height;
  }

  int getOffsetX() const {
    return m_x;
  }

  int getOffsetY() const {
    return m_y;
  }
private:
  S25Image *m_inner;
  int m_width;
  int m_height;
  int m_x;
  int m_y;
};

class S25pArchive {
public:
  S25pArchive(const char *path) {
    m_inner = S25ArchiveOpen(path);
  }

  ~S25pArchive() {
    S25ArchiveRelease(m_inner);
  }

  S25pArchive(S25pArchive const&) = delete;

  S25pArchive(S25pArchive &&archive) {
    if (this == &archive) {
      return;
    }

    this->m_inner = archive.m_inner;
    archive.m_inner = nullptr;
  }

  S25pArchive &operator=(S25pArchive &&archive) {
    if (this == &archive) {
      return *this;
    }

    this->m_inner = archive.m_inner;
    archive.m_inner = nullptr;

    return *this;
  }

  std::optional<S25pImage> getImage(size_t entry) {
    auto img = S25ArchiveLoadImage(m_inner, entry);
    auto img_wrapper = S25pImage(img);

    if (img) {
      return std::make_optional(S25pImage(std::move(img_wrapper)));
    } else {
      return std::nullopt;
    }
  }

  size_t getTotalEntries() const {
    return S25ArchiveGetTotalEntries(m_inner);
  }

  size_t getTotalLayers() const {
    return getTotalEntries() / 100 + 1;
  }
private:
  S25Archive *m_inner;
};

#endif // S25DECODERWRAPPER_HPP
