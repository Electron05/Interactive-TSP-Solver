#ifndef TSP_SOLVER_HPP
#define TSP_SOLVER_HPP

#include <vector>

std::vector<int> solveTSP(int cores, std::vector<std::vector<float>> distanceMatrix, float alpha, float beta, float rho);

#endif