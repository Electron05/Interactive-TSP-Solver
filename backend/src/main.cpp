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
#include "tsp_solver.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
using json = nlohmann::json;


// I am new to websockets so...
// https://www.boost.org/doc/libs/1_82_0/libs/beast/example/websocket/server/async/websocket_server_async.cpp

void do_session(tcp::socket socket){
    try {
        // Create WebSocket stream from TCP socket
        websocket::stream<tcp::socket> ws{std::move(socket)};
        
        // Accept the WebSocket handshake
        ws.accept();
        
        // Buffer for reading messages (dynamic, but with size check)
        beast::multi_buffer buffer;
        
        // Loop to read and echo messages
        for (;;) {
            // Wait for read
            ws.read(buffer);
            
            // Log receiving
            std::string msg = beast::buffers_to_string(buffer.data());
            //std::cout << "Received: " << msg << std::endl;
            

            json query = json::parse(msg);
            std::vector<std::vector<float>> distanceMatrix = query["data"];
            float alpha = query["alpha"];
            float beta = query["beta"];
            float rho = query["rho"];
            
            std::vector<int> payload = solveTSP(distanceMatrix, alpha, beta, rho);

            // Return calculated path
            json answer;
            answer["type"] = "path1";
            answer["payload"] = payload;
            ws.write(net::buffer(answer.dump()));
            
            // Clear buffer for next message
            buffer.consume(buffer.size());
        }
    } catch (const beast::system_error& se) {
        if (se.code() != websocket::error::closed) {
            std::cerr << "Error: " << se.code().message() << std::endl;
        }
    }
}

int main(){
	// I/O context for async operations
    net::io_context ioc;

	// Acceptor to listen on port 8080
	tcp::acceptor acceptor{ioc, tcp::endpoint{tcp::v4(), 8080}};

	std::cout << "WebSocket server listening on port 8080..." << std::endl;
    unsigned int num_cores = std::thread::hardware_concurrency();
    std::cout << "Threads: " << std::to_string(num_cores) << std::endl;
	
    srand(time(0));  // Seed once here
    for(;;){
		tcp::socket socket{ioc};
		acceptor.accept(socket);

		// Handle the session in a new thread (basic concurrency)
		std::thread{do_session, std::move(socket)}.detach();
	}

	return 0;
}