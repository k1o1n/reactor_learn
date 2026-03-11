#include <iostream>
#include <vector>
#define ll long long 
#define N 200000
using namespace std;
ll n;
vector<ll> tree[N + 1]{};
void Dfs1(ll root, vector<ll> arr, vector<ll> brr) {
    vector<ll> Left, Right, L, R;
    if (arr.size() != 1) {
        tree[root].push_back(arr.back());
        ll pos;
        for (ll q = 0; q < brr.size(); ++q) {
            if (brr[q] == arr.back()) pos = q;
        }
        for (ll q = 0; q < pos; ++q) Left.push_back(arr[q]);
        for (ll q = pos; q + 1 < arr.size(); ++q) Right.push_back(arr[q]);
        for (ll q = 0; q < pos; ++q) L.push_back(brr[q]);
        for (ll q = pos + 1; q < brr.size(); ++q) R.push_back(brr[q]);
        Dfs1(arr.back(), Left, L);
        Dfs1(arr.back(), Right, R);
    }
    else tree[root].push_back(arr.back());
}
void Dfs2(ll root = 0) {
    cout << root << " ";
    for (auto it : tree[root]) Dfs2(it);
}
int main() {
    cin >> n;
    vector<ll> arr(n), brr(n);
    for (ll q = 0; q < n; ++q) cin >> arr[q];
    for (ll q = 0; q < n; ++q) cin >> brr[q];
    Dfs1(0, arr, brr);
    Dfs2();
    cout << endl;
}