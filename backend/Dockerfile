FROM gcc:latest

RUN apt-get update && apt-get install -y cmake

WORKDIR /app

COPY . .

RUN mkdir build && cd build && cmake .. && make

CMD ["./build/solver"]