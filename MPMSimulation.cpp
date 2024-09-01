// MPMSimulation.cpp
#include "MPMSimulation.h"

void MPMSimulation::step() {
    grid.reset();
    particlesToGrid();
    updateGrid();
    gridToParticles();
}

void MPMSimulation::particlesToGrid() {
#pragma omp parallel for
    for (int i = 0; i < particles.size(); ++i) {
        Particle& p = particles[i];
        Eigen::Vector3f cellPos = p.position / grid.cellSize;
        Eigen::Vector3i baseCell(cellPos.cast<int>());

        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                for (int z = 0; z < 2; ++z) {
                    Eigen::Vector3i cell = baseCell + Eigen::Vector3i(x, y, z);
                    Eigen::Vector3f cellDist = (cellPos - cell.cast<float>()).cwiseAbs();
                    float w = (1 - cellDist.x()) * (1 - cellDist.y()) * (1 - cellDist.z());

                    int index = grid.getIndex(cell);
#pragma omp atomic
                    grid.masses[index] += w * p.mass;
#pragma omp atomic
                    grid.velocities[index] += w * p.mass * p.velocity;
                }
            }
        }
    }

#pragma omp parallel for
    for (int i = 0; i < grid.velocities.size(); ++i) {
        if (grid.masses[i] > 0) {
            grid.velocities[i] /= grid.masses[i];
        }
    }
}

void MPMSimulation::updateGrid() {
#pragma omp parallel for
    for (int i = 0; i < grid.velocities.size(); ++i) {
        if (grid.masses[i] > 0) {
            grid.velocities[i] += dt * (grid.forces[i] / grid.masses[i] + gravity);
        }
    }

    // Simple boundary conditions
    for (int x = 0; x < grid.dimensions.x(); ++x) {
        for (int y = 0; y < grid.dimensions.y(); ++y) {
            for (int z = 0; z < grid.dimensions.z(); ++z) {
                int index = grid.getIndex(Eigen::Vector3i(x, y, z));
                if (y == 0) grid.velocities[index].y() = std::max(0.0f, grid.velocities[index].y());
                if (x == 0 || x == grid.dimensions.x() - 1) grid.velocities[index].x() = 0;
                if (z == 0 || z == grid.dimensions.z() - 1) grid.velocities[index].z() = 0;
            }
        }
    }
}

void MPMSimulation::gridToParticles() {
#pragma omp parallel for
    for (int i = 0; i < particles.size(); ++i) {
        Particle& p = particles[i];
        Eigen::Vector3f cellPos = p.position / grid.cellSize;
        Eigen::Vector3i baseCell(cellPos.cast<int>());

        Eigen::Vector3f velocityUpdate = Eigen::Vector3f::Zero();
        Eigen::Matrix3f velocityGradient = Eigen::Matrix3f::Zero();

        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                for (int z = 0; z < 2; ++z) {
                    Eigen::Vector3i cell = baseCell + Eigen::Vector3i(x, y, z);
                    Eigen::Vector3f cellDist = (cellPos - cell.cast<float>()).cwiseAbs();
                    float w = (1 - cellDist.x()) * (1 - cellDist.y()) * (1 - cellDist.z());

                    int index = grid.getIndex(cell);
                    velocityUpdate += w * grid.velocities[index];

                    Eigen::Vector3f dw = Eigen::Vector3f(
                        x ? 1 : -1, y ? 1 : -1, z ? 1 : -1
                    ).cwiseProduct(Eigen::Vector3f(
                        1 - cellDist.y() * (1 - cellDist.z()),
                        1 - cellDist.x() * (1 - cellDist.z()),
                        1 - cellDist.x() * (1 - cellDist.y())
                    )) / grid.cellSize;

                    velocityGradient += grid.velocities[index] * dw.transpose();
                }
            }
        }

        p.velocity = velocityUpdate;
        p.position += dt * p.velocity;
        p.deformationGradient = (Eigen::Matrix3f::Identity() + dt * velocityGradient) * p.deformationGradient;

        Eigen::Matrix3f stress = computeStress(p);
        Eigen::Matrix3f force = -p.volume * stress * p.deformationGradient.transpose();

        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                for (int z = 0; z < 2; ++z) {
                    Eigen::Vector3i cell = baseCell + Eigen::Vector3i(x, y, z);
                    Eigen::Vector3f cellDist = (cellPos - cell.cast<float>()).cwiseAbs();
                    float w = (1 - cellDist.x()) * (1 - cellDist.y()) * (1 - cellDist.z());

                    Eigen::Vector3f dw = Eigen::Vector3f(
                        x ? 1 : -1, y ? 1 : -1, z ? 1 : -1
                    ).cwiseProduct(Eigen::Vector3f(
                        1 - cellDist.y() * (1 - cellDist.z()),
                        1 - cellDist.x() * (1 - cellDist.z()),
                        1 - cellDist.x() * (1 - cellDist.y())
                    )) / grid.cellSize;

                    int index = grid.getIndex(cell);
                    Eigen::Vector3f nodeForce = force * dw;
#pragma omp atomic
                    grid.forces[index] += nodeForce;
                }
            }
        }
    }
}

Eigen::Matrix3f MPMSimulation::computeStress(const Particle& p) {
    // Implement material model here (e.g., Neo-Hookean, snow, sand)
    // This is a simplified elastic model
    float lambda = 10.0f;  // Lame's first parameter
    float mu = 5.0f;       // Shear modulus

    Eigen::Matrix3f F = p.deformationGradient;
    Eigen::Matrix3f E = 0.5f * (F.transpose() * F - Eigen::Matrix3f::Identity());
    return 2.0f * mu * E + lambda * E.trace() * Eigen::Matrix3f::Identity();
}