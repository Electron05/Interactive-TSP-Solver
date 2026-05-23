#include "tsp_solver.hpp"

#include <iostream>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include <memory>
#include <thread>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <mutex>
#include <condition_variable>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
using json = nlohmann::json;


std::mutex tspMutex;
std::condition_variable tspCV;

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
};

struct ParallelContext {
	int cores;
	int antsFinished = 0;
	int generation = 0;
	float bestDistance = MAXFLOAT;
	const Ant* bestAnt = nullptr;
	std::vector<int> bestPath;
	std::vector<Ant>* allAnts = nullptr;
};

void createRoute (  Ant& ant, const AntContext& context, std::mt19937& gen){

	int n = context.distanceMatrix.size();
	ant.visited.assign(n, false);


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
}

void updatePheromones(AntContext& context, ParallelContext& pc) {
    int n = context.distanceMatrix.size();

    for(int i = 0; i < n; i++) {
        for(int j = 0; j < n; j++) {
            context.pheromoneLevel[i][j] *= (1.0f - context.rho);
            
            // Prevent pheromones from hitting absolute 0.0f
            if (context.pheromoneLevel[i][j] < 0.0001f) {
                context.pheromoneLevel[i][j] = 0.0001f;
            }
        }
    }

    // ALL ants from the current generation deposit pheromones
    for (const Ant& ant : *(pc.allAnts)) {
        for(int i = 0; i < n; i++) {
            int from = ant.path[i];
            int to = ant.path[i+1];
            context.pheromoneLevel[from][to] += 100.0f / ant.pathDistance;
        }
    }

    // 3. Give an extra "elitist" boost to the global best path
    for(int i = 0; i < n; i++) {
        int from = pc.bestPath[i];
        int to = pc.bestPath[i+1];
        context.pheromoneLevel[from][to] += 100.0f / pc.bestDistance;
    }
}

void antColonyWorker(websocket::stream<tcp::socket>& ws, int iterations, Ant& ant, AntContext& context, ParallelContext& pc, std::mt19937& gen) {
    for(int i = 0; i < iterations; i++) {

		ant.pathDistance = 0.0f;
		ant.path.clear();

        createRoute(ant,context,gen);
		
        {
            std::unique_lock<std::mutex> lock(tspMutex);
            
            if (ant.pathDistance < pc.bestDistance) { 
				std::cout << "Distance: " << ant.pathDistance << "\n";
				json answer;
				answer["type"] = "path1";
				answer["payload"] = pc.bestPath;
				ws.write(net::buffer(answer.dump()));
				pc.bestDistance = ant.pathDistance;
				pc.bestAnt = &ant;
			}
            
            pc.antsFinished++;
            
            if (pc.antsFinished == pc.cores) {
                // JESTEM OSTATNI:
				if(pc.bestAnt != nullptr)
					pc.bestPath = pc.bestAnt->path;

				updatePheromones(context,pc);

				pc.bestAnt = nullptr;
				pc.antsFinished = 0;
				pc.generation++;
				tspCV.notify_all();
            } else {
				int currentGen = pc.generation;
                // NIE JESTEM OSTATNI:
				tspCV.wait(lock, [&pc, currentGen] { return pc.generation != currentGen; });
            }
        }
    }
}

void solveTSP(websocket::stream<tcp::socket>& ws, int cores, std::vector<std::vector<float>> distanceMatrix, float alpha, float beta, float rho){

	int n = distanceMatrix.size();
	std::vector<float> pheromoneRow(n, 1.0f);
	std::vector<std::vector<float>> pheromoneLevel(n, pheromoneRow);
	
	AntContext context{distanceMatrix,pheromoneLevel,alpha,beta,rho};

	std::random_device rd;
    std::vector<std::mt19937> generators;
    for(int i = 0; i < cores; i++) {
        generators.push_back(std::mt19937(rd()));
    }

	std::vector<int> bestPathTmp;
	ParallelContext parallel{cores};

	std::vector<Ant> ants(cores);
	std::vector<std::thread> threads;
	parallel.allAnts = &ants;

	for(int i = 0; i < cores; i++){
		threads.push_back(std::thread(antColonyWorker, std::ref(ws), 10000, std::ref(ants[i]),std::ref(context), std::ref(parallel),std::ref(generators[i])));
	}
	for(int i = 0; i < cores; i++){
		threads[i].join();
	}

	std::cout << "Best path: ";
	for(int p : parallel.bestPath){
		std::cout << p << " ";
	}
	std::cout << std::endl;
	std::cout << "Distance: " << parallel.bestDistance << std::endl;
	
	// Return calculated path
	json answer;
	answer["type"] = "path1";
	answer["payload"] = parallel.bestPath;
	ws.write(net::buffer(answer.dump()));

	return;
}