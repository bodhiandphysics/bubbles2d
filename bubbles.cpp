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

bool Simulation::is_on_bboundary(int x, int y) {
  if (x > x_max)
    return false;
  if (y > y_max)
    return false;
  if (x >= 0 && y >= 0 && grid[x + x_max * y].on_boundary)
    return false;

  for (int i = x; i <= x + 1; i++)
    for (int j = y; j <= y + 1; j++) {

      if (i < 0 || i >= x_max)
        continue;
      if (j < 0 || j >= y_max)
        continue;
      if (grid[i + j * x_max].on_boundary)
        return true;
    }

  return false;
}

Simulation::Simulation(int x_max, int y_max, const Params &sim_params)
    : x_max(x_max), y_max(y_max) {

  std::random_device rd;

  engine = std::mt19937(rd());
  int num_bubbles = sim_params.bubbles.size();
  int num_colors = num_bubbles + 1;
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

        const Coord &bcoord = sim_params.bubbles[n];
        int dx = x - bcoord.x;
        int dy = y - bcoord.y;
        int r2 = dx * dx + dy * dy;

        if (r2 <= rs2) {
          volumes[grid[x + x_max * y].color]--;
          grid[x + x_max * y].init_colors(n + 1);
          volumes[n + 1]++;
        }
      }
  }

  for (int n = 0; n < num_colors; n++) {

    pressures[n] = sim_params.pressures[n];
    P0V0[n] = pressures[n] * volumes[n];
  }

  // build the boundary;

  std::vector<Coord> first_boundary;

  for (int x = 0; x < x_max; x++)
    for (int y = 0; y < y_max; y++) {

      if (is_on_boundary(x, y)) {
        first_boundary.push_back(Coord(x, y));
        grid[x + x_max * y].on_boundary = true;
      }
    }

  boundary_size = first_boundary.size();
  boundary = (Coord *)malloc(boundary_size * sizeof(Coord));
  for (int i = 0; i < boundary_size; i++)
    boundary[i] = first_boundary[i];

  // now build the first bboundary

  std::vector<Coord> first_bboundary;
  for (int x = -1; x < x_max; x++)
    for (int y = -1; y < y_max; y++) {

      if (is_on_bboundary(x, y)) {
        first_bboundary.push_back(Coord(x, y));
        if (x >= 0 && y >= 0)
        grid[x + x_max * y].on_bboundary = true;
      }
  }

  bboundary_size = first_bboundary.size();
  bboundary = (Coord *)malloc(bboundary_size * sizeof(Coord));
}

Simulation::~Simulation() {
  free(grid);
  free(boundary);
  free(bboundary);
}

