#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/stl.h>
#include "MPMSimulation.h"

namespace py = pybind11;

PYBIND11_MODULE(mpm_simulation, m) {
    py::class_<Grid>(m, "Grid")
        .def(py::init<const Eigen::Vector3i&, float>())
        .def_readonly("dimensions", &Grid::dimensions)
        .def_readonly("cell_size", &Grid::cellSize);

    py::enum_<MaterialType>(m, "MaterialType")
        .value("ELASTIC", MaterialType::Elastic)
        .value("SNOW", MaterialType::Snow)
        .value("FLUID", MaterialType::Fluid);

    py::class_<Material>(m, "Material")
        .def(py::init<MaterialType, float, float, float, float, float>())
        .def_readonly("type", &Material::type)
        .def_readonly("youngs_modulus", &Material::youngsModulus)
        .def_readonly("poisson_ratio", &Material::poissonRatio)
        .def_readonly("critical_compression", &Material::criticalCompression)
        .def_readonly("critical_stretch", &Material::criticalStretch)
        .def_readonly("density", &Material::density);

    py::class_<Particle>(m, "Particle")
        .def(py::init<const Eigen::Vector3f&, float, const Material*>())
        .def_readonly("position", &Particle::position)
        .def_readonly("velocity", &Particle::velocity)
        .def_readonly("mass", &Particle::mass)
        .def_readonly("volume", &Particle::volume)
        .def_readonly("material", &Particle::material);

    py::class_<MPMSimulation>(m, "MPMSimulation")
        .def(py::init<const Eigen::Vector3i&, float, float>())
        .def("add_particle", &MPMSimulation::addParticle)
        .def("step", &MPMSimulation::step)
        .def("get_particles", [](const MPMSimulation& sim) {
        const std::vector<Particle>& particles = sim.getParticles();
        return py::list(py::cast(particles));
            });
}