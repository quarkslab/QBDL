#include "pyQBDL.hpp"

#include <LIEF/MachO.hpp>

#include "QBDL/Engine.hpp"
#include "QBDL/Loader.hpp"
#include "QBDL/arch.hpp"
#include "QBDL/engines/Native.hpp"
#include "QBDL/loaders/MachO.hpp"
#include "QBDL/loaders/ELF.hpp"

#include <pybind11/functional.h>
#include <pybind11/operators.h>

#include <stdexcept>

using namespace pybind11::literals; // used for named arguments ("args"_a)

namespace QBDL {

namespace {

struct PyTargetMemory: public TargetMemory
{
  uint64_t mmap(uint64_t ptr, size_t len) override {
    PYBIND11_OVERRIDE_PURE(
      uint64_t,
      TargetMemory,
      mmap,
      ptr, len);
  }

  bool mprotect(uint64_t ptr, size_t len, int flags) override {
    PYBIND11_OVERRIDE_PURE(
      bool,
      TargetMemory,
      mprotect,
      ptr, len, flags);
  }

  void write(uint64_t dst, const void* buf, size_t len) override {
    auto view = py::memoryview::from_buffer(reinterpret_cast<uint8_t const*>(buf),
        { static_cast<ssize_t>(len) }, { 1 });
    PYBIND11_OVERRIDE_PURE(
      void,
      TargetMemory,
      write,
      dst, view);
  }

  void read(void* out, uint64_t src, size_t len) override {
    pybind11::gil_scoped_acquire gil;
    pybind11::function pyfunc = pybind11::get_override(this, "read");
    if (!pyfunc) {
      throw std::runtime_error{"TargetMemory: unimplemented read method!"};
    }
    py::bytes ret = pyfunc(src, len);
    char* retbuf; ssize_t retlen = 0;
    PYBIND11_BYTES_AS_STRING_AND_SIZE(ret.ptr(), &retbuf, &retlen);
    if (retlen != len) {
      throw std::range_error{"invalid read length"};
    }
    memcpy(out, retbuf, len);
  }
};

struct PyTargetSystem: public TargetSystem
{
  PyTargetSystem(TargetMemory& mem):
    TargetSystem(mem)
  { }

  uint64_t symlink(Loader& loader, LIEF::Symbol const& sym) override {
    PYBIND11_OVERRIDE_PURE(
      uint64_t,
      TargetSystem,
      symlink,
      &loader, &sym);
  }

  bool supports(LIEF::Binary const& bin) override {
    PYBIND11_OVERRIDE_PURE(
      bool,
      TargetSystem,
      supports,
      &bin);
  }

  uint64_t base_address_hint(uint64_t binary_base_address, uint64_t virtual_size) override {
    PYBIND11_OVERRIDE_PURE(
      uint64_t,
      TargetSystem,
      base_address_hint,
      binary_base_address, virtual_size);
  }
};

struct PyNativeTargetSystem: public Engines::Native::TargetSystem {
  PyNativeTargetSystem(TargetMemory& mem):
    TargetSystem(mem)
  { }

  uint64_t symlink(Loader& loader, LIEF::Symbol const& sym) override {
    PYBIND11_OVERRIDE_PURE(
      uint64_t,
      TargetSystem,
      symlink,
      &loader, &sym);
  }
};

} // anonymous


PYBIND11_MODULE(pyqbdl, qbdl_module) {

  qbdl_module.doc() = R"pbdoc(
        Main Interface
        --------------
        .. currentmodule:: pyqbdl

        ``pyqbdl`` exposes Python bindings over QBDL.
    )pbdoc";
  pyinit_arch(qbdl_module);
  pyinit_engine(qbdl_module);
  pyinit_loaders(qbdl_module);
}

void pyinit(py::module &m) {}

