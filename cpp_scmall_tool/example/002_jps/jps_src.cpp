#include <cmath>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <queue>
#include <string>
#include <unordered_set>
using namespace std;

// #define PRINT_CODE

struct Location
{
	int x, y;
	Location direction() const;
};

bool operator==(const Location& a, const Location& b) noexcept;
bool operator!=(const Location& a, const Location& b) noexcept;
bool operator<(const Location& a, const Location& b) noexcept;
Location operator+(const Location& a, const Location& b) noexcept;
Location operator-(const Location& a, const Location& b) noexcept;
Location operator*(const int a, const Location& b) noexcept;
Location operator*(const Location& a, const int b) noexcept;
std::ostream& operator<<(std::ostream& os, const Location& a);

const Location NoneLoc{ -1, -1 };

/* implement hash function so we can put Location into an unordered_set */
namespace std
{
	template <> struct hash<Location>
	{
		typedef Location argument_type;
		typedef std::size_t result_type;
		std::size_t operator()(const Location& id) const noexcept
		{
			return std::hash<int>()(id.x ^ (id.y << 4));
		}
	};
}

class Grid
{
private:
	int width, height;

public:
	std::unordered_set<Location> walls;

	Grid(int width_, int height_, std::unordered_set<Location> walls_) : width(width_), height(height_), walls(walls_) {};

	int get_width() const { return width; };
	int get_heigth() const { return height; };

	bool in_bounds(const Location& loc) const noexcept { return 0 <= loc.x && loc.x < width && 0 <= loc.y && loc.y < height; };
	bool passable(const Location& loc) const { return walls.find(loc) == walls.end(); };
	bool valid(const Location& loc) const { return in_bounds(loc) && passable(loc); };
	bool valid_diag_move(const Location& loc, const Location& dir) const;
	bool forced(const Location& loc, const Location& parent, const Location& travel_dir) const;

	std::vector<Location> neighbours(const Location& current) const;
	std::vector<Location> pruned_neighbours(const Location& current, const Location& parent) const;
};

const static Location DIRS[]{
	/* East, West, North, South */
	/* NE, NW, SE, SW*/
	{1, 0}, {-1, 0},
	{0, -1}, {0, 1},
	{1, 1}, {-1, 1},
	{1, -1}, {-1, -1}
};

/* Direction arrows: E, W, N, S, NE, NW, SE, SW */
const static char*dir_str[] = { "→","←","↑","↓","↘","↙","↗","↖" };

#ifdef PRINT_CODE
const char* find_str(const Location &r)
{
	for (int i = 0; i < sizeof(DIRS) / sizeof(*DIRS); ++i)
		if (DIRS[i] == r)
			return dir_str[i];
	return " ";
}
#endif

Location Location::direction() const
{
	return Location{ x > 0 ? 1 : (x < 0 ? -1 : 0), y > 0 ? 1 : (y < 0 ? -1 : 0) };
}

bool operator==(const Location& a, const Location& b) noexcept
{
	return a.x == b.x && a.y == b.y;
}

bool operator!=(const Location& a, const Location& b) noexcept
{
	return a.x != b.x || a.y != b.y;
}

bool operator<(const Location& a, const Location& b) noexcept
{
	return std::tie(a.x, a.y) < std::tie(b.x, b.y);
}

Location operator+(const Location& a, const Location& b) noexcept
{
	return { a.x + b.x, a.y + b.y };
}

Location operator-(const Location& a, const Location& b) noexcept
{
	return { a.x - b.x, a.y - b.y };
}

Location operator*(const int a, const Location& b) noexcept
{
	return { a * b.x, a * b.y };
}

Location operator*(const Location& a, const int b) noexcept
{
	return { b * a.x, b * a.y };
}

std::ostream& operator<<(std::ostream& os, const Location& a)
{
	return os << "<Location {" << a.x << ", " << a.y << "}>";
}

