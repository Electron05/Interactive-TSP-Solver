#include "tsp_solver.hpp"

#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <tuple>
#include <random>
#include <thread>
#include <mutex>

std::mutex tspMutex;

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

void createRoute (  Ant& ant, const AntContext& context){

	int n = context.distanceMatrix.size();

	// Pick ranodm starting node
	int node = rand()%n;
	int startNode = node;
	ant.visited[node] = true;
	ant.path.push_back(node);

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
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> dis(0.0f, totalProb);
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

void checkIfBest(const Ant& ant, float& bestDistance, std::vector<int>& bestPathCandidate){
	std::lock_guard<std::mutex> lock(tspMutex);
	if (ant.pathDistance < bestDistance) {
		bestDistance = ant.pathDistance;
		bestPathCandidate = ant.path;
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


	// 1 ant, 1000 iterations
	for(int iter = 0; iter < 1000; iter++){
		std::vector<Ant> ants(cores);
		std::vector<std::thread> threads;

		for(int i = 0; i < cores; i++){
			threads.push_back(std::thread(createRoute, std::ref(ants[i]),std::ref(context)));
		}

		for(int i = 0; i < cores; i++){
			threads[i].join();
		}

		std::vector<int>& bestPathCandidate = bestPath;
		int bestAntIndex = -1;
		for(int i = 0; i <cores; i++){
			checkIfBest(ants[i], bestDistance, bestPathCandidate);
		}
		bestPath = bestPathCandidate;
		// Update pheromone levels
		for(int i = 0; i < n; i++){
			for(int j = 0; j < n; j++){
				pheromoneLevel[i][j] *= (1.0f-rho);
			}
		}
		for(int i = 0; i < n-1; i++){
			int from = bestPath[i];
			int to = bestPath[i+1];
			pheromoneLevel[from][to] += 1/distanceMatrix[from][to];
		}
		bestPath = std::vector<int>(bestPath);
	}

	std::cout << "Best path: ";
	for(int p : bestPath){
		std::cout << p << " ";
	}
	std::cout << std::endl;
	std::cout << "Distance: " << bestDistance << std::endl;
	
	return bestPath;
}