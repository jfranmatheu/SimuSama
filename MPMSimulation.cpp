// MPMSimulation.cpp
#include "MPMSimulation.h"
#include <cstring>

MPMSimulation::MPMSimulation(int grid_size, double dt)
    : grid_size(grid_size), dt(dt), gravity(-9.81), rest_density(1000.0), dynamic_viscosity(0.001), particle_mass(1.0) {
    dx = 1.0 / grid_size;
    initialize_grid();
}

void MPMSimulation::initialize_grid() {
    grid.resize(grid_size * grid_size * grid_size);
}

void MPMSimulation::set_attr(const std::string& attribute_name, double attribute_value) {
    if (attribute_name == "gravity") {
        gravity = attribute_value;
    }
    else if (attribute_name == "rest_density") {
        rest_density = attribute_value;
    }
    else if (attribute_name == "dynamic_viscosity") {
        dynamic_viscosity = attribute_value;
    }
    else if (attribute_name == "particle_mass") {
        particle_mass = attribute_value;
    }
}

int MPMSimulation::add_particles(const std::vector<double>& coordinates, int emitter_id) {
    int start_index = particles.size();
    for (size_t i = 0; i < coordinates.size(); i += 3) {
        particles.push_back({ coordinates[i], coordinates[i + 1], coordinates[i + 2], 0, 0, 0, particle_mass, emitter_id });
    }
    return start_index;
}

void MPMSimulation::simulate(int frame) {
    for (int step = 0; step < frame; ++step) {
        initialize_grid();
        particle_to_grid();
        solve_incompressibility();
        apply_forces();
        grid_to_particle();
        update_particles();
    }
}

void MPMSimulation::set_particles_attr(int particle_start, int particle_count, const std::string& attribute, double value) {
    int end = std::min(particle_start + particle_count, static_cast<int>(particles.size()));
    for (int i = particle_start; i < end; ++i) {
        if (attribute == "mass") {
            particles[i].mass = value;
        }
    }
}

void MPMSimulation::particle_to_grid() {
    for (const auto& p : particles) {
        int i = static_cast<int>(p.x / dx);
        int j = static_cast<int>(p.y / dx);
        int k = static_cast<int>(p.z / dx);

        auto weights = get_weight(p.x / dx - i, p.y / dx - j, p.z / dx - k);

        for (int di = 0; di < 2; ++di) {
            for (int dj = 0; dj < 2; ++dj) {
                for (int dk = 0; dk < 2; ++dk) {
                    int index = get_cell_index(i + di, j + dj, k + dk);
                    double weight = weights[0] * (1 - di) + weights[1] * di *
                        weights[2] * (1 - dj) + weights[3] * dj *
                        weights[4] * (1 - dk) + weights[5] * dk;

                    grid[index].mass += weight * p.mass;
                    grid[index].vx += weight * p.mass * p.vx;
                    grid[index].vy += weight * p.mass * p.vy;
                    grid[index].vz += weight * p.mass * p.vz;
                }
            }
        }
    }

    for (auto& cell : grid) {
        if (cell.mass > 0) {
            cell.vx /= cell.mass;
            cell.vy /= cell.mass;
            cell.vz /= cell.mass;
        }
    }
}