bool Grid::forced(const Location& loc, const Location& parent, const Location& travel_dir) const {
	const Location dir{ (loc - parent).direction() };
	// Diagonal neighbour
	if (travel_dir.x != 0 && travel_dir.y != 0) {
		if ((dir.x == travel_dir.x && dir.y == -travel_dir.y) ||
			(dir.x == -travel_dir.x && dir.y == travel_dir.y)) {
#ifdef PRINT_CODE
			cout << "forced diag:" << travel_dir << dir << endl;
#endif
			return true;

		}
	}
	// Horizontal or vertical neighbour
	else if (dir.x != 0 && dir.y != 0) {
#ifdef PRINT_CODE
		cout << "forced h/v:" << travel_dir << dir << endl;
#endif
		return true;
	}
	return false;
}

bool Grid::valid_diag_move(const Location& loc, const Location& dir) const {
	return (dir.x != 0 && dir.y != 0) && (valid(loc + dir) &&
		(passable(loc + Location{ dir.x, 0 }) || passable(loc + Location{ 0, dir.y })));
}

vector<Location> Grid::neighbours(const Location& current) const
{
	vector<Location> results;
	//     for (auto& dir : DIRS) {
	//         if (dir.x != 0 && dir.y != 0) {
	//             if (valid_diag_move(current, dir)) {
	//                 results.push_back(current + dir);
	//                 cout << __func__ << " " << find_str(dir) << " " << current + dir << endl;
	//             }
	//         }
	//         else {
	//             if (valid(current + dir)) {
	//                 results.push_back(current + dir);
	//                 cout << __func__ << " " << find_str(dir) << " " << current + dir << endl;
	//             }
	//         }
	//     }
	for (int i = 0; i < sizeof(DIRS) / sizeof(*DIRS); ++i) {
		if (DIRS[i].x != 0 && DIRS[i].y != 0) {
			if (valid_diag_move(current, DIRS[i])) {
				results.push_back(current + DIRS[i]);
#ifdef PRINT_CODE
				cout << __func__ << " " << find_str(DIRS[i]) << " " << current + DIRS[i] << endl;
#endif
			}
		}
		else {
			if (valid(current + DIRS[i])) {
				results.push_back(current + DIRS[i]);
#ifdef PRINT_CODE
				cout << __func__ << " " << find_str(DIRS[i]) << " " << current + DIRS[i] << endl;
#endif
			}
		}
	}
	return results;
}

vector<Location> Grid::pruned_neighbours(const Location& current, const Location& parent) const
{
	if (parent == NoneLoc) {
		return neighbours(current);
	}
	vector<Location> neighbours;
	const auto dir = (current - parent).direction();
	// Diagonal neighbour
	if (dir.x != 0 && dir.y != 0) {
#ifdef PRINT_CODE
		cout << "na start:" << current << parent << endl;
#endif
		const Location dir_x{ dir.x, 0 };
		const Location dir_y{ 0, dir.y };

		// Add natural neighbours
		for (const auto& move_dir : { dir, dir_x, dir_y }) {
			if (move_dir.x != 0 && move_dir.y != 0) {
				if (valid_diag_move(current, move_dir)) {
					neighbours.push_back(current + move_dir);
#ifdef PRINT_CODE
					cout << __func__ << " " << find_str(move_dir) << " " << current + move_dir << endl;
#endif
				}
			}
			else {
				if (valid(current + move_dir)) {
					neighbours.push_back(current + move_dir);
#ifdef PRINT_CODE
					cout << __func__ << " " << find_str(move_dir) << " " << current + move_dir << endl;
#endif
				}
			}
		}
#ifdef PRINT_CODE
		cout << "......" << dir_x << dir_y << endl;
#endif
		// Add forced neighbours
		for (const auto& candidate_dir : { dir_x, dir_y }) {
			Location cur_parent = (current - dir);
			if (!valid(cur_parent + candidate_dir) && valid(cur_parent + 2 * candidate_dir) && valid(current + candidate_dir)) {
				neighbours.push_back(cur_parent + 2 * candidate_dir);
#ifdef PRINT_CODE
				cout << __func__ << " " << find_str(candidate_dir) << " " << (cur_parent + 2 * candidate_dir) << endl;
#endif
			}
		}

#ifdef PRINT_CODE
		cout << "na end" << endl;
#endif
	}
	// Horizontal or vertical neighbour
	else
	{
#ifdef PRINT_CODE
		cout << "vh start" << current << parent << endl;
#endif
		// Add forced neighbours
		const Location inverted_dir{ dir.y, dir.x };
		// Add natural neighbours
		if (valid(current + dir)) {
			neighbours.push_back(current + dir);
#ifdef PRINT_CODE
			cout << __func__ << " " << find_str(dir) << " " << current + dir << endl;
#endif

			if (!valid(current + inverted_dir) && valid(current + inverted_dir + dir)) {
				neighbours.push_back(current + inverted_dir + dir);
#ifdef PRINT_CODE
				cout << ":" << __func__ << current + inverted_dir + dir << endl;
#endif
			}
			if (!valid(current - inverted_dir) && valid(current - inverted_dir + dir)) {
				neighbours.push_back(current - inverted_dir + dir);
#ifdef PRINT_CODE
				cout << ":" << __func__ << current - inverted_dir + dir << endl;
#endif
			}
		}

#ifdef PRINT_CODE
		cout << "vh end" << endl;
#endif
	}
	return neighbours;
}

