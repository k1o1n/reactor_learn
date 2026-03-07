#include <functional>
#include <iostream>
using namespace std;
int main() {
    function<void()> f;
    if (f) cout << 1 << endl;
    f = [](){};
    if (f) cout << 2 << endl;
}