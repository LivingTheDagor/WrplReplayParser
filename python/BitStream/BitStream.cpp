#include <danet/daNetTypes.h>
#include "danet/BitStream.h"
#include "modules/BitStream.h"
extern "C" {
#include "lz4.h"
}
#include "Logger.h"
#include "span"

using ssize_t = Py_ssize_t;
PyBitStream py_bitstream{};

std::optional<py::bytes> read_bits(BitStream &bs, uint32_t bits) {
  if((bits) > bs.GetNumberOfUnreadBits())
    return {};
  std::string payload{};
  payload.resize(BITS_TO_BYTES(bits));
  if(bs.ReadBits((uint8_t*)payload.data(), bits)) {
    return {payload};
  }
  return {};
}
///mnt/d/ReplayParser/cmake-build-wsldebugasan/experiment/python/PyReplayParser.cpython-312-x86_64-linux-gnu.so
std::span<const char> bytes_to_span(const py::bytes& py_bytes) {
  // Extract data pointer and size from py::bytes
  char* data;
  ssize_t size;
  // This will not copy, just gets access to internals
  PYBIND11_BYTES_AS_STRING_AND_SIZE(py_bytes.ptr(), &data, &size);

  // Convert to std::span<std::byte>
  return {reinterpret_cast<const char*>(data), static_cast<size_t>(size)};
}

void PyBitStream::include(py::module_ &m) {
  DO_INCLUDE()
  py::class_<BitStream>(m, "BitStream")
      .def(py::init([](py::bytes &data){
        auto spn = bytes_to_span(data);
        return BitStream(spn.data(), spn.size(), true);
      }))
      .def("ReadBytes", [](BitStream &bs, uint32_t bytes) -> std::optional<py::bytes> {
        return read_bits(bs, BYTES_TO_BITS(bytes));
      }, py::arg("bytes"))
      .def("ReadBits", [](BitStream &bs, uint32_t bits) -> std::optional<py::bytes> {
        return read_bits(bs, bits);
      }, py::arg("bits"))
      .def("ReadUInt8", [](BitStream &bs) -> std::optional<uint8_t> {
        uint8_t v;
        if(bs.Read(v)) {
          return {v};
        }
        return {};
      })
      .def("ReadUInt16", [](BitStream &bs) -> std::optional<uint16_t> {
        uint16_t v;
        if(bs.Read(v)) {
          return {v};
        }
        return {};
      })
      .def("ReadUInt32", [](BitStream &bs) -> std::optional<uint32_t> {
        uint32_t v;
        if(bs.Read(v)) {
          return {v};
        }
        return {};
      })
      .def("ReadUInt64", [](BitStream &bs) -> std::optional<uint64_t> {
        uint64_t v;
        if(bs.Read(v)) {
          return {v};
        }
        return {};
      })
      .def("ReadInt8", [](BitStream &bs) -> std::optional<int8_t> {
        int8_t v;
        if(bs.Read(v)) {
          return {v};
        }
        return {};
      })
      .def("ReadInt16", [](BitStream &bs) -> std::optional<int16_t> {
        int16_t v;
        if(bs.Read(v)) {
          return {v};
        }
        return {};
      })
      .def("ReadInt", [](BitStream &bs) -> std::optional<int32_t> {
        int32_t v;
        if(bs.Read(v)) {
          return {v};
        }
        return {};
      })
      .def("ReadInt64", [](BitStream &bs) -> std::optional<int64_t> {
        int64_t v;
        if(bs.Read(v)) {
          return {v};
        }
        return {};
      })
      .def("ReadFloat", [](BitStream &bs) -> std::optional<float> {
        float v;
        if(bs.Read(v)) {
          return {v};
        }
        return {};
      })
      .def("ReadDouble", [](BitStream &bs) -> std::optional<double> {
        double v;
        if(bs.Read(v)) {
          return {v};
        }
        return {};
      })
      .def("ReadString", [](BitStream &bs) -> std::optional<std::string> {
        std::string v{};
        if(bs.Read(v)) {
          return {v};
        }
        return {};
      })
      .def("ReadUleb16", [](BitStream &bs) -> std::optional<uint16_t> {
        uint16_t v{};
        if(bs.ReadCompressed(v)) {
          return {v};
        }
        return {};
      })
      .def("ReadUleb32", [](BitStream &bs) -> std::optional<uint32_t> {
        uint32_t v{};
        if(bs.ReadCompressed(v)) {
          return {v};
        }
        return {};
      })
      .def("ReadUleb64", [](BitStream &bs) -> std::optional<uint64_t> {
        uint64_t v{};
        if(bs.ReadCompressed(v)) {
          return {v};
        }
        return {};
      })
      .def("ReadBool", [](BitStream &bs) -> std::optional<bool> {
        bool v{};
        if(bs.Read(v)) {
          return {v};
        }
        return {};
      })

      .def("ReadBS", [](BitStream &bs) -> std::optional<BitStream> {
        BitStream v{};
        if(bs.Read(v)) {
          return {v};
        }
        return {};
      })
      .def("readBitsInt", [](BitStream &bs, uint32_t bits) -> std::optional<uint64_t> {
        if(bits > 64) {
          throw py::index_error{};
        }
        uint64_t v;
        if(bs.Read(v)) {
          v >>= (64-bits);
          return {v};
        }
        return {};
      }, py::arg("bits"))
      .def("IgnoreBits", &BitStream::IgnoreBits)
      .def("IgnoreBytes", &BitStream::IgnoreBytes)
      .def("GetReadOffset", &BitStream::GetReadOffset)
      .def("SetReadOffset", &BitStream::SetReadOffset, py::arg("offset"))
      .def("GetNumberOfBitsUsed", &BitStream::GetNumberOfBitsUsed)
      .def("GetNumberOfUnreadBits", &BitStream::GetNumberOfUnreadBits)
      .def("AlignReadToByteBoundary", &BitStream::AlignReadToByteBoundary)
      .def("GetData", [](BitStream &bs) {
        return py::memoryview::from_memory(
            bs.GetData(),
            (ssize_t)bs.GetNumberOfBytesUsed(),
            true
        );
      });
  m.def("lz4_decompress", [](py::bytes &in_data, size_t &out_len) -> py::bytes{
    auto spn = bytes_to_span(in_data);
    std::string payload{};
    payload.resize(out_len);
    int readedSize = LZ4_decompress_safe(spn.data(), payload.data(), spn.size(), payload.size());
    if (readedSize < 0) {
      throw py::value_error(fmt::format("lz4 decompress fail error code: {}", readedSize));
    }
    payload.resize(readedSize);
    return {payload};
  }, py::arg("compress_data"), py::arg("max_output_size"));
  m.def("BYTES_TO_BITS", [](BitSize_t in){
    return BYTES_TO_BITS(in);
  });
  m.def("BITS_TO_BYTES", [](BitSize_t in) {
    return BITS_TO_BYTES(in);
  });
}