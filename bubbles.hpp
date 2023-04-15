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
	int old_color = FREESPACE;
	int new_color = FREESPACE;
	size_t x = 0;
	size_t y = 0;
	bool on_boundary = false;
	bool on_bboundary = false;

	void set_old_color(int acolor) {old_color = acolor;}
	void set_new_color(int acolor) {new_color = acolor;}
	void set_color(int acolor) {color = acolor;}
	void init_colors(int acolor) {
		set_old_color(acolor);
		set_new_color(acolor);
		set_color(acolor);
	}

	int get_color(WHICH_COLOR which) {

			if (which == COLOR) return color;
			if (which == OLD) return old_color;
			if (which == NEW) return new_color;
			else return color;
		}

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

	const double k_a = 1.0;
	const double k_p = 1.0; 

	std::mt19937 engine;


	struct Params {

		std::vector<Coord> bubbles;
		std::vector<double> radii;
		std::vector<int> colors;
		std::vector<double> pressures; 
	};

	double find_surface_area(WHICH_COLOR which = COLOR);
	Simulation(int x_max, int y_max, const Params& sim_params);
	~Simulation();



	Coord* boundary;
	int boundary_size = 0;
	Coord* bboundary;
	int bboundary_size = 0;
	bool is_on_boundary(int x, int y);
	bool is_on_bboundary(int x, int y);

	
	double local_sa_color(int x, int y, int color, int fx, int fy, int fcolor, WHICH_COLOR which);
	double local_sa(int x, int y, int fx, int fy, int fcolor, WHICH_COLOR which);
	double local_sa(int x, int y, WHICH_COLOR which);

	double roll01();
	void flip_cell(int x, int y);
	void rebuild(); 
	void run_sim_iteration();

	
};






