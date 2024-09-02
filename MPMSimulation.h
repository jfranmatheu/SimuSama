#pragma once
#include <vector>
#include <array>
#include <string>
#include <cmath>
#include <algorithm>

struct Particle {
    double x, y, z;
    double vx, vy, vz;
    double mass;
    int emitter_id;
};

struct Cell {
    double mass;
    double vx, vy, vz;
    double ax, ay, az;
};

class MPMSimulation {
public:
    MPMSimulation(int grid_size, double dt);
    void set_attr(const std::string& attribute_name, double attribute_value);
    int add_particles(const std::vector<double>& coordinates, int emitter_id);
    void simulate(int frame);
    const std::vector<Particle>& get_particles() const { return particles; }
    void set_particles_attr(int particle_start, int particle_count, const std::string& attribute, double value);

private:
    std::vector<Particle> particles;
    std::vector<Cell> grid;
    int grid_size;
    double dx;
    double dt;
    double gravity;
    double rest_density;
    double dynamic_viscosity;
    double particle_mass;

    void initialize_grid();
    void particle_to_grid();
    void solve_incompressibility();
    void apply_forces();
    void grid_to_particle();
    void update_particles();

    std::array<double, 6> get_weight(double x, double y, double z); // Updated return type
    int get_cell_index(int i, int j, int k) const;
};