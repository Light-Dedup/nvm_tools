#include <iostream>
#include <string>

using namespace std;

int main() {
	double x;
	string unit;

	cin >> x;
	cin >> unit;
	if (unit == "(nsec)") {
		cout << x;
	} else if(unit == "(usec)"){
        x = x * 1000;
        cout << x;
    } else if(unit == "(msec)"){
        x = x * 1000000;
        cout << x;
    } else {
		cerr << "Unrecognized unit " << unit << endl;
		return 1;
	}

	return 0;
}
