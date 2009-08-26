#include <iostream>
#include "../lynx/BSPTree.h"
#include <string>

using namespace std;

int main(int argc, char** argv)
{
    string path_in, path_out;
    CBSPTree tree;

    if(argc < 3)
    {
        cout << "Argumensts: bspcompile input.obj output.lbsp" << endl;
        return -1;
    }

    path_in = argv[1];
    path_out = argv[2];

    tree.Load(path_in);
    tree.WriteToBinary(path_out);

    return 0;
}