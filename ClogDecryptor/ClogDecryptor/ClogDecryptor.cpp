#include "ClogDecryptor.h"

Decryptor::Decryptor(char *path, bool print)   {
  if(path)
  {
    write = new std::ofstream(path);
    G_ASSERTF(write, "Unable to write to file {}", path);
    G_ASSERTF(write->is_open(), "Unable to write to file {}", path);
  }
  this->print = print;
  this->index = 0;
}

constexpr std::size_t CHUNK_SIZE = 1<<15;

/*void print_in_chunks(const std::vector<unsigned char>& data) {
  size_t total = data.size();
  size_t offset = 0;
  while (offset < total) {
    size_t to_print = std::min(CHUNK_SIZE, total - offset);
    std::cout.write((const char *)&data[offset], to_print);
    //std::cout << std::endl;
    offset += to_print;
  }
}*/

void print_in_chunks(const std::vector<unsigned char>& data, std::ostream *buff) {
  //constexpr size_t CHUNK_SIZE = 1 << 15; // 32 KiB,
  const char* p = reinterpret_cast<const char*>(data.data());
  size_t total = data.size();
  size_t offset = 0;

  while (offset < total) {
    size_t n = std::min(CHUNK_SIZE, total - offset);
    buff->write(p + offset, n);
    offset += n;
  }
  buff->flush(); // single flush at the end
}

void Decryptor::decrypt(const std::span<unsigned char> &data) {
  std::vector<unsigned char> out{};
  out.resize(data.size());
  for (std::size_t i = 0; i < data.size(); i++)
  {
    out[i] = data[i] ^ xor_key[(index)% xor_key_len];
    index++;
  }
  if (print || write)
  {
    if(print)
      print_in_chunks(out, &std::cout);
    if (write)
      print_in_chunks(out, write);
  }
}

Decryptor::~Decryptor() {
  delete write;
}
