    #include <iostream>
	#include "Ai.h"
    using namespace std;

    int main() 
    {
    	for (int i = 0; i < 20; ++i)
    	{
			CAi ai(1, 7, 1);
			int moves[7] = {180, 161, 104, 200, 144, 198, 201};
			int newMove = ai.getMove(moves, 7);

	        cout << "Hello, World! " << newMove << "\n";
    	}

        return 0;
    }
