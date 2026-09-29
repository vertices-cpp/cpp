// Huffman tree simplified, by YouRanXiaoCi
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#pragma warning(disable:4996)
#endif

using namespace std;
// #define HuffmanCode

struct HuffmanNode {
	int weight, parent, left, right;
	friend ostream &operator<<(ostream& os, const HuffmanNode& r) {
		char ch[100];
		sprintf(ch, "%3d %3d %3d %3d", r.weight, r.parent, r.left, r.right);
		return os << ch;
	}
};

void Select(vector<HuffmanNode> &HT, int end, int &m1, int &m2) {
	int w1, w2;
	w1 = w2 = INT_MAX;
	for (int i = 1; i <= end; ++i)
	{
		if (HT[i].parent != 0) continue;

		if (HT[i].weight < w1)
		{
			w2 = w1;
			m2 = m1;

			w1 = HT[i].weight;
			m1 = i;
		}
		else if (HT[i].weight >= w1 && HT[i].weight < w2) {
			w2 = HT[i].weight;
			m2 = i;
		}
	}
}

void HuffmanCoding(vector<HuffmanNode> &HT, vector<string> &HC, vector<int> &w, int n) {
	if (n <= 1) return;

	int m = 2 * n - 1, i, j;
	HT.resize(m + 1);
	HT[0].weight = n;

	for (i = 1, j = 0; i <= n; ++i, ++j)
		HT[i] = { w[j],0,0,0 };
	for (; i <= m; ++i)
		HT[i] = { 0,0,0,0 };

	int m1, m2;
	for (i = n + 1; i <= m; ++i)
	{
		Select(HT, i - 1, m1, m2);

		HT[m1].parent = HT[m2].parent = i;
		HT[i].left = m1, HT[i].right = m2;
		HT[i].weight = HT[m1].weight + HT[m2].weight;
	}
	HC.resize(n + 1);
#ifndef HuffmanCode
	int start, f;
	string s(n, '\0');
	for (i = 1; i <= n; ++i) {
		start = n - 1;
		for (j = i, f = HT[i].parent;
			f != 0;
			j = f, f = HT[f].parent)
		{
			if (HT[f].left == j)
				s[--start] = '0';
			else if (HT[f].right == j)
				s[--start] = '1';
		}
		HC[i].resize(n - start);
		HC[i] = string(s.begin() + start, s.end());
	}
#else
	int r = m, len = 0;
	auto H = HT;
	for (auto&i : H)
		i.weight = 0;
	string s(n, '\0');
	while (r != 0) {
		if (H[r].weight == 0)
		{
			H[r].weight = 1;
			if (H[r].left != 0)
			{
				r = H[r].left;
				s[len++] = '0';
			}
			else if (H[r].right == 0)
			{
				s[len] = '\0';
				HC[r] = s;
			}
		}
		else if (H[r].weight == 1)
		{
			H[r].weight = 2;
			if (H[r].right != 0)
			{
				s[len++] = '1';
				r = H[r].right;
			}
		}
		else
		{
			r = H[r].parent;
			len--;
		}
	}
#endif
}

void HuffmanDecode(vector<HuffmanNode> &HT, vector<string> &HC, vector<int> &w) {
	int n = HC.size() - 1, m = n * 2 - 1;
	for (int i = 1; i <= n; ++i) {
		int f = m;
		string s = HC[i];
		for (auto &j : s) {
			if (j == '0')
				f = HT[f].left;
			else if (j == '1')
				f = HT[f].right;
			else
				break;
		}
		w.push_back(HT[f].weight);
	}
}

template<typename T,
	template<typename>class Alloc,
	template<typename, typename> class Con>
void printContainer(Con<T, Alloc<T>> &t) {
	for (size_t i = 0; i < t.size(); ++i)
		cout << i << " " << t[i] << endl;
}

void printCode(vector<string> &HC)
{
	cout << "Print codes" << endl;
	for (size_t i = 0; i < HC.size(); ++i)
	{
		cout << i << " " << HC[i].c_str() << endl;
	}
}

void printWeight(vector<int> &HC)
{
	cout << "Print weights" << endl;
	for (size_t i = 0; i < HC.size(); ++i)
	{
		cout << HC[i] << " ";
	}
	cout << "\n";
}

int main() {
	vector<HuffmanNode> HT;
	vector<string> HC;
	vector<int> w;
	int i, n;
	stringstream is("1 2 3 5 5 5 8");
	// stringstream is("5 29 7 8 14 23 3 11");

	cout << is.str().c_str() << endl;
	while (is >> i)
		w.push_back(i);

	n = w.size();

	HuffmanCoding(HT, HC, w, n);

	printContainer(HT);

	printCode(HC);

	w.clear();
	HuffmanDecode(HT, HC, w);

	printWeight(w);
}