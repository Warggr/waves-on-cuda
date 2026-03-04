#include "vof.hpp"
#include <Eigen/SparseCore>
#include <Eigen/IterativeLinearSolvers>
#include <cassert>
#include <cmath>
#include <ostream>
#include <algorithm>

double rho(double volume_fraction) {
    return 0.01 + 1 * volume_fraction;
}

/*
Let's walk backwards.
- We want the pressure gradient on the cell faces.
- For this, we need the pressure on the cell centres.
- This means that also the pressure divergence $u \cdot \nabla u$ must be on the cell center.
- So we want the divergence $\nabla \cdot u_trans$ also on a centered grid
- i.e. we want u_trans on a staggered grid
- So, we want u_i u_j on an i-staggered grid, but *also* on a y-staggered grid... unless we use a centered scheme, in which case we can have it on the cell center
*/
template<template<typename> class allocator>
::Grid<Speed, ndim, allocator<Speed>> VOF<allocator>::compute_transport_velocity(const StaggeredGrid& before, _Grid<Speed> forces, std::array<double, 3> dx){
    const auto inner_grid_shape = before.volume_fraction.shape();
    const auto inner_grid_indices = before.volume_fraction.indices();
    assert(before.volume_fraction.shape() == forces.shape());
    ::Grid<double, ndim> uiuj[3] = {
        {inner_grid_shape},
        {inner_grid_shape},
        {inner_grid_shape},
    };
    for(const auto& [i, j, k]: inner_grid_indices) {
        double ui = (before.u[0][i][j][k] + before.u[0][i+1][j][k]) / 2;
        double uj = (before.u[1][i][j][k] + before.u[1][i][j+1][k]) / 2;
        double uk = (before.u[2][i][j][k] + before.u[2][i][j][k+1]) / 2;
        double ujuk = uj * uk,
               ujui = uj * ui,
               uiuk = ui * uk;
        uiuj[0][i][j][k] = ujui + uiuk;
        uiuj[1][i][j][k] = ujui + ujuk;
        uiuj[2][i][j][k] = ujuk + uiuk;
    }
    _Grid<Speed> u_trans(inner_grid_shape);
    for(const auto& idxs: inner_grid_indices){
        for(int dim = 0; dim < ndim; dim++){
            std::array<std::size_t, 3> plus = idxs, minus = idxs;
            int nbcells = 0;
            if(idxs[dim] == 0){
                u_trans[idxs][dim] = 0;
                continue;
            } else {
                minus[dim]--;
                nbcells++;
            }
            if(idxs[dim] == inner_grid_indices.shape[dim] - 1){
                u_trans[idxs][dim] = 0;
                continue;
            } else {
                plus[dim]++;
                nbcells++;
            }
            u_trans[idxs][dim] = (uiuj[0][plus] - uiuj[0][minus]) / (nbcells * dx[dim]) + forces[idxs][dim];
            assert(not std::isnan(u_trans[idxs][dim]));
        }
    }
    return u_trans;
}

template<template<typename> class allocator>
::Grid<double, ndim, allocator<double>> VOF<allocator>::compute_pressure(const _Grid<double>& volume_fraction, const _Grid<Speed>& u_trans, std::array<double, 3> dx) {
    _Grid<double> div_u(volume_fraction.shape());
    for(const auto& idxs: div_u.indices()){
        for(int dim = 0; dim < ndim; dim++){
            std::array<std::size_t, 3> plus = idxs, minus = idxs;
            int nbcells = 0;
            if(idxs[dim] != 0){
                minus[dim]--;
                nbcells++;
            }
            if(idxs[dim] != u_trans.shape()[dim] - 1){
                plus[dim]++;
                nbcells++;
            }
            div_u[idxs] += (u_trans[plus][dim] - u_trans[minus][dim]) / (nbcells * dx[dim]);
            assert(not std::isnan(div_u[idxs]));
        }
    }

    using namespace Eigen;

    SparseMatrix<double> A(div_u.size(), div_u.size());
    A.reserve(VectorXi::Constant(div_u.size(), ndim*2 + 1));
    for(const auto& idxs: div_u.indices()) {
        double total = 0.0;
        std::size_t c = div_u.idx_to_offset(idxs);
        for(int dim = 0; dim < ndim; dim++){
            if(idxs[dim] != 0){
                std::array<std::size_t, 3> minus = idxs;
                minus[dim]--;
                double rho_inv = (
                    1 / rho(volume_fraction[idxs]) +
                    1 / rho(volume_fraction[minus])
                ) * 0.5;
                total += rho_inv;
                A.insert(c, volume_fraction.idx_to_offset(minus)) = rho_inv;
            }
            if(idxs[dim] != volume_fraction.shape()[dim] - 1){
                std::array<std::size_t, 3> plus = idxs;
                plus[dim]++;
                double rho_inv = (
                    1 / rho(volume_fraction[idxs]) +
                    1 / rho(volume_fraction[plus])
                ) * 0.5;
                total += rho_inv;
                A.insert(c, volume_fraction.idx_to_offset(plus)) = rho_inv;
            }
        }
        A.insert(c, c) = -total;
    }

    ConjugateGradient<SparseMatrix<double>, Lower|Upper> cg;
    cg.compute(A);
    Map<VectorXd> rhs(div_u.data(), div_u.size());
    VectorXd pressure_eig = cg.solve(rhs);
    for(const auto& element: pressure_eig)
        assert(not std::isnan(element));

    _Grid<double> pressure(div_u.shape());
    Map<VectorXd> map(pressure.data(), pressure.size());
    map = std::move(pressure_eig);
    return pressure;
}

