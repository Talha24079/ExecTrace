#include <iostream>
#include "btree/Node.hpp"

using namespace std;

int main() {
    cout << "--- B-Tree Project Test ---" << endl;

    Node myNode(1, true);

    myNode.keys.push_back(50);
    myNode.keys.push_back(30);
    myNode.keys.push_back(10);

    myNode.print_node();

    if (myNode.keys.size() <= (2 * DEGREE - 1)) {
        cout << "Node size is within limit." << endl;
    } else {
        cout << "Node overflow!" << endl;
    }

    return 0;
}