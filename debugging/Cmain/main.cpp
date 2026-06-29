#define PY_SSIZE_T_CLEAN

#include <cstdio>
#include <Python.h>
#include <iostream>
#include "BitStream.h"

typedef struct {
  PyObject_HEAD BitStream *bs;
} PyBitStream;

static PyObject *BitStream_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  PyBitStream *self;
  self = (PyBitStream *) type->tp_alloc(type, 0);
  if (self != nullptr) {
    // self->bs = nullptr;
  }
  return (PyObject *) self;
}

static int BitStream_init(PyBitStream *self, PyObject *args, PyObject *kwds) {
  const char *buffer;
  Py_ssize_t length = 0;

  if (!PyArg_ParseTuple(args, "y#", &buffer, &length))
    return -1;
  self->bs = new BitStream(buffer, length, true);

  return 0;
}

static void BitStream_dealloc(PyBitStream *self) {
  // delete self->bs;
  Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *BitStream_ReadBits(PyBitStream *self, PyObject *args) {
  int bits = 0;
  if (!PyArg_ParseTuple(args, "i", &bits))
    Py_RETURN_NONE;

  int bytes = (bits + 7) >> 3;
  auto *outbuff = (uint8_t *) malloc(bytes);
  if (!self->bs->ReadBits(outbuff, bits)) {
    Py_RETURN_NONE;
  }
  return PyBytes_FromStringAndSize((const char *) outbuff, bytes);
}

static PyMethodDef BitStream_methods[] = {{"ReadBits", (PyCFunction) BitStream_ReadBits, METH_VARARGS, "Read Bits"},
                                          {nullptr, nullptr, 0, nullptr}};

static PyTypeObject BitStreamType = {
  PyVarObject_HEAD_INIT(NULL, 0) "BitStream.MyClass", // tp_name
  sizeof(PyBitStream), // tp_basicsize
  0, // tp_itemsize
  (destructor) BitStream_dealloc, // tp_dealloc
  0, // tp_vectorcall_offset
  0, // tp_getattr
  0, // tp_setattr
  0, // tp_as_async
  0, // tp_repr
  0, // tp_as_number
  0, // tp_as_sequence
  0, // tp_as_mapping
  0, // tp_hash
  0, // tp_call
  0, // tp_str
  0, // tp_getattro
  0, // tp_setattro
  0, // tp_as_buffer
  Py_TPFLAGS_DEFAULT, // tp_flags
  "MyClass objects", // tp_doc
  0, // tp_traverse
  0, // tp_clear
  0, // tp_richcompare
  0, // tp_weaklistoffset
  0, // tp_iter
  0, // tp_iternext
  BitStream_methods, // tp_methods
  0, // tp_members
  0, // tp_getset
  0, // tp_base
  nullptr, // tp_dict
  0, // tp_descr_get
  0, // tp_descr_set
  0, // tp_dictoffset
  (initproc) BitStream_init, // tp_init
  0, // tp_alloc
  BitStream_new, // tp_new
};


static PyModuleDef WtFileUtilsmodule = {
  PyModuleDef_HEAD_INIT,
  .m_name = "WtFileUtils",
  .m_doc = "Example module with a custom class",
  .m_size = -1,
};


extern "C" PyMODINIT_FUNC PyInit_WtFileUtils(void) {

  if (PyType_Ready(&BitStreamType) < 0)
    return NULL;


  PyObject *m = PyModule_Create(&WtFileUtilsmodule);
  if (m == NULL)
    return NULL;

  Py_INCREF(&BitStreamType);
  PyModule_AddObject(m, "BitStream", (PyObject *) &BitStreamType);
  return m;
}