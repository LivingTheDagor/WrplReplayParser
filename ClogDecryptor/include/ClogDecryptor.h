

#ifndef MYEXTENSION_CLOGDECRYPTOR_H
#define MYEXTENSION_CLOGDECRYPTOR_H
#include "string"
#include "fstream"
#include "span"
#include "vector"
#include "iostream"

#include "dag_assert.h"

// A .clog file is XOR'ed with this key. Byte n of the file uses key byte n % xor_key_len.
constexpr unsigned char xor_key[] = {
    130, 135, 151, 64, 141, 139, 70, 11, 187, 115, 148, 3, 229, 179, 131, 83,
    105, 107, 131, 218, 149, 175, 74, 35, 135, 229, 151, 172, 36, 88, 175, 54,
    78, 225, 90, 249, 241, 1, 75, 177, 173, 182, 76, 76, 250, 116, 40, 105,
    194, 139, 17, 23, 213, 182, 71, 206, 179, 183, 205, 85, 254, 249, 193, 36,
    255, 174, 144, 46, 73, 108, 78, 9, 146, 129, 78, 103, 188, 107, 156, 222,
    177, 15, 104, 186, 139, 128, 68, 5, 135, 94, 243, 78, 254, 9, 151, 50,
    192, 173, 159, 233, 187, 253, 77, 6, 145, 80, 137, 110, 224, 232, 238, 153,
    83, 0, 60, 166, 184, 34, 65, 50, 177, 189, 245, 40, 80, 224, 114, 174
};
constexpr std::size_t xor_key_len = sizeof(xor_key) / sizeof(xor_key[0]);

class Decryptor {
private:
  std::size_t index;
  std::ofstream *write;
  bool print;
public:
  Decryptor(char * path, bool print=true);
  ~Decryptor();


  // Call it with the file bytes in order: the key position continues from the last call. Returns false when the
  // text holds "flush_all_std_and_debug", and the continuous mode then stops.
  bool decrypt(const std::span<unsigned char> &data);
};

#endif //MYEXTENSION_CLOGDECRYPTOR_H