namespace Tool
{
	inline double manhattan(const Location& a, const Location& b) { return abs(a.x - b.x) + abs(a.y - b.y); };
	inline double euclidean(const Location& a, const Location& b) { return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y)); };
	std::vector<Location> reconstruct_path(
		const Location& start,
		const Location& goal,
		const std::unordered_map<Location, Location>& came_from);

	void draw_grid(
		const Grid& grid,
		const std::unordered_map<Location, double>& distances = {},
		const std::unordered_map<Location, Location>& point_to = {},
		const std::vector<Location>& path = {},
		const std::unordered_map<Location, Location>& came_from = {},
		const Location& start = NoneLoc,
		const Location& goal = NoneLoc);
};

vector<Location> Tool::reconstruct_path(
	const Location& start,
	const Location& goal,
	const unordered_map<Location, Location>& came_from)
{
	vector<Location> path{};
	Location current = goal;
	while (current != start) {
		path.push_back(current);
		if (came_from.count(current)) {
			current = came_from.at(current);
		}
		else {
			break;
		}
	}
	reverse(path.begin(), path.end());
	return path;
}

// This outputs a grid. Pass in a distances map if you want to print
// the distances, or pass in a point_to map if you want to print
// arrows that point to the parent location, or pass in a path vector
// if you want to draw the path, or pass a came_from map to print jump
// point nodes
void Tool::draw_grid(
	const Grid& grid,
	const unordered_map<Location, double>& distances,
	const unordered_map<Location, Location>& point_to,
	const vector<Location>& path,
	const unordered_map<Location, Location>& came_from,
	const Location& start,
	const Location& goal)
{
	const int field_width = 3;
	cout << string(field_width * grid.get_width(), '_') << '\n';
	for (int y = 0; y != grid.get_heigth(); ++y) {
		printf("%2d:", y);
		for (int x = 0; x != grid.get_width(); ++x) {
			const Location id{ x, y };
			if (grid.walls.find(id) != grid.walls.end()) {
				cout << "#";
			}
			else if (start != NoneLoc && id == start) {
				cout << "A";
			}
			else if (goal != NoneLoc && id == goal) {
				cout << "Z";
			}
			else if (!path.empty() && find(path.begin(), path.end(), id) != path.end()) {
				cout << "@";
			}
			else if (came_from.count(id)) {
				cout << "J";
			}
			else if (point_to.count(id)) {
				const auto next = point_to.at(id);
				if (next.x == x + 1) { cout << " > "; }
				else if (next.x == x - 1) { cout << " < "; }
				else if (next.y == y + 1) { cout << " v "; }
				else if (next.y == y - 1) { cout << " ^ "; }
				else { cout << "*"; }
			}
			else if (distances.count(id)) {
				cout << ' ' << left << setw(field_width - 1) << distances.at(id);
			}
			else {
				cout << ".";
			}
		}
		cout << '\n';
	}
	cout << string(field_width * grid.get_width(), '~') << '\n';
}

