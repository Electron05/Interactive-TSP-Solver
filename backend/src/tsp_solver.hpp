#ifndef TSP_SOLVER_HPP
#define TSP_SOLVER_HPP

#include <vector>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>

void solveTSP(  boost::beast::websocket::stream<boost::asio::ip::tcp::socket>& ws, 
				int cores, 
				int iterations,
				std::vector<std::vector<float>> distanceMatrix, 
				float alpha, 
				float beta, 
				float rho);

#endif