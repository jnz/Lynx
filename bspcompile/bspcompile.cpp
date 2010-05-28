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
    if(!tree.Load(path_in))
    {
        cout << "Failed to open input file" << endl;
        return -1;
    }
    cout << endl << "Writing binary output to " << path_out << endl;
    if(!tree.WriteToBinary(path_out))
    {
        cout << "Failed to open output file" << endl;
        return -1;
    }

    return 0;
}
