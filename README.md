# Parallel TSP Solver with Ant Colony Optimization + Live Visualization  
(Work in progress – planning & learning phase)

This is going to be my side project where I want to properly understand parallel programming in C++ while building something visually cool: a Traveling Salesman Problem solver using Ant Colony Optimization (ACO), with real-time visualization in the browser.

The rough idea:  
- Heavy parallel ACO computation in C++ using raw `std::thread`  
- Angular frontend showing cities, pheromone trails and the best path updating live  
- WebSocket connection so you can literally watch the ants converge

## What I Want to Learn

This project is mostly about forcing myself to understand these things hands-on:

- How to actually manage threads manually in modern C++ (and understand the idea for most programming languages)
- Using `std::mutex` to protect shared state
- Coordinating threads between iterations with `std::condition_variable` or barriers
- Spotting and fixing race conditions the hard way
- Sending reasonably efficient live updates over WebSockets without killing performance
- Rendering hundreds/thousands of pheromone-weighted edges smoothly on HTML5 Canvas
- Handling real-time data streams properly in Angular with RxJS
- Putting a full frontend + backend app into Docker Compose for easy deployment

## Planned Features

- Click-to-add cities on an interactive canvas (with drag & zoom)
- Live pheromone heatmap — edges should get brighter where ants are walking more
- Real-time best path overlay that updates every few iterations
- Adjustable sliders for α (pheromone influence) and β (heuristic influence)
- Start / pause / reset buttons
- Maybe later: Add different biomes to map impacting distances between cities

## Tech I Plan to Use

- **Backend** → C++20
- **Parallelism** → `std::thread`
- **Frontend** → Angular (latest) + Canvas 2D context  
- **Communication** → native WebSocket
- **Dev/Deployment** → Docker Compose

## Core Algorithm I Want to Implement

Classic ACO for TSP:

Each ant builds a tour by choosing next city j from current i with probability:

$$P_{ij} = \frac{(\tau_{ij}^\alpha)(\eta_{ij}^\beta)}{\sum (\tau_{ik}^\alpha)(\eta_{ik}^\beta)}$$

Where:
* **$\tau_{ij}$**: Pheromone intensity on the edge between city $i$ and $j$.
* **$\eta_{ij}$**: Visibility (the inverse of distance $1/d_{ij}$).
* **$\alpha$**: A parameter to control the influence of $\tau_{ij}$ (history).
* **$\beta$**: A parameter to control the influence of $\eta_{ij}$ (heuristic/distance).

After all ants finish:
- evaporate pheromone globally (multiply by (1-ρ))
- deposit new pheromone on better paths

I want to run many ants in parallel threads, synchronize at the end of each cycle, update pheromones, and push best-so-far + (maybe sampled) pheromone state to the frontend.

## Notes

This README is basically my brain dump + motivation document.

If anyone wants to follow along or yell at my bad design decisions later — feel free to star/watch/fork whatever.
Wish me luck!
