/************************************************************/
/************************************************************/
/**************** A star algorithm   ************************/
/************        by c_cpp123    **************************/
/**************       ************************/
/************************************************************/
/************************************************************/

#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <string>
#include <vector>
#include <iomanip>
#include <iostream>

using namespace std;


/*	0:空白点 NULL tile
*	1:起始点 Start
*	2:目标点 Finish
*	3:墙	Wall
*	4:路线	Path
*/

struct Location
{
	int x, y;
	double g;
};
bool operator==(const Location &a, const Location &b) noexcept{
	return a.x == b.x && a.y == b.y;
}
bool operator!=(const Location &a, const Location &b) noexcept {
	return !(a==b);
}
Location operator + (const Location &a, const Location &b) noexcept {
	return { a.x + b.x,a.y + b.y,a.g+b.g };
}

namespace std {
	template<>
	struct hash<Location>
	{
		std::size_t operator()(const Location &id)const noexcept {
			return std::hash<int>()(id.x ^( id.y << 4));
		}
	};
};

//const char *dir[] = { "上","下","左","右" };
//Location LocationDir[4]  = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };	// 用来生成节点周围上下左右的坐标移动值
const char *dir[] = { "上","下","左","右","左斜上","右斜下","左斜下","右斜上" };
Location LocationDir[8] = {
	{0, -1,1},
{0, 1,1},
{-1, 0,1},
{1, 0,1},
{-1,-1,1.4},
{1,1,1.4},
{-1,1,1.4},
{1,-1,1.4} };// 用来生成节点周围上下左右的坐标移动值

class Grid
{
private:
	int width, height;
public:
	std::unordered_set<Location> walls;
	Grid(int w, int h, std::unordered_set<Location> walls_) :width(w), height(h), walls(walls_) {};
	int get_width() const { return width; }
	int get_height() const { return height; }
	/* 是否到达边界 */
	bool in_bounds(const Location &loc)const {
		return loc.x >= 0 && loc.x < width &&loc.y >= 0 && loc.y < height;
	}
	/* 是否遇到墙 */
	bool passable(const Location &loc)const {
		return walls.find(loc) == walls.end();
	}

};

/* 包含坐标
	*	F=G+H
	*	G表示该点到起始点位所需要的代价 
	*	H表示该点到终点的曼哈顿距离。
	*	F就是G和H的总和，而最优路径也就是选择最小的F值，进行下一步移动（后边会做详细介绍）
	*	地图块
	*	方向
	*/
namespace Tool {
	inline double manhattan(const Location& a, const Location& b) { return abs(a.x - b.x) + abs(a.y - b.y); };
	inline double euclidean(const Location& a, const Location& b) {
		return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
	};
	inline double calculateCost(const Location& a, const Location& b)
	{
		return euclidean(a, b) + a.g;
	}
	std::vector<Location> reconstruct_path(const Location &start,
		const Location &goal, const unordered_map<Location, Location> &came_from);

	void draw_grid(
		const Grid& grid,
		const std::unordered_map<Location, double>& distances = {},
		const std::unordered_map<Location, Location>& point_to = {},
		const std::vector<Location>& path = {},
		const std::unordered_map<Location, Location>& came_from = {},
		const Location& start = Location{ -1,-1 },
		const Location& goal = Location{ -1,-1 });
};

vector<pair<Location, double>>::iterator findLastNode(vector<pair<Location, double>>& v)
{
	auto result = v.begin();
	for (auto i = v.begin(); i != v.end(); i++) {
		if (result->second > i->second) {
			result = i;
		}
	}
	return result;
}

typedef double(calculateCost_fn)(const Location&, const Location&);

// A* 算法
unordered_map<Location, Location>  astar(const Grid &grid,
	const Location &start,
	const Location &goal,
	calculateCost_fn calculateCost)
{
	unordered_map<Location, Location> came_from{};
	came_from[start] = start;
	/* 第二项是h值 */
	vector<pair<Location, double>> openlist;
	vector<Location> closelist;
	openlist.push_back({ start,0 });

	while (!openlist.empty())
	{
		auto curitem = findLastNode(openlist);

		auto curLocation = curitem->first;

		closelist.push_back(curLocation);
		openlist.erase(curitem);


		if (curLocation == goal)
		{
			break;
		}
		/* 先得到有效邻居 */
		vector<Location> successors;

		for (auto neighbours : LocationDir)
		{
			Location tmpLoc =  curLocation +neighbours;
			if (grid.in_bounds(tmpLoc) && grid.passable(tmpLoc) &&
				find(closelist.begin(), closelist.end(), tmpLoc) == closelist.end())
			{
				successors.push_back(tmpLoc);
			}
		}
		for (auto &neighbours : successors)
		{
			pair<Location, double>  tmpLoc = { neighbours,calculateCost(neighbours,goal) };

			auto open_it= find_if(openlist.begin(), openlist.end(),[&](pair<Location, double>& a) {return a.first == tmpLoc.first; });
			
			if (open_it == openlist.end())
			{
				openlist.push_back(tmpLoc);
				came_from[tmpLoc.first] = curLocation;
			}
			else if (open_it != openlist.end() && open_it->second > tmpLoc.second)
			{
				*open_it = tmpLoc;
				came_from[tmpLoc.first] = curLocation;
			}

		}
	}
	return came_from; // no route found
}

vector<Location> Tool::reconstruct_path(const Location &start,
	const Location &goal, const unordered_map<Location, Location> &came_from)
{
	vector<Location> path;
	Location current = goal;
	while (current!=start)
	{
		path.push_back(current);
		if (came_from.count(current))
		{
			current = came_from.at(current);
		}
		else
			break;
	}
	reverse(path.begin(), path.end());
	return path;
}

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
	for (int y = 0; y != grid.get_height(); ++y)
	{
		for (int x = 0; x != grid.get_width(); ++x)
		{
			const Location id{ x, y };
			if (grid.walls.find(id) != grid.walls.end())
			{
				cout << string(" # ");// (field_width, '#');
			}
			else if (start != Location{ -1,-1 } && id == start)
			{
				cout << " A ";
			}
			else if (goal != Location{ -1,-1 } && id == goal)
			{
				cout << " Z ";
			}
			else if (!path.empty() && find(path.begin(), path.end(), id) != path.end())
			{
				cout << " @ ";
			}
// 			else if (came_from.count(id))
// 			{
// 				cout << " J ";
// 			}
			else if (point_to.count(id))
			{
				const auto next = point_to.at(id);
				if (next.x == x + 1)
				{
					cout << " > ";
				}
				else if (next.x == x - 1)
				{
					cout << " < ";
				}
				else if (next.y == y + 1)
				{
					cout << " v ";
				}
				else if (next.y == y - 1)
				{
					cout << " ^ ";
				}
				else
				{
					cout << " * ";
				}
			}
			else if (distances.count(id))
			{
				cout << ' ' << left << setw(field_width - 1) << distances.at(id);
			}
			else
			{
				cout << " . ";
			}
		}
		cout << '\n';
	}
	cout << string(field_width * grid.get_width(), '~') << '\n';
}

int main()
{
	unordered_set<Location> walls{ {1,1},{1,2},{1,3},{0,3},{4,4} ,{4,1},{3,4} ,{4,3} ,{4,2} };
	Grid map{ 10,10,walls };

	Location start{ 0,0,0 }, goal{ 8,8 };

	auto came_from = astar(map, start, goal, Tool::calculateCost);
	auto path = Tool::reconstruct_path(start, goal, came_from);
	Tool::draw_grid(map, {}, {}, path, came_from, start, goal);
	return 0;
}