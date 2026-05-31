// Single translation unit that compiles the stb_image implementation.
// stb_image is a public-domain PNG/image decoder (see third_party/stb_image.h).
// We only need PNG decoding from memory (input prompt glyph atlases are
// embedded as byte arrays), so STDIO is disabled and the decoder is limited
// to PNG to keep the build lean.
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#include "third_party/stb_image.h"
