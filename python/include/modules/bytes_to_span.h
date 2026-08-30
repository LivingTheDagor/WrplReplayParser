#pragma once
inline std::span<uint8_t> bytes_to_span(const py::bytes& py_bytes) {
  // Extract data pointer and size from py::bytes
  char* data;
  ssize_t size;
  // This will not copy, just gets access to internals
  PYBIND11_BYTES_AS_STRING_AND_SIZE(py_bytes.ptr(), &data, &size);

  // Convert to std::span<std::byte>
  return {reinterpret_cast<uint8_t*>(data), static_cast<size_t>(size)};
}

inline std::span<uint8_t> bytes_to_span(const py::bytearray& py_bytes) {
  // Extract data pointer and size from py::bytes

  py::buffer buf = py::reinterpret_borrow<py::buffer>(py_bytes);
  py::buffer_info info = buf.request();
  char* ptr = static_cast<char*>(info.ptr);
  ssize_t len = info.size; // total number of bytes

  // Convert to std::span<std::byte>
  return {reinterpret_cast<uint8_t*>(ptr), static_cast<size_t>(len)};
}
