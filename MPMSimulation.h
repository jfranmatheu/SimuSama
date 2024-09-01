// MPMSimulation.h
#pragma once
#include <vector>
#include <Eigen/Dense>
#include <omp.h>

struct Particle {
    Eigen::Vector3f position;
    Eigen::Vector3f velocity;
    Eigen::Matrix3f deformationGradient;
    float mass;
    float volume;
    int materialType;

    Particle(const Eigen::Vector3f& pos, float m, int type)
        : position(pos), velocity(Eigen::Vector3f::Zero()),
        deformationGradient(Eigen::Matrix3f::Identity()),
        mass(m), volume(1.0f), materialType(type) {}
};

class Grid {
public:
    Eigen::Vector3i dimensions;
    float cellSize;
    std::vector<Eigen::Vector3f> velocities;
    std::vector<Eigen::Vector3f> forces;
    std::vector<float> masses;

    Grid(const Eigen::Vector3i& dims, float size)
        : dimensions(dims), cellSize(size) {
        int totalCells = dims.x() * dims.y() * dims.z();
        velocities.resize(totalCells, Eigen::Vector3f::Zero());
        forces.resize(totalCells, Eigen::Vector3f::Zero());
        masses.resize(totalCells, 0.0f);
    }

    void reset() {
        std::fill(velocities.begin(), velocities.end(), Eigen::Vector3f::Zero());
        std::fill(forces.begin(), forces.end(), Eigen::Vector3f::Zero());
        std::fill(masses.begin(), masses.end(), 0.0f);
    }

    int getIndex(const Eigen::Vector3i& cellPos) const {
        return cellPos.x() + cellPos.y() * dimensions.x() + cellPos.z() * dimensions.x() * dimensions.y();
    }
};

class MPMSimulation {
private:
    std::vector<Particle> particles;
    Grid grid;
    float dt;
    Eigen::Vector3f gravity;

public:
    MPMSimulation(const Eigen::Vector3i& gridDimensions, float cellSize, float timeStep)
        : grid(gridDimensions, cellSize), dt(timeStep), gravity(0, -9.81f, 0) {}

    void addParticle(const Eigen::Vector3f& position, float mass, int materialType) {
        particles.emplace_back(position, mass, materialType);
    }

    void step();

private:
    void particlesToGrid();
    void updateGrid();
    void gridToParticles();
    Eigen::Matrix3f computeStress(const Particle& p);
};