#include "tsp_solver.hpp"

#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <tuple>
#include <random>

std::vector<int> solveTSP(std::vector<std::vector<float>> distanceMatrix, float alpha, float beta, float rho){
	std::vector<int> bestPath;
	float bestDistance = MAXFLOAT;

	// Initialize pheromone levels
	int n = distanceMatrix.size();
	std::vector<float> pheromoneRow(n, 1.0f);
	std::vector<std::vector<float>> pheromoneLevel(n, pheromoneRow);
	
	// Let's simulate 1 ant, 1000 iterations
	for(int iter = 0; iter < 1000; iter++){
		std::vector<int> path;
		std::vector<bool> visited(n, false);
		std::vector<std::vector<bool>> edgeUsed(n, visited);
		float pathDistance =0.0;

		// Pick ranodm starting node
		int node = rand()%n;
		int startNode = node;
		visited[node] = true;
		path.push_back(node);

		std::random_device rd;
		srand(rd());  // Use random_device for better entropy

		// We wil travel to n-1 other nodes
		for(int i = 0; i < n - 1; i++){
			float totalProb = 0.0f;
			std::vector<std::tuple<int,float>> travelProb; 
			for(int j = 0; j < n; j++){
				if(visited[j]) continue;
				float currentProb = std::pow(pheromoneLevel[node][j],alpha)*1/std::pow(distanceMatrix[node][j],beta);
				totalProb += currentProb;
				travelProb.push_back(std::make_tuple(j,totalProb));
			}

			// Let's roll for next node
			float nextNodeRoll = (float)rand() / RAND_MAX * totalProb;

			for(std::tuple<int, float> travel : travelProb){
				if(nextNodeRoll > std::get<1>(travel)) continue; 
				int nextNode = std::get<0>(travel);
				edgeUsed[node][nextNode] = true;
				pathDistance += distanceMatrix[node][nextNode];
				node = nextNode;
				visited[node] = true;
				path.push_back(node);
				break;
			}
		}
		
		path.push_back(startNode);
		pathDistance += distanceMatrix[node][startNode];
		edgeUsed[node][startNode] = true;

		// Update pheromone levels
		for(int i = 0; i < n; i++){
			for(int j = 0; j < n; j++){
				pheromoneLevel[i][j] *= (1.0f-rho);
				if(edgeUsed[i][j]){
					pheromoneLevel[i][j] += 1/distanceMatrix[i][j];
				}
			}
		}
		//Check if path is the best
		if(pathDistance < bestDistance){
			bestDistance = pathDistance;
			bestPath = path;
		}

	}

	// Add cout logs after every iteration
	std::cout << "Best path: ";
	for(int p : bestPath){
		std::cout << p << " ";
	}
	std::cout << std::endl;
	std::cout << "Distance: " << bestDistance << std::endl;
	
	return bestPath;
}