#ifndef X13_H
#define X13_H

#ifdef __cplusplus
extern "C" {
#endif

// input is 80 bytes long, output is 32 bytes
  void x11_hash(const char* input, size_t sz, char* output);

#ifdef __cplusplus
}
#endif

#endif
