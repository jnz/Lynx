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

    cout << "Loading polygon soup from " << path_in << endl;
    tree.Load(path_in);
    cout << endl << "Writing binary output to " << path_out << endl;
    tree.WriteToBinary(path_out);

    return 0;
}