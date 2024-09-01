// MPMSimulation.cpp
#include "MPMSimulation.h"
#include <cmath>
#include <iostream>

void MPMSimulation::step() {
    grid.reset();
    particlesToGrid();
    updateGrid();
    gridToParticles();
}

void threadSafeVectorAdd(Eigen::Vector3f& target, const Eigen::Vector3f& value) {
#pragma omp critical
    {
        target += value;
    }
}
 
void MPMSimulation::particlesToGrid() {
    if (particles.empty()) {
        std::cerr << "Warning: No particles in the simulation." << std::endl;
        return;
    }

#pragma omp parallel for
    for (int i = 0; i < particles.size(); ++i) {
        Particle& p = particles[i];
        Eigen::Vector3f cellPos = p.position / grid.cellSize;
        Eigen::Vector3i baseCell(cellPos.cast<int>());

        // Clamp baseCell to grid boundaries
        baseCell = baseCell.cwiseMax(Eigen::Vector3i::Zero()).cwiseMin(grid.dimensions - Eigen::Vector3i::Ones());

        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                for (int z = 0; z < 2; ++z) {
                    Eigen::Vector3i cell = baseCell + Eigen::Vector3i(x, y, z);

                    // Ensure cell is within grid boundaries
                    if ((cell.array() < 0).any() || (cell.array() >= grid.dimensions.array()).any()) {
                        continue;
                    }

                    Eigen::Vector3f cellDist = (cellPos - cell.cast<float>()).cwiseAbs();
                    float w = (1 - cellDist.x()) * (1 - cellDist.y()) * (1 - cellDist.z());

                    int index = grid.getIndex(cell);

#pragma omp atomic
                    grid.masses[index] += w * p.mass;

#pragma omp critical
                    {
                        grid.velocities[index] += w * p.mass * p.velocity;
                    }
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
                    if (index >= 0 && index < grid.forces.size()) {
                        threadSafeVectorAdd(grid.forces[index], nodeForce);
                    } else {
                        // Log error or handle out-of-bounds access
                        std::cerr << "Error: Out.of bounds." << std::endl;
                    }
                }
            }
        }
    }
}

// Helper function for matrix logarithm
Eigen::Matrix3f matrixLogarithm(const Eigen::Matrix3f& F) {
    Eigen::JacobiSVD<Eigen::Matrix3f> svd(F, Eigen::ComputeFullU | Eigen::ComputeFullV);
    Eigen::Vector3f log_s = svd.singularValues().array().log();
    return svd.matrixU() * log_s.asDiagonal() * svd.matrixV().transpose();
}

Eigen::Matrix3f MPMSimulation::computeStress(const Particle& p) {
    Eigen::Matrix3f F = p.deformationGradient;
    Eigen::Matrix3f strain = 0.5f * (F.transpose() * F - Eigen::Matrix3f::Identity());
    float J = F.determinant();

    float lambda = p.material->youngsModulus * p.material->poissonRatio /
        ((1 + p.material->poissonRatio) * (1 - 2 * p.material->poissonRatio));
    float mu = p.material->youngsModulus / (2 * (1 + p.material->poissonRatio));

    switch (p.material->type) {
    case MaterialType::Elastic:
        return 2.0f * mu * strain + lambda * strain.trace() * Eigen::Matrix3f::Identity();

    case MaterialType::Snow: {
        float Je = std::max(J, 0.1f);
        Eigen::Matrix3f Fe = std::pow(Je, -1.0f / 3.0f) * F;
        Eigen::Matrix3f logFe = matrixLogarithm(Fe);
        Eigen::Matrix3f strainE = 0.5f * (logFe + logFe.transpose());

        float mu_c = mu * std::exp(p.material->criticalCompression * (1.0f - Je));
        float lambda_c = lambda * std::exp(p.material->criticalCompression * (1.0f - Je));

        return 2.0f * mu_c * strainE + lambda_c * strainE.trace() * Eigen::Matrix3f::Identity();
    }

    case MaterialType::Fluid: {
        float pressure = -lambda * (J - 1);
        return pressure * Eigen::Matrix3f::Identity();
    }

    default:
        return Eigen::Matrix3f::Zero();
    }
}