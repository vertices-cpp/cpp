/************************************************************/
/************************************************************/
/**************** A star algorithm   ************************/
/************        by c_cpp123    **************************/
/**************       ************************/
/************************************************************/
/************************************************************/
#include<vector>
#include<string>
#include<list>
#include<algorithm>
#include<iostream>
using namespace std;

/* up,down,left,right */
//const char *dir[] = { "上","下","左","右" };
//int direction[4][2] = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };	// 用来生成节点周围上下左右的坐标移动值
const char *dir[] = { "上","下","左","右","左斜上","右斜下","左斜下","右斜上" };
int direction[8][2] = { {0, -1}, {0, 1}, {-1, 0}, {1, 0},{-1,-1},{1,1},{-1,1},{1,-1} };	// 用来生成节点周围上下左右的坐标移动值
/*	0:空白点 NULL tile
*	1:起始点 Start
*	2:目标点 Finish
*	3:墙	Wall
*	4:路线	Path
*/

struct Node
{
public:
	Node(char d_tile, int dx, int dy, int dg = 0, int df = 0)
		: tile(d_tile), x(dx), y(dy), g(dg), h(0), f(df), dir(0)
	{
	}
	/* 包含坐标
	*	F=G+H
	*	G表示该点到起始点位所需要的代价
	*	H表示该点到终点的曼哈顿距离。
	*	F就是G和H的总和，而最优路径也就是选择最小的F值，进行下一步移动（后边会做详细介绍）
	*	地图块
	*	方向
	*/
	int x, y, g, h, f, dir;
	char tile;
	Node(const Node &r) :x(r.x), y(r.y), g(r.g), h(r.h), f(r.f), dir(0), tile(r.tile) {}
	Node& operator=(const Node &r) {
		if (this != &r)
		{
			x = r.x, y = r.y, g = r.g, h = r.h, f = r.f, dir = 0, tile = r.tile;
		}
		return *this;
	}
	void Assign(int dx, int dy, int dg = 0, int df = 0)
	{
		x = dx, y = dy, g = dg, h = df, dir = 0, tile = '\0';
	}
	void calculateCost(int x2, int y2)
	{
		g += 1;
		h = std::abs(x2 - x) + std::abs(y2 - y);
		f = g + h;
	}
};


class Astar
{
	int mWidth, mHeight, xStart, yStart, xFinish, yFinish, success;
public:
	string path;
	vector<vector<Node>> nodeMap;
	vector <Node>openlist;	// open表
	vector < Node>closedlist;	// closed表
	Astar()
	{
		// Assign(w, h);
	}
	void Assign(int w, int h)
	{
		nodeMap.resize(h);
		mWidth = w, mHeight = h;
		xStart = yStart = xFinish = yFinish = -1, success = 0;
		for (int y = 0; y < mHeight; ++y)
		{
			for (int x = 0; x < mWidth; ++x)
				nodeMap[y].push_back(Node('.', x, y, 0, 0));
		}
	}
	void free()
	{
		for (size_t i = 0; i < nodeMap.size(); ++i)
		{
			nodeMap[i].clear();
		}
		nodeMap.clear();

		path.clear();
		openlist.clear();
		closedlist.clear();
	}
	~Astar()
	{
		free();
	}

	void setWall(int x, int y)	// 设置障碍
	{
		char c = nodeMap[y][x].tile;

		if (c != 'A' && c != 'B')
		{
			nodeMap[y][x].tile = '#';
		}
	}
	void setFinish(int x, int y)	// 设置目标
	{
		char c = nodeMap[y][x].tile;

		if (c != 'A'&& c != '#')
		{
			if ((xFinish != -1 && yFinish != -1) && (xFinish != x || yFinish != y))
				nodeMap[yFinish][xFinish].tile = '.';//如果前面有设置过则置0

			nodeMap[y][x].tile = (c == '.' ? 'B' : '.');
			xFinish = x, yFinish = y;
		}
	}
	void setStart(int x, int y)	// 设置开始
	{
		char c = nodeMap[y][x].tile;

		if (c != 'B'&& c != '#')
		{
			if ((xStart != -1 && yStart != -1) && (xStart != y || yStart != x))
				nodeMap[yStart][xStart].tile = '.';

			nodeMap[y][x].tile = (c == '.' ? 'A' : '.');

			xStart = x, yStart = y;
		}
	}
	

