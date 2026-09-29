#include <iostream>
#include <sstream>
#include <stack>
#include <queue>

using namespace std;

typedef struct BitTree {
	struct BitTree* left, *right;
	char c;
}BitTree,*PBitTree;

void CreateBitTree(stringstream &is, PBitTree *PBtn)
{
	char c;
	is >> c;
	if (c == '^')
		*PBtn = NULL;
	else
	{
		(*PBtn) = new BitTree;
		(*PBtn)->c = c;
		CreateBitTree(is, &((*PBtn)->left));
		CreateBitTree(is, &((*PBtn)->right));
	}
}
void DestroyBitTree(PBitTree PBtn) {
	if (PBtn)
	{
		PBitTree l = PBtn->left, r = PBtn->right;

		delete PBtn;
		DestroyBitTree(l);
		DestroyBitTree(r);
	}
}
void levelTraverse(PBitTree PBtn) {
	PBitTree t = PBtn;
	queue<PBitTree> q;

	q.push(t);
	while (!q.empty())
	{
		t = q.front();
		q.pop();

		cout << t->c << " ";

		if (t->left)q.push(t->left);
		if (t->right)q.push(t->right);
	}
}
int main(){
	stringstream is("abd^^^c^^");
	PBitTree PBtn;
	CreateBitTree(is, &PBtn);
	levelTraverse(PBtn);
	DestroyBitTree(PBtn);
}