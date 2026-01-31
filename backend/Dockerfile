FROM gcc:latest

RUN apt-get update && apt-get install -y cmake libboost-system-dev libboost-thread-dev nlohmann-json3-dev
WORKDIR /app

COPY . .

RUN mkdir build && cd build && cmake .. && make

CMD ["./build/solver"]