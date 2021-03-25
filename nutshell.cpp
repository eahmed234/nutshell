#include <iostream>
#include "nutshell.tab.hpp"
using namespace std;

bool keepgoing = true;

int main()  {
    while (keepgoing) {
        cout << "> " << flush;
        yyparse();
    }
    cout << "Goodbye" << endl;
    return 0;
}