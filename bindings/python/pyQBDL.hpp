#ifndef PY_QBDL_H_
#define PY_QBDL_H_

#include <pybind11/pybind11.h>
namespace py = pybind11;

using namespace pybind11::literals;

namespace QBDL {
void pyinit(py::module &);
void pyinit_engine(py::module &);
void pyinit_loaders(py::module &);
void pyinit_arch(py::module &);
} // namespace QBDL

#endif