typedef pair<double, Location> PQElement;
typedef priority_queue<PQElement, vector<PQElement>, greater<PQElement>> PQLoc;

typedef double(heuristic_fn)(const Location&, const Location&);

Location jump(const Grid& grid, const Location initial, const Location dir,
	const Location goal)
{
	auto new_loc = initial + dir;
	if (!grid.valid(new_loc) ||
		(dir.x != 0 && dir.y != 0) && !grid.valid_diag_move(initial, (new_loc - initial).direction())) {
		return NoneLoc;
	}
	if (new_loc == goal) {
		return new_loc;
	}
	auto neighbours = grid.pruned_neighbours(new_loc, initial);
	for (const auto next : neighbours) {
		if (grid.forced(next, new_loc, dir)) {

			return new_loc;
		}

	}
	if (dir.x != 0 && dir.y != 0) {
		for (const auto& new_dir : { Location{dir.x, 0}, Location{0, dir.y} }) {
			auto jump_point{ jump(grid, new_loc, new_dir, goal) };
			if (jump_point != NoneLoc) {
				return new_loc;
			}
		}
	}

	return jump(grid, new_loc, dir, goal);
}

unordered_map<Location, Location> jps(
	const Grid& grid,
	const Location& start, const Location& goal,
	heuristic_fn heuristic)
{
	PQLoc open_set;
	unordered_map<Location, Location> came_from{};
	unordered_map<Location, double> cost_so_far{};
	vector<Location> close_list;

	open_set.emplace(0, start);
	came_from[start] = start;
	cost_so_far[start] = 0;
	Location parent{ NoneLoc };
	int expanded(0);

	while (!open_set.empty()) {
		const auto current = open_set.top().second;
		if (current != start) {
			parent = came_from[current];
		}
#ifdef PRINT_CODE
		cout << "[" << current << "]" << endl;
#endif

		open_set.pop();
		close_list.push_back(current);

		expanded++;

		if (current == goal) {
			break;
		}
#ifdef PRINT_CODE
		cout << "successors start" << endl;
#endif
		vector<Location> successors;
		auto neighbours = grid.pruned_neighbours(current, parent);
		for (const auto& n : neighbours) {
			auto jump_point = jump(grid, current, (n - current).direction(), goal);
			if (jump_point != NoneLoc) {
				if (find(close_list.begin(), close_list.end(), jump_point) != close_list.end())
				{
					continue;
				}
				successors.push_back(jump_point);
			}
		}
#ifdef PRINT_CODE
		cout << "successors end" << endl;
#endif
		for (const auto& next : successors) {

			const auto new_cost = cost_so_far[current] + heuristic(current, next);
			if (cost_so_far.find(next) == cost_so_far.end() || new_cost < cost_so_far[next]) {
				cost_so_far[next] = new_cost;
				came_from[next] = current;
				open_set.emplace(new_cost + heuristic(next, goal), next);
			}

		}
	}
	cout << "Expanded nodes: " << expanded << ". Total nodes: " << expanded + open_set.size() << '\n';
	return came_from;
}

int main()
{
	unordered_set<Location> walls{
		//    {0, 2},{2, 2}, {2, 1}, {2, 0},{2,5}};
		{12,0},{11,1},{10,2},{9,3},{8,4},{7,5},
		{6,6},{5,7},{4,8},{3,9},{2,10},{1,11},
		{1,12},{1,13},{2,14},{2,15},{5,6},
		{0,1},{1,0},{2,1},{3,16},{4,16},{5,16} };
	Grid map{ 18, 18, walls };

	Location start{ 1, 1 };
	Location goal{ 17,8 };

	auto came_from = jps(map, start, goal, Tool::euclidean);
	auto path = Tool::reconstruct_path(start, goal, came_from);
	Tool::draw_grid(map, {}, {}, path, came_from, start, goal);
}
// void jps_main()
// {
//     Grid grid(GridWidth, GridHeight, GridWalls);
//     auto came_from = jps(grid, start, goal, Tool::euclidean);
//     Tool::reconstruct_path(start, goal, came_from);
// }