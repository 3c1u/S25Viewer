#ifndef S25DECODER_H
#define S25DECODER_H

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct S25Archive;
struct S25Image;

typedef int32_t S25DecoderError;

// TODO
enum S25DecoderErrorType {
  kS25NoError = 0,
  kS25FileIOError,
  kS25InvalidArchiveError,
  kS25UnsupportedFileFormatError,
  kS25NoEntryError,
};

S25Archive *S25ArchiveOpen(const char *path);
void        S25ArchiveRelease(S25Archive *archive);
S25Image *  S25ArchiveLoadImage(S25Archive *archive, size_t entry);
size_t      S25ArchiveGetTotalEntries(const S25Archive *archive);
void        S25ImageRelease(S25Image *image);
void        S25ImageGetSize(const S25Image *image, int *width, int *height);
void        S25ImageGetOffset(const S25Image *image, int *x, int *y);
const unsigned char *S25ImageGetBGRABufferView(const S25Image *image,
                                               size_t *        size);

#ifdef __cplusplus
};
#endif

#endif