void MPMSimulation::solve_incompressibility() {
    // Simple pressure solver (Jacobi iteration)
    std::vector<double> pressure(grid.size(), 0.0);
    const int iterations = 10;
    const double relaxation = 0.5;

    for (int iter = 0; iter < iterations; ++iter) {
        for (int i = 1; i < grid_size - 1; ++i) {
            for (int j = 1; j < grid_size - 1; ++j) {
                for (int k = 1; k < grid_size - 1; ++k) {
                    int index = get_cell_index(i, j, k);
                    if (grid[index].mass > 0) {
                        double div = (grid[get_cell_index(i + 1, j, k)].vx - grid[get_cell_index(i - 1, j, k)].vx) / (2 * dx) +
                            (grid[get_cell_index(i, j + 1, k)].vy - grid[get_cell_index(i, j - 1, k)].vy) / (2 * dx) +
                            (grid[get_cell_index(i, j, k + 1)].vz - grid[get_cell_index(i, j, k - 1)].vz) / (2 * dx);

                        pressure[index] = (1 - relaxation) * pressure[index] +
                            relaxation * (div * rest_density * dx / dt -
                                (pressure[get_cell_index(i + 1, j, k)] + pressure[get_cell_index(i - 1, j, k)] +
                                    pressure[get_cell_index(i, j + 1, k)] + pressure[get_cell_index(i, j - 1, k)] +
                                    pressure[get_cell_index(i, j, k + 1)] + pressure[get_cell_index(i, j, k - 1)]) / 6);
                    }
                }
            }
        }
    }

    // Apply pressure forces
    for (int i = 1; i < grid_size - 1; ++i) {
        for (int j = 1; j < grid_size - 1; ++j) {
            for (int k = 1; k < grid_size - 1; ++k) {
                int index = get_cell_index(i, j, k);
                if (grid[index].mass > 0) {
                    double px = (pressure[get_cell_index(i + 1, j, k)] - pressure[get_cell_index(i - 1, j, k)]) / (2 * dx);
                    double py = (pressure[get_cell_index(i, j + 1, k)] - pressure[get_cell_index(i, j - 1, k)]) / (2 * dx);
                    double pz = (pressure[get_cell_index(i, j, k + 1)] - pressure[get_cell_index(i, j, k - 1)]) / (2 * dx);

                    grid[index].vx -= dt * px / rest_density;
                    grid[index].vy -= dt * py / rest_density;
                    grid[index].vz -= dt * pz / rest_density;
                }
            }
        }
    }
}

void MPMSimulation::apply_forces() {
    for (auto& cell : grid) {
        if (cell.mass > 0) {
            cell.vy += gravity * dt;
        }
    }
}

void MPMSimulation::grid_to_particle() {
    for (auto& p : particles) {
        int i = static_cast<int>(p.x / dx);
        int j = static_cast<int>(p.y / dx);
        int k = static_cast<int>(p.z / dx);

        auto weights = get_weight(p.x / dx - i, p.y / dx - j, p.z / dx - k);

        double pic_vx = 0, pic_vy = 0, pic_vz = 0;
        double flip_vx = p.vx, flip_vy = p.vy, flip_vz = p.vz;

        for (int di = 0; di < 2; ++di) {
            for (int dj = 0; dj < 2; ++dj) {
                for (int dk = 0; dk < 2; ++dk) {
                    int index = get_cell_index(i + di, j + dj, k + dk);
                    double weight = weights[0] * (1 - di) + weights[1] * di *
                        weights[2] * (1 - dj) + weights[3] * dj *
                        weights[4] * (1 - dk) + weights[5] * dk;

                    pic_vx += weight * grid[index].vx;
                    pic_vy += weight * grid[index].vy;
                    pic_vz += weight * grid[index].vz;

                    flip_vx += weight * (grid[index].vx - grid[index].vx);
                    flip_vy += weight * (grid[index].vy - grid[index].vy);
                    flip_vz += weight * (grid[index].vz - grid[index].vz);
                }
            }
        }

        const double alpha = 0.95; // FLIP/PIC blending factor
        p.vx = alpha * flip_vx + (1 - alpha) * pic_vx;
        p.vy = alpha * flip_vy + (1 - alpha) * pic_vy;
        p.vz = alpha * flip_vz + (1 - alpha) * pic_vz;
    }
}

void MPMSimulation::update_particles() {
    for (auto& p : particles) {
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.z += p.vz * dt;

        // Simple boundary conditions
        if (p.x < 0 || p.x > 1) p.vx *= -0.5;
        if (p.y < 0 || p.y > 1) p.vy *= -0.5;
        if (p.z < 0 || p.z > 1) p.vz *= -0.5;

        p.x = std::max(0.0, std::min(1.0, p.x));
        p.y = std::max(0.0, std::min(1.0, p.y));
        p.z = std::max(0.0, std::min(1.0, p.z));
    }
}

int MPMSimulation::get_cell_index(int i, int j, int k) const {
    return i + j * grid_size + k * grid_size * grid_size;
}

std::array<double, 6> MPMSimulation::get_weight(double x, double y, double z) {
    return { 1 - x, x, 1 - y, y, 1 - z, z };
}