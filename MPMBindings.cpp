// MPMBindings.cpp
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include "MPMSimulation.h"

namespace py = pybind11;

PYBIND11_MODULE(mpm_simulation, m) {
    py::class_<Particle>(m, "Particle")
        .def_readwrite("x", &Particle::x)
        .def_readwrite("y", &Particle::y)
        .def_readwrite("z", &Particle::z)
        .def_readwrite("vx", &Particle::vx)
        .def_readwrite("vy", &Particle::vy)
        .def_readwrite("vz", &Particle::vz)
        .def_readwrite("mass", &Particle::mass)
        .def_readwrite("emitter_id", &Particle::emitter_id);

    py::class_<MPMSimulation>(m, "Simulation")
        .def(py::init<int, double>())
        .def("set_attr", &MPMSimulation::set_attr)
        .def("add_particles", [](MPMSimulation& self, py::array_t<double> coordinates, int emitter_id) {
        py::buffer_info buf = coordinates.request();
        if (buf.ndim != 2 || buf.shape[1] != 3) {
            throw std::runtime_error("Input must be a Nx3 array");
        }
        std::vector<double> coords(buf.size);
        std::memcpy(coords.data(), buf.ptr, buf.size * sizeof(double));
        return self.add_particles(coords, emitter_id);
            })
        .def("simulate", &MPMSimulation::simulate)
        .def_property_readonly("particles", [](const MPMSimulation& self) {
        return py::make_iterator(self.get_particles().begin(), self.get_particles().end());
            }, py::keep_alive<0, 1>())
        .def("set_particles_attr", &MPMSimulation::set_particles_attr);
}