	vector<Node>::iterator findLastNode(vector<Node>& v)
	{
		auto result = v.begin();
		for (auto i = v.begin(); i != v.end(); i++) {
			if (result->f > i->f) {
				result = i;
			}
		}
		return result;
	}
	string pathFind()
	{
		nodeMap[yStart][xStart].calculateCost(xFinish, yFinish);
		openlist.push_back(nodeMap[yStart][xStart]);

		// A* 算法
		while (!openlist.empty())
		{

			auto curit = findLastNode(openlist);


			int cx = curit->x, cy = curit->y;
			//需要计算过的
			Node curNode = *curit;
			//Node curNode = nodeMap[cy][cx];

			closedlist.push_back(*curit);
			openlist.erase(curit);


			if (cx == xFinish && cy == yFinish)
			{
				char c;
				while (!(cx == xStart && cy == yStart))
				{
					int j = nodeMap[cy][cx].dir;
					c = j ^ 1;
					path = c + path;//往前插入
					cx += direction[j][0];
					cy += direction[j][1];
				}
				return path;
			}

			for (int i = 0; i < sizeof(direction) / sizeof(*direction); i++)
			{
				int dx = cx + direction[i][0], dy = cy + direction[i][1];

				if (!(dx < 0 || dx >= mWidth || dy < 0 || dy >= mHeight || nodeMap[dy][dx].tile == '#' ||
					find_if(closedlist.begin(), closedlist.end(),
						[&](Node &a) {return dx == a.x && dy == a.y; }) != closedlist.end()))
				{

					Node tmp('.', dx, dy, curNode.g, curNode.f);
					tmp.calculateCost(xFinish, yFinish);

					auto open_it = find_if(openlist.begin(), openlist.end(),
						[&](Node &a) {return dx == a.x && dy == a.y; });

					if (open_it == openlist.end())
					{
						openlist.push_back(tmp);
						nodeMap[dy][dx].dir = i ^ 1;

					}
					else if (open_it != openlist.end() && open_it->f > tmp.f)
					{
						*open_it = tmp;

						nodeMap[dy][dx].dir = i ^ 1;
					}

				}
			}

		}
		return ""; // no route found
	}
	void ShowPath() {
		if (path.size())
		{
			for (auto &i : path)
			{
				cout << " " << dir[i];
			}
			cout << endl;
		}
	}
	//	private:
	void  ShowMap()
	{

		if (path.size())
		{
			int cx = xStart, cy = yStart;
			for (size_t i = 0; i < path.size() - 1; ++i)
			{
				int j = path[i];
				cx += direction[j][0], cy += direction[j][1];
				nodeMap[cy][cx].tile = 'r';
			}
			// 		nodeMap[yStart][xStart].tile = 'A';
			// 		nodeMap[yFinish][xFinish].tile = 'B';
		}
		for (auto y = 0; y < mHeight; ++y) {
			for (int x= 0; x < mWidth; ++x)
			{
				cout << nodeMap[y][x].tile;
			}
			cout << endl;
		}
	}
};

int main()
{
	Astar astar;
	//初始化地图大小
	astar.Assign(10, 10);

	/* 设置墙 */
	for (int i = 2; i < 8; i++)
	{
		astar.setWall(i, 5);
		astar.setWall(5, i);
	}
	astar.setWall(0, 5);
	astar.setWall(1, 5);
	astar.ShowMap();
	/* 设置起点与终点 */
	astar.setStart(3, 2);
	astar.setFinish(2, 6);

	cout << "总共步数:" << astar.pathFind().size() << endl;
	astar.ShowPath();
	astar.ShowMap();

	return 0;
}