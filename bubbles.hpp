#include <vector>
#include <limits.h>
#include <vector>
#include "table.hpp"
#include <cstdlib>
#include <sys/types.h>
#include <random>
#include <climits>


static const int FREESPACE = 0;
static const int BOUNDARY = INT_MAX; 

enum WHICH_COLOR {COLOR, OLD, NEW};
struct Pot {

	int color = FREESPACE;

	size_t x = 0;
	size_t y = 0;
	int boundary_index = -1;
};

struct Coord {

	int x;
	int y;

	Coord(int x, int y);
};

struct Simulation {

	Pot* grid;
	int x_max;
	int y_max;
	int num_colors;
	std::vector<double> pressures;
	std::vector<int> volumes;
	std::vector<double> P0V0;
	std::vector<int> V0;  
	double T = 30.f;
	double surface_area = 0; 


	const double k_a = 1.3;
	const double k_p = 1.0; 

	std::mt19937 engine;


	struct Params {

		std::vector<Coord> bubbles;
		std::vector<double> radii;
		std::vector<int> colors;
		std::vector<double> pressures; 
		std::vector<double> targets;
	};

	double find_surface_area();
	Simulation(int x_max, int y_max, const Params& sim_params);
	~Simulation();



	std::vector<Coord> boundary;
	bool is_on_boundary(int x, int y);

	
	double local_sa_color(int x, int y, int color, int fx, int fy, int fcolor);
	double local_sa(int x, int y, int fx, int fy, int fcolor);
	double local_sa(int x, int y);

	void add_boundary(int x, int y);
	void remove_boundary(int x, int y);

	double roll01();
	void flip_cell();
	void run_sim_iteration();

	
};