void pyinit_arch(py::module &m) {
  py::class_<Arch>(m, "Arch")
      .def(py::init<LIEF::ARCHITECTURES, LIEF::ENDIANNESS, bool>(),
          R"pbdoc(
          Create an Arch object from its architecture, endianness, and whether it is 64 bits

          .. code-block:: python

            arm64_v8a = Arch(lief.ARCHITECTURES.ARM64, lief.ENDIANNESS.LITTLE, True)

          )pbdoc",
          "arch"_a, "endianness"_a, "is_64"_a)
      .def_static("from_bin", &Arch::from_bin,
                  "Construct an Arch object from a LIEF binary object",
                  "bin"_a,
                  py::return_value_policy::take_ownership)
      .def_readwrite("arch", &Arch::arch, "LIEF `Architecture <https://lief.quarkslab.com/doc/stable/api/python/abstract.html#architectures>`_.")
      .def_readwrite("endianness", &Arch::endianness,
          R"pbdoc(
          Endianness associated with the underlying architecture:

          * lief.ENDIANNESS.LITTLE
          * lief.ENDIANNESS.BIG

          )pbdoc")
      .def_readwrite("is64", &Arch::is64, "True if the architecture is based on 64 bits")
      .def(py::self == py::self)
      .def(py::self != py::self)
  ;
}

void pyinit_engine(py::module &m) {
  py::class_<TargetMemory, PyTargetMemory>(m, "TargetMemory", "Memory model of the targeted system")
    .def(py::init<>())
    .def("mmap", &TargetMemory::mmap,
        "Function used by the loaders to allocate memory pages",
        "ptr"_a, "len"_a)
    .def("mprotect", &TargetMemory::mprotect,
        "Function used by the loaders to change permissions on a memory area",
        "addr"_a, "len"_a, "prot"_a)
    .def("write", &TargetMemory::write,
        "Function used by the loader to write data in memory")
    .def("read", &TargetMemory::read,
        "Function used by the loaders to read data from memory")
    ;

  py::class_<TargetSystem, PyTargetSystem>(m, "TargetSystem")
    .def(py::init<TargetMemory&>(), py::keep_alive<1,2>())
    .def("symlink", &TargetSystem::symlink,
        R"pbdoc(
          Callback used by the loader to resolve external functions.

          The first parameter of this callback is the :class:`~.Loader` and
          the second parameter is the LIEF's symbol object to resolve (``lief.Symbol``).

          This callback must return an address (`int`) that will be bound
          to the symbol.

          .. code-block:: python

            def resolve(loader: pyqbdl.Loader, symbol: lief.Symbol):
                if symbol.name == "printf":
                    return printf_address
                return default_address
        )pbdoc")

    .def("supports", &TargetSystem::supports,
        "Function that returns whether we support the architecture associated with the given binary",
        "binary"_a)

    .def("base_address_hint", &TargetSystem::base_address_hint,
        R"pbdoc(
        Function that returns the preferred based address where the binary should be mapped.
        If it returns 0, the loader can choose any address.
        )pbdoc" ,
        "binary_base_address"_a, "virtual_size"_a)
    ;

  py::module_ engines = m.def_submodule("engines");
  engines.doc() = R"pbdoc(
      Engines
      -------

      .. currentmodule:: pyqbdl.engines

      Default engines provided by QBDL

      .. automodule:: pyqbdl.engines.Native
         :members:
         :undoc-members:

      )pbdoc";
  py::module_ native = engines.def_submodule("Native");
  native.doc() = R"pbdoc(
      Native
      ------

      .. currentmodule:: pyqbdl.engines.Native

      Native engine that provides that matches the OS on which QBDL is running
      )pbdoc";


  native.def("memory", &Engines::Native::memory,
      R"pbdoc(
        Return a native implementation of the memory accesses.

        This model assumes that the loader and the loaded binary share
        the same memory space.
      )pbdoc");
  native.def("arch", &Engines::Native::arch,
      ":class:`~.Arch` object that matches the system on which QBDL is running on.");

  py::class_<Engines::Native::TargetSystem, QBDL::TargetSystem, PyNativeTargetSystem>(native, "TargetSystem")
    .def(py::init<TargetMemory&>(), py::keep_alive<1,2>())
    .def("symlink", &Engines::Native::TargetSystem::symlink,
        "See :meth:`pyqbdl.TargetSystem.symlink`")
    ;
}

