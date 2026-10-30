// PathFinding.js translation, JPS extracted and ported to C++
// by YouRanXiaoCi qq108201645
// Source: PathFinding.js
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <queue>
#include <iomanip>
#include <initializer_list>
#include <string>
using namespace std;

typedef struct Location
{
	int x, y;
	double g, h, f;
	bool walkable, opened, closed;
	struct Location *parent;
} Loc;

ostream &operator<<(ostream &os, const Location &a)
{
	return os << "<" << a.x << "-" << a.y << ">";
}
inline bool operator==(const Loc &a, const Loc &b)
{
	// cout << "operator==(const Loc &a, const Loc &b)" << endl;
	return a.x == b.x && a.y == b.y;
}
inline bool operator==(const reference_wrapper<Location> &a, const reference_wrapper<Location> &b)
{
	// cout << "operator==(const reference_wrapper<Location> &a, const reference_wrapper<Location> &b)" << endl;
	return a.get().x == b.get().x && a.get().y == b.get().y;
}
bool operator!=(const Loc &a, const Loc &b)
{
	return a.x != b.x || a.y != b.y;
}

bool operator<(const Loc &a, const Loc &b)
{
	return a.f < b.f;
}
Location NoneLoc = { -1, -1 };

double heuristic(int ax, int ay, int bx, int by)
{
	double dx = abs(ax - bx);
	double dy = abs(ay - by);
	return sqrt(dx * dx + dy * dy);
}
typedef vector<std::reference_wrapper<Location>> VecRef;

typedef VecRef VecLoc;
typedef reference_wrapper<Location> Ref_Loc;

struct Grid
{
	int width, height;
	vector<vector<Location>> node_map;
	Grid(int w, int h, vector<Location> walls) : width(w), height(h)
	{
		node_map.resize(height);
		for (int i = 0; i < width; ++i)
			for (int j = 0; j < height; ++j)
				node_map[i].push_back({ i, j, 0, 0, false, false, false, NULL });

		setWalks(walls);
	}
	void setWalks(vector<Location> walls)
	{
		for (auto &i : walls)
			node_map[i.x][i.y].walkable = true;
	}

	int get_width() const
	{
		return width;
	}
	int get_height() const
	{
		return height;
	}
	bool is_bounds(int x, int y) const
	{
		return (x >= 0 && x < width) && (y >= 0 && y < height);
	}
	bool passable(int x, int y) const
	{
		return node_map[x][y].walkable != true;
	}
	int isWalkableAt(int x, int y) const
	{
		return is_bounds(x, y) && passable(x, y);
	}

	VecRef getNeighbors(const Location &node);
	VecRef findNeighbors(const Location &node);
};

VecRef Grid::getNeighbors(const Location &node)
{
	int x = node.x, y = node.y;
	VecRef neighbors;
	int s0 = 0, d0 = 0, s1 = 0, d1 = 0, s2 = 0, d2 = 0, s3 = 0, d3 = 0;

	// Up
	if (isWalkableAt(x, y - 1))
	{
		neighbors.push_back(node_map[x][y - 1]);
		s0 = true;
	}
	// Right
	if (isWalkableAt(x + 1, y))
	{
		neighbors.push_back(node_map[x + 1][y]);
		s1 = true;
	}
	// Down
	if (isWalkableAt(x, y + 1))
	{
		neighbors.push_back(node_map[x][y + 1]);
		s2 = true;
	}
	// Left
	if (isWalkableAt(x - 1, y))
	{
		neighbors.push_back(node_map[x - 1][y]);
		s3 = true;
	}

	d0 = s3 || s0;
	d1 = s0 || s1;
	d2 = s1 || s2;
	d3 = s2 || s3;

	// Up-left
	if (d0 && isWalkableAt(x - 1, y - 1))
	{
		neighbors.push_back(node_map[x - 1][y - 1]);
	}
	// Up-right
	if (d1 && isWalkableAt(x + 1, y - 1))
	{
		neighbors.push_back(node_map[x + 1][y - 1]);
	}
	// Down-right
	if (d2 && isWalkableAt(x + 1, y + 1))
	{
		neighbors.push_back(node_map[x + 1][y + 1]);
	}
	// Down-left
	if (d3 && isWalkableAt(x - 1, y + 1))
	{
		neighbors.push_back(node_map[x - 1][y + 1]);
	}

	return neighbors;
};