template<typename dtype>
std::ostream& operator<<(std::ostream& os, const std::array<dtype, 3>& vec) {
    os << '(';
    for(const auto& elem: vec) os << elem << ',';
    os << ')';
    return os;
}

template<template<typename> class allocator>
void VOF<allocator>::step(const StaggeredGrid& before, StaggeredGrid& after, double _t, double dt) const {
    std::array<double, 3> dx;
    for(int dim = 0; dim < 3; dim++) dx[dim] = 1.0 / before.volume_fraction.shape()[dim];
    _Grid<Speed> forces(before.volume_fraction.shape());
    const double cell_volume = std::reduce(dx.begin(), dx.end(), 1, std::multiplies<double>{});
    for(const auto& idx: forces.indices()) {
        forces[idx][2] = -9.81 * rho(before.volume_fraction[idx]);
    }

    auto u_trans = compute_transport_velocity(before, std::move(forces), dx);
    auto pressure = compute_pressure(before.volume_fraction, u_trans, dx);
    for(const auto& idxs: pressure.indices()){
        assert(not std::isnan(pressure[idxs]));
    }

    for(int dim = 0; dim < ndim; dim++){
        for(const auto& idxs: before.u[dim].indices()){
            if(idxs[dim] == 0 or idxs[dim] == before.u[dim].shape()[dim] - 1){
                // TODO: how to handle the boundary?
                after.u[dim][idxs] = 0;
            } else {
                std::array<std::size_t, 3> minus = idxs;
                minus[dim]--;
                after.u[dim][idxs] =
                    before.u[dim][idxs]
                    + dt * (pressure[minus] - pressure[idxs]) / 2 / (rho(before.volume_fraction[minus]) + rho(before.volume_fraction[idxs])) * 2
                    + dt * (u_trans[minus][dim] + u_trans[idxs][dim]) / 2
                ;
            }
        }
    }

    _Grid<Speed> normals(before.volume_fraction.shape());
    for(const auto& [i, j, k]: before.volume_fraction.indices()) {
        const auto cell_vf = before.volume_fraction[i][j][k];
        if(0 > cell_vf) {
            //assert(false);
            before.volume_fraction[i][j][k] = 0;
        }
        else if(1 < cell_vf) {
            //assert(false);
            before.volume_fraction[i][j][k] = 1.0;
        }
        else if(0 == cell_vf or 1 == cell_vf){
            continue;
        }
        /* Reconstruction of the line segment with Mixed Young Centered */
        std::array<double, 3> normal;
        for(int di = -1; di <= 1; di++){
            int di_pushed = di;
            if(i+di < 0 or i+di >= before.volume_fraction.shape()[0]) di_pushed = 0;
            for(int dj = -1; dj <= 1; dj++){
                int dj_pushed = dj;
                if(j+dj < 0 or j+dj >= before.volume_fraction.shape()[1]) dj_pushed = 0;
                for(int dk = -1; dk <= 1; dk++){
                    int dk_pushed = dk;
                    if(k+dk < 0 or k+dk >= before.volume_fraction.shape()[2]) dk_pushed = 0;
                    int diff = (di != 0) + (dj != 0) + (dk != 0);
                    int coeff = diff == 1 ? 4 : diff == 2 ? 2 : diff == 3 ? 1 : 0;
                    if(di == -1 or di == 1)
                        normal[0] += di * before.volume_fraction[i+di_pushed][j+dj_pushed][k+dk_pushed] * coeff;
                    if(dj == -1 or dj == 1)
                        normal[1] += dj * before.volume_fraction[i+di_pushed][j+dj_pushed][k+dk_pushed] * coeff;
                    if(dk == -1 or dk == 1)
                        normal[2] += dk * before.volume_fraction[i+di_pushed][j+dj_pushed][k+dk_pushed] * coeff;
                }
            }
        }

        double normal_norm = std::sqrt(
            std::pow(normal[0], 2) +
            std::pow(normal[1], 2) +
            std::pow(normal[2], 2)
        );
        for(int dim = 0; dim < ndim; dim++)
            normal[dim] /= normal_norm; 
        normals[i][j][k] = normal;
    }

    _Grid<std::array<float, ndim>> wall_sizes_early(before.volume_fraction.shape()),
        wall_sizes_late(before.volume_fraction.shape());
    for(const auto& idxs: before.volume_fraction.indices()) {
        /*
        +--8---+.
        |`7    | a.
        2  `+--+-b-+
        |   4  5   |
        +-1-+--+.  9
         `0 |    6.|
           `+-3----+
        */
        if(before.volume_fraction[idxs] >= 1.0) {
            for(int dim = 0; dim < ndim; dim++){
                wall_sizes_early[idxs][dim] = 1.0;
                wall_sizes_late[idxs][dim] = 1.0;
            }
        } else if(before.volume_fraction[idxs] <= 0.0) {
            for(int dim = 0; dim < ndim; dim++){
                wall_sizes_early[idxs][dim] = 0.0;
                wall_sizes_late[idxs][dim] = 0.0;
            }
        } else {
            double intersect[12];
            const std::array<double, ndim> n = {
                std::abs(normals[idxs][0]),
                std::abs(normals[idxs][1]),
                std::abs(normals[idxs][2])
            };
            std::array<float, ndim> wall_sizes_early_rot, wall_sizes_late_rot;
            const double alpha = before.volume_fraction[idxs] * (n[0] + n[1] + n[2]);
            if(n[0] >= alpha) {
                intersect[0] = alpha / n[0];
                intersect[3] = 0.0;
                intersect[4] = 0.0;
                intersect[9] = 0.0;
            } else {
                intersect[0] = 1.0;
                const double alpha_var = alpha - n[0];
                if(n[1] >= alpha_var) {
                    assert(n[1] != 0);
                    intersect[3] = alpha_var / n[1];
                    assert(not std::isnan(intersect[3]));
                    intersect[9] = 0.0;
                } else {
                    intersect[3] = 1.0;
                    intersect[9] = (alpha_var - n[1]) / n[2];
                }
                if(n[2] >= alpha_var) {
                    intersect[4] = alpha_var / n[2];
                } else {
                    intersect[4] = 1.0;
                }
            }
            if(n[1] >= alpha) {
                intersect[1] = alpha / n[1];
                intersect[5] = 0.0;
                intersect[6] = 0.0;
                intersect[10] = 0.0;
            } else {
                intersect[1] = 1.0;
                const double alpha_var = alpha - n[1];
                if(n[2] >= alpha_var) {
                    intersect[5] = alpha_var / n[2];
                    intersect[10] = 0.0;
                } else {
                    intersect[5] = 1.0;
                    intersect[10] = (alpha_var - n[2]) / n[0];
                }
                if(n[0] >= alpha_var) {
                    intersect[6] = alpha_var / n[0];
                } else {
                    intersect[6] = 1.0;
                }
            }
            if(n[2] >= alpha) {
                intersect[2] = alpha / n[2];
                intersect[7] = 0.0;
                intersect[8] = 0.0;
                intersect[11] = 0.0;
            } else {
                intersect[2] = 1.0;
                const double alpha_var = alpha - n[2];
                if(n[0] >= alpha_var) {
                    intersect[7] = alpha_var / n[0];
                } else {
                    intersect[7] = 1.0;
                    intersect[11] = (alpha_var - n[0]) / n[1];
                }
                if(n[1] >= alpha_var) {
                    intersect[8] = alpha_var / n[1];
                } else {
                    intersect[8] = 1.0;
                }
            }
            if(intersect[10] > 0) {
                wall_sizes_early_rot[0] = 1.0;
            } else {
                if(intersect[1] < 1.0 and intersect[2] < 1.0) {
                    wall_sizes_early_rot[0] = 0.5 * intersect[1] * intersect[2];
                } else if(intersect[1] >= 1.0 and intersect[2] < 1.0) {
                    wall_sizes_early_rot[0] = 0.5 * (intersect[5] + intersect[2]);
                } else if(intersect[1] < 1.0 and intersect[2] >= 1.0) {
                    wall_sizes_early_rot[0] = 0.5 * (intersect[1] + intersect[8]);
                } else {
                    wall_sizes_early_rot[0] = 1 - 0.5 * intersect[8] * intersect[5];
                }
            }
            if(intersect[0] < 1.0) {
                wall_sizes_late_rot[0] = 0.0;
            } else {
                if(intersect[3] < 1.0 and intersect[4] < 1.0) {
                    wall_sizes_late_rot[0] = 0.5 * intersect[3] * intersect[4];
                } else if(intersect[3] >= 1.0 and intersect[4] < 1.0) {
                    wall_sizes_late_rot[0] = 0.5 * (intersect[4] + intersect[9]);
                } else if(intersect[3] < 1.0 and intersect[4] >= 1.0) {
                    wall_sizes_late_rot[0] = 0.5 * (intersect[3] + intersect[11]);
                } else {
                    wall_sizes_late_rot[0] = 1 - 0.5 * intersect[9] * intersect[11];
                }
            }
            if(intersect[11] > 0) {
                wall_sizes_early_rot[1] = 1.0;
            } else {
                if(intersect[0] < 1.0 and intersect[2] < 1.0) {
                    wall_sizes_early_rot[1] = 0.5 * intersect[0] * intersect[2];
                } else if(intersect[0] >= 1.0 and intersect[2] < 1.0) {
                    wall_sizes_early_rot[1] = 0.5 * (intersect[4] + intersect[2]);
                } else if(intersect[0] < 1.0 and intersect[2] >= 1.0) {
                    wall_sizes_early_rot[1] = 0.5 * (intersect[0] + intersect[7]);
                } else {
                    wall_sizes_early_rot[1] = 1 - 0.5 * intersect[7] * intersect[4];
                }
            }
            if(intersect[1] < 1.0) {
                wall_sizes_late_rot[1] = 0.0;
            } else {
                if(intersect[5] < 1.0 and intersect[6] < 1.0) {
                    wall_sizes_late_rot[1] = 0.5 * intersect[5] * intersect[6];
                } else if(intersect[5] >= 1.0 and intersect[6] < 1.0) {
                    wall_sizes_late_rot[1] = 0.5 * (intersect[6] + intersect[10]);
                } else if(intersect[5] < 1.0 and intersect[6] >= 1.0) {
                    wall_sizes_late_rot[1] = 0.5 * (intersect[5] + intersect[9]);
                } else {
                    wall_sizes_late_rot[1] = 1 - 0.5 * intersect[9] * intersect[10];
                }
            }
            if(intersect[9] > 0) {
                wall_sizes_early_rot[2] = 1.0;
            } else {
                if(intersect[0] < 1.0 and intersect[1] < 1.0) {
                    wall_sizes_early_rot[2] = 0.5 * intersect[0] * intersect[1];
                } else if(intersect[0] >= 1.0 and intersect[1] < 1.0) {
                    wall_sizes_early_rot[2] = 0.5 * (intersect[3] + intersect[1]);
                } else if(intersect[0] < 1.0 and intersect[1] >= 1.0) {
                    wall_sizes_early_rot[2] = 0.5 * (intersect[0] + intersect[6]);
                } else {
                    wall_sizes_early_rot[2] = 1 - 0.5 * intersect[6] * intersect[3];
                }
            }
            if(intersect[2] < 1.0) {
                wall_sizes_late_rot[2] = 0.0;
            } else {
                if(intersect[7] < 1.0 and intersect[8] < 1.0) {
                    wall_sizes_late_rot[2] = 0.5 * intersect[7] * intersect[8];
                } else if(intersect[7] >= 1.0 and intersect[8] < 1.0) {
                    wall_sizes_late_rot[2] = 0.5 * (intersect[8] + intersect[11]);
                } else if(intersect[7] < 1.0 and intersect[8] >= 1.0) {
                    wall_sizes_late_rot[2] = 0.5 * (intersect[7] + intersect[10]);
                } else {
                    wall_sizes_late_rot[2] = 1 - 0.5 * intersect[11] * intersect[10];
                }
            }

            for(int dim = 0; dim < ndim; dim++){
                if(normals[idxs][0] > 0) {
                    wall_sizes_late[idxs][dim] = wall_sizes_late_rot[dim];
                    wall_sizes_early[idxs][dim] = wall_sizes_early_rot[dim];
                } else {
                    wall_sizes_late[idxs][dim] = wall_sizes_early_rot[dim];
                    wall_sizes_early[idxs][dim] = wall_sizes_late_rot[dim];
                }
                if(normals[idxs][dim] == 0)
                    assert(wall_sizes_early_rot[dim] == wall_sizes_late_rot[dim]);
                assert(not std::isnan(wall_sizes_early[idxs][dim]));
                assert(not std::isnan(wall_sizes_late[idxs][dim]));
            }
        }
    }

    _Grid<std::array<double, ndim>> advected_volume_early(before.volume_fraction.shape()),
        advected_volume_late(before.volume_fraction.shape());
    for(const auto& idxs: before.volume_fraction.indices()) {
        for(int dim = 0; dim < ndim; dim++){
            auto idxs_after = idxs;
            idxs_after[dim] += 1;
            /* Divergence from the Python code: we're not going to evaluate the 1st derivative of the wall size (dsize_x, dsize_y),
            because that sounds too hard */
            advected_volume_early[idxs][dim] = after.u[dim][idxs] * std::clamp(wall_sizes_early[idxs][dim], 0.0f, 1.0f);
            advected_volume_late[idxs][dim] = -after.u[dim][idxs_after] * std::clamp(wall_sizes_late[idxs][dim], 0.0f, 1.0f);
        }
    }

    StaggeredGrid advected_volume(before.volume_fraction.shape());
    for(int dim = 0; dim < ndim; dim++){
        for(const auto& idxs: before.u[dim].indices()) {
            // [ i-1 ]  -|-> u_i [ i ]
            //  late[i-1]|early[i]
            auto minus = idxs;
            minus[dim] -= 1;
            if(idxs[dim] == 0) {
                advected_volume.u[dim][idxs] = advected_volume_early[idxs][dim];
            } else if(idxs[dim] == before.u[dim].shape()[dim] - 1) {
                advected_volume.u[dim][idxs] = advected_volume_late[minus][dim];
            } else {
                if(after.u[dim][idxs] > 0) {
                    double max_pos = before.volume_fraction[minus] / (dt / dx[dim]);
                    advected_volume.u[dim][idxs] = std::min(advected_volume_late[minus][dim], max_pos);
                } else {
                    double max_neg = before.volume_fraction[idxs] / (dt / dx[dim]);
                    advected_volume.u[dim][idxs] = -std::min(advected_volume_early[idxs][dim], max_neg);
                }
            }
        }
    }

    // Idea of the dt / dx:
    // u * wall_size is how much volume would pass through a unit wall in one unit of time.
    // ("unit wall" justifies wall_size being in [0, 1]).
    // But I don't have a unit of time, I only have dt -> so only dt * u * wall_size passes through.
    // And I also don't have a unit wall, I have a dx * dy wall -> only dt * u * wall_size * dx * dy passes through.
    // But I don't want to fill a unit cell, I only want to fill a cell of size dxdydz -> dt * u * wall_size * dx * dy * dz means that the cell is filled.
    // So the "fill percentage" that passes through is dt * u * wall_size / dz.

    // Split scheme
    after.volume_fraction = before.volume_fraction;
    for(int dim = 0; dim < ndim; dim++){
        for(const auto& idxs: before.volume_fraction.indices()) {
            auto idxs_after = idxs;
            idxs_after[dim] += 1;
            after.volume_fraction[idxs] += (advected_volume.u[dim][idxs] - advected_volume.u[dim][idxs_after]) * (dt / dx[dim]);
            if(before.volume_fraction[idxs] >= 0.5) {
                after.volume_fraction[idxs] += (dt / dx[dim]) * (after.u[dim][idxs] - after.u[dim][idxs_after]);
            }
            assert(not std::isnan(after.volume_fraction[idxs]));
        }
    }
}

#ifdef NO_CUDA
template class VOF<std::allocator>;
#else
template class VOF<>;
#endif
