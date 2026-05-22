#include "tsp_solver.hpp"

#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <tuple>
#include <random>
#include <thread>

struct AntContext {
    std::vector<std::vector<float>>& distanceMatrix;
    std::vector<std::vector<float>>& pheromoneLevel; 
    float alpha; 
    float beta; 
    float rho;
};

struct Ant {
    float pathDistance = 0.0f; 
    std::vector<int> path; 
    std::vector<bool> visited;
    std::vector<std::vector<bool>> edgeUsed;
};

void createRoute (  Ant& ant, const AntContext& context, std::mt19937& gen){

	int n = context.distanceMatrix.size();
	ant.visited.assign(n, false);
	ant.edgeUsed.assign(n, std::vector<bool>(n, false));


	// Pick ranodm starting node
	std::uniform_int_distribution<int> startNodeDist(0, n - 1);
    int node = startNodeDist(gen);
	int startNode = node;
	ant.visited[node] = true;
	ant.path.push_back(node);

	std::uniform_real_distribution<float> dis;

	// Ant wil travel to n-1 other nodes
	for(int i = 0; i < n - 1; i++){
		float totalProb = 0.0f;
		std::vector<std::tuple<int,float>> travelProb; 
		for(int j = 0; j < n; j++){
			if(ant.visited[j]) continue;
			float currentProb = std::pow(context.pheromoneLevel[node][j],context.alpha) /
								std::pow(context.distanceMatrix[node][j],context.beta);
			totalProb += currentProb;
			travelProb.push_back(std::make_tuple(j,totalProb));
		}

		// Pick next node at weighted random
        dis.param(std::uniform_real_distribution<float>::param_type(0.0f, totalProb));
        float nextNodeRoll = dis(gen);

		for(std::tuple<int, float> travel : travelProb){
			if(nextNodeRoll > std::get<1>(travel)) continue; 
			int nextNode = std::get<0>(travel);
			ant.edgeUsed[node][nextNode] = true;
			ant.pathDistance += context.distanceMatrix[node][nextNode];

			node = nextNode;
			ant.visited[node] = true;
			ant.path.push_back(node);
			break;
		}
	}

	// Close the cycle
	ant.path.push_back(startNode);
	ant.pathDistance += context.distanceMatrix[node][startNode];
	ant.edgeUsed[node][startNode] = true;
}

void checkIfBest(const Ant& ant, float& bestDistance, const Ant*& bestAnt){
	if (ant.pathDistance < bestDistance) {
		bestDistance = ant.pathDistance;
		bestAnt = &ant;
	}
}

std::vector<int> solveTSP(int cores, std::vector<std::vector<float>> distanceMatrix, float alpha, float beta, float rho){
	float bestDistance = MAXFLOAT;
	
	std::vector<int> bestPath;

	// Initialize pheromone levels
	int n = distanceMatrix.size();
	std::vector<float> pheromoneRow(n, 1.0f);
	std::vector<std::vector<float>> pheromoneLevel(n, pheromoneRow);
	
	AntContext context{distanceMatrix,pheromoneLevel,alpha,beta,rho};

	std::random_device rd;
    std::vector<std::mt19937> generators;
    for(int i = 0; i < cores; i++) {
        generators.push_back(std::mt19937(rd()));
    }

	for(int iter = 0; iter < 1000; iter++){
		std::vector<Ant> ants(cores);
		std::vector<std::thread> threads;

		for(int i = 0; i < cores; i++){
			threads.push_back(std::thread(createRoute, std::ref(ants[i]),std::ref(context), std::ref(generators[i])));
		}

		for(int i = 0; i < cores; i++){
			threads[i].join();
		}


		const Ant* bestAnt = nullptr;
		for(int i = 0; i <cores; i++){
			checkIfBest(ants[i], bestDistance, bestAnt);
		}
		if (bestAnt != nullptr) {
            bestPath = bestAnt->path; 
        }

		// Update pheromone levels
		for(int i = 0; i < n; i++){
			for(int j = 0; j < n; j++){
				pheromoneLevel[i][j] *= (1.0f-rho);
			}
		}
		if (bestPath.empty()) continue;
		for(int i = 0; i < n; i++){
			int from = bestPath[i];
			int to = bestPath[i+1];
			pheromoneLevel[from][to] += 1/distanceMatrix[from][to];
		}
	}

	std::cout << "Best path: ";
	for(int p : bestPath){
		std::cout << p << " ";
	}
	std::cout << std::endl;
	std::cout << "Distance: " << bestDistance << std::endl;
	
	return bestPath;
}