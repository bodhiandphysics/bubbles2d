#include "bubbles.hpp"
#include <iostream>

int main(int argc, char const *argv[])
{
	
	Simulation::Params params;
	int x_max = 100;
	int y_max = 100;
	double scale = 1.0;
	params.bubbles = {Coord(50, 50)};
	params.radii = {30.0, 30.0};
	params.colors = {0};
	params.pressures = {.5, 1.0};
	Simulation sim(x_max, y_max, scale, params);

	std::cout << sim.find_surface_area() << std::endl;
	for (int i = 0; i < 10; i++) {
		sim.run_sim_iteration();
		std::cout << sim.find_surface_area() << std::endl;
	}
	return 0;
}