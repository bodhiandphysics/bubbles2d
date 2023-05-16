#include "bubbles.hpp"

Coord::Coord(int x, int y) : x(x), y(y) {}

bool Simulation::is_on_boundary(int x, int y) {
  if (x < 0 || x > x_max)
    return false;
  if (y < 0 || y > y_max)
    return false;
  int pot_color = grid[x + x_max * y].color;
  for (int i = x - 1; i <= x + 1; i++)
    for (int j = y - 1; j <= y + 1; j++) {

      if (i < 0 || i >= x_max)
        continue;
      if (j < 0 || j >= y_max)
        continue;

      int check_color = grid[i + x_max * j].color;

      if (check_color != pot_color)
        return true;
    }

  return false;
}

void Simulation::add_boundary(int x, int y) {

  if (grid[x + x_max * y].boundary_index >= 0)
    return;
  boundary.push_back(Coord(x, y));
  grid[x + y * x_max].boundary_index = boundary.size() - 1;
}

void Simulation::remove_boundary(int x, int y) {

  int index = grid[x + y * x_max].boundary_index;
  if (index < 0)
    return;
  auto &last_entry = boundary.back();
  boundary[index] = last_entry;
  grid[last_entry.x + x_max * last_entry.y].boundary_index = index;
  boundary.pop_back();
  grid[x + y * x_max].boundary_index = -1;
}

Simulation::Simulation(int x_max, int y_max, const Params &sim_params)
    : x_max(x_max), y_max(y_max) {

  std::random_device rd;

  engine = std::mt19937(rd());
  int num_bubbles = sim_params.bubbles.size();
  num_colors = num_bubbles + 1;
  volumes = std::vector<int>(num_colors);
  volumes[0] = x_max * y_max;
  pressures = std::vector<double>(num_colors);
  P0V0 = std::vector<double>(num_colors);

  grid = (Pot *)malloc(x_max * y_max * sizeof(Pot));

  for (int n = 0; n < num_bubbles; n++) {
    int rsphere = sim_params.radii[n];
    int rs2 = rsphere * rsphere;
    for (int x = 0; x < x_max; x++)
      for (int y = 0; y < y_max; y++) {

        auto &cell = grid[x + x_max * y];
        cell.x = x;
        cell.y = y;
        cell.boundary_index = -1;

        const Coord &bcoord = sim_params.bubbles[n];
        int dx = x - bcoord.x;
        int dy = y - bcoord.y;
        int r2 = dx * dx + dy * dy;

        if (r2 <= rs2) {
          volumes[grid[x + x_max * y].color]--;
          grid[x + x_max * y].color = n + 1;
          volumes[n + 1]++;
        }
      }
     
  }

  for (int n = 0; n < num_colors; n++) {
    V0.push_back(sim_params.targets[n]);
    pressures[n] = sim_params.pressures[n];
    P0V0[n] = pressures[n] * volumes[n];
  }

  // build the boundary;

  for (int x = 0; x < x_max; x++)
    for (int y = 0; y < y_max; y++) {

      if (is_on_boundary(x, y)) {
        boundary.push_back(Coord(x, y));
        grid[x + x_max * y].boundary_index = boundary.size() - 1;
      }
    }

  surface_area = find_surface_area();
}

Simulation::~Simulation() { free(grid); }

double Simulation::local_sa_color(int x, int y, int color, int fx, int fy,
                                  int fcolor) {

  unsigned char index = 0b0000;
  unsigned char mask = 1;

  for (int j = y; j <= y + 1; j++)
    for (int i = x; i <= x + 1; i++) {

      if (i < 0 || i >= x_max)
        continue;
      if (j < 0 || j >= y_max)
        continue;

      int test_color =
          (i == fx && j == fy) ? fcolor : grid[i + x_max * j].color;
      if (test_color == color)
        index ^= mask;

      mask = mask << 1;
    }

  return AREA_TABLE[index];
}

bool color_is_present(int *colors, int size, int color) {

  for (int i = 0; i < size; i++)
    if (colors[i] == color)
      return true;
  return false;
}

