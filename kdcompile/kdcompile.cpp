#include <iostream>
#include <string>
#include "../src/KDTree.h"

using namespace std;

int main(int argc, char** argv)
{
    string path_in, path_out;
    CKDTree tree;

    cout << "lynx kdcompile\n";
    cout << "Compiling a WaveFront .obj to a binary Lynx level.\n";
    cout << "Copyright 2011 Jan Zwiener\n";
    if(argc < 3)
    {
        cout << "Arguments: kdcompile input.obj output.lbsp" << endl;
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
    cout << "Complete." << endl;
    system("pause");

    return 0;
}
