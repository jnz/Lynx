#include <iostream>
#include <string>
#include "../src/KDTree.h"

using namespace std;

int main(int argc, char** argv)
{
    string path_in, path_out;
    CKDTree tree;
    string path_lightmap;

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

    // the lightmap geometry needs to be identical with
    // the actual geometry from the input.obj, the reason
    // we have this, is that .obj files don't allow two
    // different texture coordinates assigned to a vertex.
    path_lightmap = "lightmap.obj";
    cout << "Lightmap geometry file: " << path_lightmap << endl;
    if(!tree.Load(path_in, path_lightmap))
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
#ifdef WIN32
    system("pause");
#endif

    return 0;
}