void pyinit_loaders(py::module &m) {
  class PyLoader : public Loader {
  public:
    uint64_t get_address(const std::string &sym) const override {
      PYBIND11_OVERRIDE_PURE(uint64_t, Loader, get_address, sym);
    }

    uint64_t get_address(uint64_t offset) const override {
      PYBIND11_OVERRIDE_PURE(uint64_t, Loader, get_address, offset);
    }

    uint64_t entrypoint() const override {
      PYBIND11_OVERRIDE_PURE(uint64_t, Loader, entrypoint, );
    }
    ~PyLoader() override = default;
  };

  py::class_<Loader, PyLoader> pyloader(m, "Loader", "Base class for all format loaders. See: :mod:`~pyqbdl.loaders`");
  py::enum_<Loader::BIND>(pyloader, "BIND", "Enum used to tweak the symbol binding mechanism")
      .value("DEFAULT", Loader::BIND::DEFAULT, "The loader chooses the binding method by itself")
      .value("NOT_BIND", Loader::BIND::NOT_BIND, "Do not bind symbol at all")
      .value("NOW", Loader::BIND::NOW, "Bind all the symbols while loading the binary")
      .value("LAZY", Loader::BIND::LAZY, "Bind symbols when they are used (i.e lazily)");

  pyloader
      .def("get_address", py::overload_cast<const std::string &>(
                              &Loader::get_address, py::const_),
          "Return the absolute address associated with the symbol given in parameter",
          "sym_name"_a)
      .def("get_address",
           py::overload_cast<uint64_t>(&Loader::get_address, py::const_),
           "Get the absolute address form the offset given in parameter",
           "offset"_a)
      .def_property_readonly("entrypoint", &Loader::entrypoint,
          "Binary entrypoint as an **absolute** address");

  py::module_ loaders = m.def_submodule("loaders");
  loaders.doc() = R"pbdoc(
        Loaders
        -------

        .. currentmodule:: pyqbdl.loaders

        This module contains the different classes used to load binary formats.

    )pbdoc";

  py::class_<Loaders::MachO, Loader>(loaders, "MachO", "Mach-O loader")
      //.def_static("from_binary", &Loaders::MachO::from_binary,
      //    py::return_value_policy::take_ownership)

      .def_static("from_file", &Loaders::MachO::from_file,
          "Load a Mach-O file from its path on the disk",
          "path"_a, "arch"_a, "engine"_a, "binding"_a = Loader::BIND::DEFAULT,
          py::keep_alive<0, 2>())
      .def_static("take_arch_binary", &Loaders::MachO::take_arch_binary,
          "Extract a Mach-O binary from a Fat binary that matches the given architecture",
          "fatbin"_a, "arch"_a)
      .def("is_valid", &Loaders::MachO::is_valid,
          "Whether the loader is in a consistent state");

  py::class_<Loaders::ELF, Loader>(loaders, "ELF", "ELF loader")
    .def_static("from_file", &Loaders::ELF::from_file,
        "Load an ELF file from its path on the disk",
        "bin_path"_a, "engines"_a, "bind"_a = Loader::BIND::DEFAULT,
        py::keep_alive<0, 2>())
    .def("is_valid", &Loaders::ELF::is_valid,
        "Whether the loader object is consistent");

  py::class_<Loaders::PE, Loader>(loaders, "PE", "PE loader")
      .def_static("from_file", &Loaders::PE::from_file,
                  "Load an PE file from its path on the disk", "bin_path"_a,
                  "engines"_a, "bind"_a = Loader::BIND::DEFAULT,
                  py::keep_alive<0, 2>())
      .def("is_valid", &Loaders::PE::is_valid,
           "Whether the loader object is consistent");
}

} // namespace QBDL