VecRef Grid::findNeighbors(const Location &node)
{
	const Location parent = *node.parent;
	int x = node.x, y = node.y;

	int px, py, dx, dy;
	VecRef neighbors;

	// directed pruning: can ignore most neighbors, unless forced.
	if (parent != NoneLoc)
	{
		px = parent.x;
		py = parent.y;
		// get the normalized direction of travel
		dx = (x - px) / max(abs(x - px), 1);
		dy = (y - py) / max(abs(y - py), 1);
		// cout << dx << " " << dy << endl;
		// search diagonally
		if (dx != 0 && dy != 0)
		{
			if (isWalkableAt(x, y + dy))
			{
				neighbors.push_back(node_map[x][y + dy]);
			}
			if (isWalkableAt(x + dx, y))
			{
				neighbors.push_back(node_map[x + dx][y]);
			}
			/* If one side of the diagonal is walkable and the diagonal front is passable */
			if (isWalkableAt(x + dx, y + dy) && (isWalkableAt(x, y + dy) || isWalkableAt(x + dx, y)))
			{
				neighbors.push_back(node_map[x + dx][y + dy]);
			}
			if (!isWalkableAt(x - dx, y) && isWalkableAt(x, y + dy) && isWalkableAt(x - dx, y + dy))
			{
				neighbors.push_back(node_map[x - dx][y + dy]);
			}
			if (!isWalkableAt(x, y - dy) && isWalkableAt(x + dx, y) && isWalkableAt(x + dx, y - dy))
			{
				neighbors.push_back(node_map[x + dx][y - dy]);
			}
		}
		// search horizontally/vertically
		else
		{
			if (dx == 0)
			{
				if (isWalkableAt(x, y + dy))
				{
					neighbors.push_back(node_map[x][y + dy]);
					if (!isWalkableAt(x + 1, y) && isWalkableAt(x + 1, y + dy))
					{
						neighbors.push_back(node_map[x + 1][y + dy]);
					}
					if (!isWalkableAt(x - 1, y) && isWalkableAt(x - 1, y + dy))
					{
						neighbors.push_back(node_map[x - 1][y + dy]);
					}
				}
			}
			else
			{
				if (isWalkableAt(x + dx, y))
				{
					neighbors.push_back(node_map[x + dx][y]);
					if (!isWalkableAt(x, y + 1) && isWalkableAt(x + dx, y + 1))
					{
						neighbors.push_back(node_map[x + dx][y + 1]);
					}
					if (!isWalkableAt(x, y - 1) && isWalkableAt(x + dx, y - 1))
					{
						neighbors.push_back(node_map[x + dx][y - 1]);
					}
				}
			}
		}
	}
	// return all neighbors
	else
	{
		return getNeighbors(node);
	}

	return neighbors;
}

struct JumpPointFinder
{
	Grid &grid;
	Ref_Loc start;
	Ref_Loc goal;
	JumpPointFinder(Grid &grid_, Location &start_, Location &goal_) : grid(grid_), start(grid.node_map[start_.x][start_.y]), goal(grid.node_map[goal_.x][goal_.y])
	{
	}
	Ref_Loc jump(int x, int y, int px, int py);
	bool jps();
};

vector<Location> reconstruct_path(JumpPointFinder &jpf)
{
	vector<Location> path{};
	Location current = jpf.goal.get();
	while (current != jpf.start.get()) {
		path.push_back(current);
		current = *jpf.grid.node_map[current.x][current.y].parent;
	}
	reverse(path.begin(), path.end());
	for (auto &i : path)
		cout << i << endl;
	return path;
}

void draw_grid(JumpPointFinder &jpf, vector<Location> &path)
{
	const int field_width = 3;
	cout << string(field_width * jpf.grid.get_width(), '_') << '\n';
	for (int y = 0; y != jpf.grid.get_height(); ++y) {

		printf("%2d:", y);

		for (int x = 0; x != jpf.grid.get_width(); ++x) {
			const Location id{ x, y };
			if (jpf.grid.node_map[x][y].walkable) {
				cout << "#";
			}
			else if (Location{ x,y } == jpf.start.get()) {
				cout << "A";
			}
			else if (Location{ x,y } == jpf.goal.get()) {
				cout << "Z";
			}
			else if (!path.empty() && find(path.begin(), path.end(), id) != path.end()) {
				cout << "@";
			}
			else if (jpf.grid.node_map[x][y].parent != NULL) {
				cout << "J";
			}
			else {
				cout << ".";
			}
		}
		cout << '\n';
	}
}

