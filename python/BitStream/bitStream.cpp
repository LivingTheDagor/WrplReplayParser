#include <daNet/daNetTypes.h>
#include "daNet/bitStream.h"
#include "modules/BitStream.h"
extern "C" {
#include "lz4.h"
}
#include "Logger.h"
#include "span"
#include "idFieldSerializer.h"
#include "ioSys/dag_dataBlock.h"
#include "modules/bytes_to_span.h"

using ssize_t = Py_ssize_t;
PyBitStream py_bitstream{};

std::optional<py::bytes> read_bits(danet::BitStream &bs, uint32_t bits) {
  if ((bits) > bs.GetNumberOfUnreadBits())
    return {};
  std::string payload{};
  payload.resize(BITS_TO_BYTES(bits));
  if (bs.ReadBits((uint8_t *) payload.data(), bits)) {
    return {payload};
  }
  return {};
}

void PyBitStream::include(py::module_ &m) {
  DO_INCLUDE()
  py::class_<danet::BitStream>(m, "BitStream")
    .def(py::init([](py::bytes &data) {
      auto spn = bytes_to_span(data);
      return danet::BitStream(spn.data(), spn.size(), true);
    }))
    .def(
      "ReadBytes",
      [](danet::BitStream &bs, uint32_t bytes) -> std::optional<py::bytes> {
        return read_bits(bs, BYTES_TO_BITS(bytes));
      },
      py::arg("bytes"))
    .def(
      "ReadBits", [](danet::BitStream &bs, uint32_t bits) -> std::optional<py::bytes> { return read_bits(bs, bits); },
      py::arg("bits"))
    .def("ReadUInt8",
         [](danet::BitStream &bs) -> std::optional<uint8_t> {
           uint8_t v;
           if (bs.Read(v)) {
             return {v};
           }
           return {};
         })
    .def("ReadUInt16",
         [](danet::BitStream &bs) -> std::optional<uint16_t> {
           uint16_t v;
           if (bs.Read(v)) {
             return {v};
           }
           return {};
         })
    .def("ReadUInt32",
         [](danet::BitStream &bs) -> std::optional<uint32_t> {
           uint32_t v;
           if (bs.Read(v)) {
             return {v};
           }
           return {};
         })
    .def("ReadUInt64",
         [](danet::BitStream &bs) -> std::optional<uint64_t> {
           uint64_t v;
           if (bs.Read(v)) {
             return {v};
           }
           return {};
         })
    .def("ReadInt8",
         [](danet::BitStream &bs) -> std::optional<int8_t> {
           int8_t v;
           if (bs.Read(v)) {
             return {v};
           }
           return {};
         })
    .def("ReadInt16",
         [](danet::BitStream &bs) -> std::optional<int16_t> {
           int16_t v;
           if (bs.Read(v)) {
             return {v};
           }
           return {};
         })
    .def("ReadInt",
         [](danet::BitStream &bs) -> std::optional<int32_t> {
           int32_t v;
           if (bs.Read(v)) {
             return {v};
           }
           return {};
         })
    .def("ReadInt64",
         [](danet::BitStream &bs) -> std::optional<int64_t> {
           int64_t v;
           if (bs.Read(v)) {
             return {v};
           }
           return {};
         })
    .def("ReadFloat",
         [](danet::BitStream &bs) -> std::optional<float> {
           float v;
           if (bs.Read(v)) {
             return {v};
           }
           return {};
         })
    .def("ReadDouble",
         [](danet::BitStream &bs) -> std::optional<double> {
           double v;
           if (bs.Read(v)) {
             return {v};
           }
           return {};
         })
    .def("ReadString",
         [](danet::BitStream &bs) -> std::optional<std::string> {
           std::string v{};
           if (bs.Read(v)) {
             return {v};
           }
           return {};
         })
    .def("ReadUleb16",
         [](danet::BitStream &bs) -> std::optional<uint16_t> {
           uint16_t v{};
           if (bs.ReadCompressed(v)) {
             return {v};
           }
           return {};
         })
    .def("ReadUleb32",
         [](danet::BitStream &bs) -> std::optional<uint32_t> {
           uint32_t v{};
           if (bs.ReadCompressed(v)) {
             return {v};
           }
           return {};
         })
    .def("ReadUleb64",
         [](danet::BitStream &bs) -> std::optional<uint64_t> {
           uint64_t v{};
           if (bs.ReadCompressed(v)) {
             return {v};
           }
           return {};
         })
    .def("ReadBool",
         [](danet::BitStream &bs) -> std::optional<bool> {
           bool v{};
           if (bs.Read(v)) {
             return {v};
           }
           return {};
         })

    .def("ReadBS",
         [](danet::BitStream &bs) -> std::optional<danet::BitStream> {
           danet::BitStream v{};
           if (bs.Read(v)) {
             return {v};
           }
           return {};
         })
    .def(
      "readBitsInt",
      [](danet::BitStream &bs, uint32_t bits) -> std::optional<uint64_t> {
        if (bits > 64) {
          throw py::index_error{};
        }
        uint64_t v;
        if (bs.Read(v)) {
          v >>= (64 - bits);
          return {v};
        }
        return {};
      },
      py::arg("bits"))
    .def("ReadBLK",
         [](danet::BitStream &bs) {
           DataBlock blk{};
           G_ASSERT(bs.Read(blk));
           return blk;
         })
    .def("IgnoreBits", &danet::BitStream::IgnoreBits)
    .def("IgnoreBytes", &danet::BitStream::IgnoreBytes)
    .def("GetReadOffset", &danet::BitStream::GetReadOffset)
    .def("SetReadOffset", &danet::BitStream::SetReadOffset, py::arg("offset"))
    .def("GetNumberOfBitsUsed", &danet::BitStream::GetNumberOfBitsUsed)
    .def("GetNumberOfUnreadBits", &danet::BitStream::GetNumberOfUnreadBits)
    .def("AlignReadToByteBoundary", &danet::BitStream::AlignReadToByteBoundary)
    .def("GetData", [](danet::BitStream &bs) {
      return py::memoryview::from_memory(bs.GetData(), (ssize_t) bs.GetNumberOfBytesUsed(), true);
    });
  m.def(
    "lz4_decompress",
    [](py::bytes &in_data, size_t &out_len) -> py::bytes {
      auto spn = bytes_to_span(in_data);
      std::string payload{};
      payload.resize(out_len);
      int readedSize = LZ4_decompress_safe(reinterpret_cast<const char *>(spn.data()), payload.data(), (int) spn.size(),
                                           (int) payload.size());
      if (readedSize < 0) {
        throw py::value_error(fmt::format("lz4 decompress fail error code: {}", readedSize));
      }
      payload.resize(readedSize);
      return {payload};
    },
    py::arg("compress_data"), py::arg("max_output_size"));
  m.def("BYTES_TO_BITS", [](BitSize_t in) { return BYTES_TO_BITS(in); });
  m.def("BITS_TO_BYTES", [](BitSize_t in) { return BITS_TO_BYTES(in); });


  py::class_<IdFieldSerializer255>(m, "IdFieldSerializer255")
    .def(py::init<>())
    .def(
      "readFieldsSizeAndCount",
      [](IdFieldSerializer255 &serializer255, danet::BitStream &bs) -> py::tuple {
        BitSize_t end;
        auto count = serializer255.readFieldsSizeAndCount(bs, end);
        return py::make_tuple(count, end);
      },
      py::arg("bs"))
    .def("readFieldsIndex", &IdFieldSerializer255::readFieldsIndex, py::arg("bs"))
    .def("skipReadingField", &IdFieldSerializer255::skipReadingField, py::arg("index"), py::arg("bs"))
    .def("getFieldId", &IdFieldSerializer255::getFieldId, py::arg("index"))
    .def("getFieldSize", &IdFieldSerializer255::getFieldSize, py::arg("index"));
}