double Simulation::local_sa(int x, int y, int fx, int fy, int fcolor) {

  u_int64_t mask = 1;

  int local_colors[4];
  int num_local_colors = 0;

  for (int i = x; i <= x + 1; i++)
    for (int j = y; j <= y + 1; j++) {

      if (i < 0 || i >= x_max)
        continue;
      if (j < 0 || j >= y_max)
        continue;

      int pot_color = (i == fx && j == fy) ? fcolor : grid[i + x_max * j].color;
      if (!color_is_present(local_colors, num_local_colors, pot_color))
        local_colors[num_local_colors++] = pot_color;
    }

  double local_sa = 0;
  for (int i = 0; i < num_local_colors; i++) {
    //if (local_colors[i] == 0)
      //continue;
    local_sa += local_sa_color(x, y, local_colors[i], fx, fy, fcolor);
  }

  return local_sa;
}

double Simulation::local_sa(int x, int y) {

  return local_sa(x, y, -10, -10, -1);
}
double Simulation::find_surface_area() {

  double sa = 0;

  for (int i = 0; i < x_max; i++)
    for (int j = 0; j < y_max; j++)
      sa += local_sa(i, j);

  return sa;
}

double Simulation::roll01() {
  std::uniform_real_distribution<> dis(0.0, 1.0);
  return dis(engine);
}

double sign(double a) { return (a > 0) ? 1 : -1; }

void Simulation::flip_cell() {

  double aroll = roll01();

  Coord &cell = boundary[floor(aroll * boundary.size())];
  int x = cell.x;
  int y = cell.y;
  if (x < 0 || x >= x_max)
    return;
  if (y < 0 || y >= y_max)
    return;

  int old_color = grid[x + y * x_max].color;
  int local_colors[9];
  int num_local_colors = 0;
  double probs[9];
  double sum_probs = 0;

  for (int i = 0; i < 9; i++)
    local_colors[i] = INT_MAX;

  for (int i = x - 1; i <= x + 1; i++)
    for (int j = y - 1; j <= y + 1; j++) {

      if (i < 0 || i >= x_max)
        continue;
      if (j < 0 || j >= y_max)
        continue;

      int test_color = grid[i + x_max * j].color;
      if (!color_is_present(local_colors, num_local_colors, test_color))
        local_colors[num_local_colors++] = test_color;
    }

  long roll = floor(roll01() * num_local_colors);
  int new_color = local_colors[roll];
  double new_surface_area = surface_area;
  for (int i = x - 1; i <= x; i++)
    for (int j = y - 1; j <= y; j++) {

      new_surface_area -= local_sa(i, j);
      new_surface_area += local_sa(i, j, x, y, new_color);
    }
  double ep_old = 0;
  double ep_new = 0;
  for (int i = 1; i < num_colors; i++) {
    double ep_change_o = volumes[i] - V0[i];
    double ep_change_n = ep_change_o;
    if (i == new_color) ep_change_n++;
    if (i == old_color) ep_change_n--; 
    ep_old += pow(ep_change_o, 2);
    ep_new += pow(ep_change_n, 2);
  }
  double delta_ep = ep_new - ep_old;
  double delta_sa = new_surface_area - surface_area;

  double de = k_p * pow(delta_ep, 1) + k_a  * pow(delta_sa, 1);
  double prob = exp(-de / T);
  if (roll01() < prob) {

    grid[x + x_max * y].color = new_color;
    surface_area = new_surface_area;
    volumes[old_color]--;
    volumes[new_color]++;

    if (old_color != 0)
      pressures[old_color] = P0V0[old_color] / volumes[old_color];
    if (new_color != 0)
      pressures[new_color] = P0V0[new_color] / volumes[new_color];
    for (int i = x - 1; i <= x + 1; i++) {
      for (int j = y - 1; j <= y + 1; j++) {

        if (i < 0 || i >= x_max)
          continue;
        if (j < 0 || j >= y_max)
          continue;
        if (!is_on_boundary(i, j))
          remove_boundary(i, j);
        else
          add_boundary(i, j);
      }
    }
  }

  T *= .9999995;
}

void Simulation::run_sim_iteration() {

  for (int i = 0; i < 100000; i++)
    flip_cell();
}