Ref_Loc JumpPointFinder::jump(int x, int y, int px, int py)
{
	int dx = x - px, dy = y - py;

	if (!grid.isWalkableAt(x, y))
	{
		return NoneLoc;
	}

	if (grid.node_map[x][y] == goal.get())
	{
		return grid.node_map[x][y];
	}

	// check for forced neighbors
	// along the diagonal
	if (dx != 0 && dy != 0)
	{
		if ((grid.isWalkableAt(x - dx, y + dy) && !grid.isWalkableAt(x - dx, y)) || (grid.isWalkableAt(x + dx, y - dy) && !grid.isWalkableAt(x, y - dy)))
		{
			return grid.node_map[x][y];
		}
		// when moving diagonally, must check for vertical/horizontal jump
		// points
		if (jump(x + dx, y, x, y).get() != NoneLoc || jump(x, y + dy, x, y).get() != NoneLoc)
		{
			return grid.node_map[x][y];
		}
	}
	// horizontally/vertically
	else
	{
		if (dx != 0)
		{ // moving along x
			if (grid.isWalkableAt(x + dx, y) &&
				((grid.isWalkableAt(x + dx, y + 1) && !grid.isWalkableAt(x, y + 1)) || (grid.isWalkableAt(x + dx, y - 1) && !grid.isWalkableAt(x, y - 1))))
			{
				return grid.node_map[x][y];
			}
		}
		else
		{
			if (grid.isWalkableAt(x, y + dy) &&
				((grid.isWalkableAt(x + 1, y + dy) && !grid.isWalkableAt(x + 1, y)) || (grid.isWalkableAt(x - 1, y + dy) && !grid.isWalkableAt(x - 1, y))))
			{
				return grid.node_map[x][y];
			}
		}
	}

	// moving diagonally, must make sure one of the vertical/horizontal
	// neighbors is open to allow the path
	if (grid.isWalkableAt(x + dx, y) || grid.isWalkableAt(x, y + dy))
	{
		return jump(x + dx, y + dy, x, y);
	}
	else
	{
		return NoneLoc;
	}
}

bool JumpPointFinder::jps()
{
	VecLoc openList;
	openList.push_back(start);
	start.get().opened = true;
	start.get().parent = &NoneLoc;

	// while the open list is not empty
	while (!openList.empty())
	{
		// pop the position of node which has the minimum `f` value.

		auto it = std::min_element(openList.begin(), openList.end());
		Ref_Loc current = *it;
		openList.erase(it);

		if (current == goal)
		{
			cout << "Goal found" << endl;
			return true;
		}
		int x = current.get().x, y = current.get().y, jx = 0, jy = 0, endX = goal.get().x, endY = goal.get().y;
		double d = 0, ng = 0;

		VecRef neighbors = grid.findNeighbors(current.get());
		// break;
		for (int i = 0, len = neighbors.size(); i < len; ++i)
		{
			Ref_Loc jumpPoint = jump(neighbors[i].get().x, neighbors[i].get().y, x, y);
			if (jumpPoint.get() != NoneLoc)
			{
				jx = jumpPoint.get().x;
				jy = jumpPoint.get().y;
				if (grid.node_map[jx][jy].closed)
				{
					continue;
				}

				d = heuristic(x, y, jx, jy);
				ng = current.get().g + d;

				if (!jumpPoint.get().opened || ng < jumpPoint.get().g)
				{
					jumpPoint.get().g = ng;
					jumpPoint.get().h = jumpPoint.get().h || heuristic(jx, jy, endX, endY);
					jumpPoint.get().f = jumpPoint.get().g + jumpPoint.get().h;
					jumpPoint.get().parent = &current.get();
					if (!jumpPoint.get().opened)
					{
						openList.push_back(jumpPoint.get());
						jumpPoint.get().opened = true;
					}
					else
						*find(openList.begin(), openList.end(), jumpPoint) = jumpPoint.get();
				}
			}
		}
	}

	// fail to find the path
	return false;
}

int main()
{
	vector<Location> walls{
		// {0, 2},{2, 2}, {2, 1}, {2, 0},{2,5}};
		{12, 0},{11, 1},{10, 2},{9, 3},{8, 4},{7, 5},{6, 6},
		{5, 7},{4, 8},{3, 9},{2, 10},{1, 11},{1, 12},{1, 13},{2, 14},{2, 15},
		{5, 6},{0, 1},{1, 0},{2, 1},{3, 16} };
	Grid map{ 18, 18, walls };
	// Grid map{ 6, 6, walls };
	Location start{ 1, 1 };
	Location goal{ 17, 17 };

	JumpPointFinder jpf(map, start, goal);

	if (jpf.jps())
	{
		auto path = reconstruct_path(jpf);
		draw_grid(jpf, path);
	}
	else
		cerr << "not found" << endl;
}