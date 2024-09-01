// MPMSimulation.h
#pragma once
#include <vector>
#include <Eigen/Dense>
#include <Eigen/SVD>
#include <omp.h>

enum class MaterialType {
    Elastic,
    Snow,
    Fluid
};

struct Material {
    MaterialType type;
    float youngsModulus;
    float poissonRatio;
    float criticalCompression;
    float criticalStretch;
    float density;

    Material(MaterialType t, float E, float nu, float cComp, float cStretch, float rho)
        : type(t), youngsModulus(E), poissonRatio(nu),
        criticalCompression(cComp), criticalStretch(cStretch), density(rho) {}
};

struct Particle {
    Eigen::Vector3f position;
    Eigen::Vector3f velocity;
    Eigen::Matrix3f deformationGradient;
    float mass;
    float volume;
    const Material* material;

    Particle(const Eigen::Vector3f& pos, float m, const Material* mat)
        : position(pos), velocity(Eigen::Vector3f::Zero()),
        deformationGradient(Eigen::Matrix3f::Identity()),
        mass(m), volume(m / mat->density), material(mat) {}
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
    std::vector<Material> materials;
    Grid grid;
    float dt;
    Eigen::Vector3f gravity;

public:
    MPMSimulation(const Eigen::Vector3i& gridDimensions, float cellSize, float timeStep)
        : grid(gridDimensions, cellSize), dt(timeStep), gravity(0, -9.81f, 0) {
        initializeMaterials();
    }

    void addParticle(const Eigen::Vector3f& position, float mass, int materialIndex) {
        particles.emplace_back(position, mass, &materials[materialIndex]);
    }

    void step();
    const std::vector<Particle>& getParticles() const { return particles; }

private:
    void initializeMaterials() {
        materials.emplace_back(MaterialType::Elastic, 1e5f, 0.3f, 1e-2f, 1e-3f, 1000.0f);
        materials.emplace_back(MaterialType::Snow, 1.4e5f, 0.2f, 2.5e-2f, 7.5e-3f, 400.0f);
        materials.emplace_back(MaterialType::Fluid, 5e4f, 0.4f, 0.0f, 0.0f, 1000.0f);
    }

    void particlesToGrid();
    void updateGrid();
    void gridToParticles();
    Eigen::Matrix3f computeStress(const Particle& p);
};