#include "threadpool.h"
#include <iostream>
using namespace std;
int main() {
    auto task = [](){
        cout << "hello!" << endl;
        return "world!";
    };
    auto fut = adachi::tool::ThreadPool::GetInstance().Submit(task); 
    cout << fut.get() << endl;
}