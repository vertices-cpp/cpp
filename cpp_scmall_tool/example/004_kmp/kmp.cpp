#include <iostream>
#include <string>
using namespace std;

const int maxNum = 255;

int shift[maxNum];

int Sunday(const string& text, const string& pattern) {
	int t_length = text.length();
	int p_length = pattern.length();

	// Default value: shift by m+1
	for (int i = 0; i < maxNum; i++) {
		shift[i] = p_length + 1;
	}

	// For each character in the pattern, record the last index where it appears.
	// The shift is the distance needed to move the character after the matched
	// suffix of the text to that position in the pattern.
	for (int i = 0; i < p_length; i++) {
		shift[pattern[i]] = p_length - i;
		cout << i << "  pattern[i] int:" << (int)pattern[i] << " pattern[i] char:" << pattern[i] << "-" << shift[pattern[i]] << "\n";
	}

	// Start position of the pattern in the text
	int i = 0;
	// Current matched position in the pattern
	int j;
	while (i <= t_length - p_length) {
		j = 0;
		cout << "-------------Start-------i: " << i << "-------" << endl;
		while ([&]() {cout << "i : " << i << ", j : " << j
			<< " compare( text[i + j] :" << text[i + j] << " )==( pattern[j]:" << pattern[j] << ")" << endl;
		return text[i + j] == pattern[j]; }()) {
			j++;
			// Match succeeded
			if (j >= p_length) {
				cout << "Match succeeded!" << endl;
				return i;
			}
		}
		cout << "i + j : " << i + j << " j:" << j << endl;
		cout << "Match failed! (text [i + j]: " << text[i + j] << ") != (pattern[j]:" << pattern[j] << ")" << endl;
		// Find the character in the text just after the current matched suffix
		// and its last occurrence in the pattern.
		// The shift is the distance from (pattern end + 1) to that position.
		int tPos = text[i + p_length];
		cout << "  i:" << i << "  (char)tPos:" << (char)tPos << "  shift[tPos]:" << shift[tPos] << endl;
		i += shift[tPos];
		cout << "i: " << i << endl;
		cout << "------------------------------------" << endl;
	}
	return -1;
}

int main()
{
	string text, pattern;
	// cout << "enter text:";
	text = "BBC ABCDAB ABCDABCDABDE";
	// cout << "enter pattern:";
	pattern = "ABCDABD";

	cout << Sunday(text, pattern) << endl;

	return 0;
}