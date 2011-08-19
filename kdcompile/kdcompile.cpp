#include <iostream>
#include <string>

using namespace std;

int main(int argc, char** argv)
{
    string path_in, path_out;

    cout << "lynx kdcompile\n";
    if(argc < 3)
    {
        cout << "Arguments: kdcompile input.obj output.lbsp" << endl;
        return -1;
    }

    path_in = argv[1];
    path_out = argv[2];

    cout << "Loading polygon soup from " << path_in << endl;
    cout << endl << "Writing binary output to " << path_out << endl;

    return 0;
}