double Simulation::local_sa_color(int x, int y, int color, int fx, int fy,
                                  int fcolor, WHICH_COLOR which) {

  unsigned char index = 0b0000;
  unsigned char mask = 1;

  for (int j = y; j <= y + 1; j++)
    for (int i = x; i <= x + 1; i++) {

      if (i < 0 || i >= x_max)
        continue;
      if (j < 0 || j >= y_max)
        continue;

      int test_color =
          (i == fx && j == fy) ? fcolor : grid[i + x_max * j].get_color(which);
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

double Simulation::local_sa(int x, int y, int fx, int fy, int fcolor,
                            WHICH_COLOR which) {

  u_int64_t mask = 1;

  int local_colors[4];
  int num_local_colors = 0;

  for (int i = x; i <= x + 1; i++)
    for (int j = y; j <= y + 1; j++) {

      if (i < 0 || i >= x_max)
        continue;
      if (j < 0 || j >= y_max)
        continue;

      int pot_color =
          (i == fx && j == fy) ? fcolor : grid[i + x_max * j].get_color(which);
      if (!color_is_present(local_colors, num_local_colors, pot_color))
        local_colors[num_local_colors++] = pot_color;
    }

  double local_sa = 0;
  for (int i = 0; i < num_local_colors; i++) {
    if (local_colors[i] == 0)
      continue;
    local_sa += local_sa_color(x, y, local_colors[i], fx, fy, fcolor, which);
  }

  return local_sa;
}

double Simulation::local_sa(int x, int y, WHICH_COLOR which) {

  return local_sa(x, y, -10, -10, -1, which);
}
double Simulation::find_surface_area(WHICH_COLOR which) {

  double sa = 0;

  for (int i = 0; i < boundary_size; i++)
    sa += local_sa(boundary[i].x, boundary[i].y, which);

  for (int i = 0; i < bboundary_size; i++)
    sa += local_sa(bboundary[i].x, bboundary[i].y, which);

  return sa;
}

double Simulation::roll01() {
  std::uniform_real_distribution<> dis(0.0, 1.0);
  return dis(engine);
}

void Simulation::flip_cell(int x, int y) {

  if (x < 0 || x >= x_max)
    return;
  if (y < 0 || y >= y_max)
    return;

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

      int test_color = grid[i + x_max * j].old_color;
      if (!color_is_present(local_colors, num_local_colors, test_color))
        local_colors[num_local_colors++] = test_color;
    }

  for (int cnum = 0; cnum < num_local_colors; cnum++) {

    double old_sa = 0;
    double new_sa = 0;
    for (int i = x - 1; i <= x; i++)
      for (int j = y - 1; j <= y; j++) {
        old_sa += local_sa(i, j, -1, -1, -1, OLD);
        new_sa += local_sa(i, j, x, y, cnum, OLD);
      }

    double dsa = new_sa - old_sa;
    double oldp = pressures[grid[x + y * x_max].get_color(OLD)];
    double newp = pressures[cnum];
    double dp = oldp - newp;

    double de = k_a * dsa + k_p * dp;

    double likelihood = exp(-de);
    probs[cnum] = likelihood;
    sum_probs += likelihood;
  }

  for (int cnum = 0; cnum < num_local_colors; cnum++)
    probs[cnum] /= sum_probs;

  double aggprob = 0;
  int flip_index = 0;
  double roll = roll01();
  while (flip_index <= num_local_colors) {
    aggprob += probs[flip_index];
    if (roll <= aggprob)
      break;
    flip_index++;
  }

  grid[x + x_max * y].set_new_color(local_colors[flip_index]);
}

void Simulation::rebuild() {

  std::vector<Coord> new_boundary;

  std::vector<int> newv(volumes.size());
  for (int i = 0; i < volumes.size(); i++)
    newv[i] = volumes[i];

  for (int i = 0; i < boundary_size; i++) {

    Coord cellcoords = boundary[i];
    auto *cell = &grid[cellcoords.x + x_max * cellcoords.y];
    cell->on_boundary = false;
    if (cell->get_color(COLOR) != cell->get_color(OLD)) {
      newv[cell->get_color(COLOR)]--;
      newv[cell->get_color(OLD)]++;
    }
    cell->init_colors(cell->get_color(OLD));
  }

  for (int i = 0; i < boundary_size; i++) {

    if (is_on_boundary(boundary[i].x, boundary[i].y)) {
      new_boundary.push_back(boundary[i]);
      grid[boundary[i].x + x_max * boundary[i].y].on_boundary = true;
    }
  }

  for (int i = 0; i < bboundary_size; i++) {
    if (bboundary[i].x >= 0 && bboundary[i].y >= 0)
      grid[bboundary[i].x + x_max * bboundary[i].y].on_bboundary = false;
    if (is_on_boundary(bboundary[i].x, bboundary[i].y)) {
      new_boundary.push_back(bboundary[i]);
      grid[bboundary[i].x + x_max * bboundary[i].y].on_boundary = true;
    }
  }

  std::vector<bool> already_placed_bboundary_i(x_max, false);
  std::vector<bool> already_placed_bboundary_j(y_max, false);
  std::vector<Coord> new_bboundary;
  for (int n = 0; n < new_boundary.size(); n++) {

    auto cell = new_boundary[n];
    for (int i = cell.x - 1; i < cell.x; i++)
      for (int j = cell.y - 1; j < cell.y; j++) {
        if (i < 0 || already_placed_bboundary_i[i])
          continue;
        if (j < 0 || already_placed_bboundary_j[j])
          continue;
        if (grid[i + x_max * j].on_boundary)
          continue;
        if (is_on_bboundary(i, j)) {

          new_bboundary.push_back(Coord(i, j));
          if (i >= 0 && j >= 0)
            grid[i + x_max * j].on_bboundary = true;
          if (i < 0) already_placed_bboundary_i[i] = true;
          if (j < 0) already_placed_bboundary_j[j] = true;
        }
      }
  }

  free(boundary);
  free(bboundary);
  boundary = (Coord *)malloc(new_boundary.size() * sizeof(Coord));
  for (int i = 0; i < new_boundary.size(); i++)
    boundary[i] = new_boundary[i];
  boundary_size = new_boundary.size();
  bboundary = (Coord *)malloc(new_bboundary.size() * sizeof(Coord));
  for (int i = 0; i < new_bboundary.size(); i++)
    bboundary[i] = new_bboundary[i];
  bboundary_size = new_bboundary.size();
  for (int i = 0; i < num_colors; i++) {
    volumes[i] = newv[i];
    pressures[i] = P0V0[i] / volumes[i];
  }
}

void Simulation::run_sim_iteration() {

  double sa0 = find_surface_area(COLOR);
  for (int iter = 0; iter < 30; iter++) {
    double dep = 0;
    for (int i = 0; i < boundary_size; i++) {
      auto cell = boundary[i];
      flip_cell(cell.x, cell.y);
      if (grid[cell.x + x_max * cell.y].get_color(NEW) !=
          grid[cell.x + x_max * cell.y].get_color(OLD)) {

        dep += pressures[grid[cell.x + x_max * cell.y].get_color(OLD)] -
               pressures[grid[cell.x + x_max * cell.y].get_color(NEW)];
      }
    }

    double sa1 = find_surface_area(NEW);
    double dsa = sa1 - sa0;

    double de = k_a * dsa + k_p * dep;
    double prob = std::min(1.0, exp(-de));
    double roll = roll01();
    if (roll < prob) {
      for (int i = 0; i < boundary_size; i++)
        grid[boundary[i].x + x_max * boundary[i].y].old_color =
            grid[boundary[i].x + x_max * boundary[i].y].new_color;
      sa0 = sa1;
    }
  }
  rebuild();
}
