#include <ctime>
#include <algorithm>
#include <iostream>

#include "BoardBuilder.h"

BoardBuilder boardBuilder;

//utility function that clamps n between lower and upper. Should be put in a separate file to be called from
float clip(float n, float lower, float upper) 
{
	return std::max(lower, std::min(n, upper));
}

//this is the game loop thats passed into board builder upon initialization
void gameLoop()
{
	//alternate rgb values
	float rgb = clip(std::sin(std::clock() / 100), 0, 1);
	//this sets color of the background
	boardBuilder.setBackgroundColor(0, rgb, 0);
}

//this is the callback thats passed into board builder to receive mouse input
void inputCallBack(int column, int row, enum BoardBuilder::MOUSE_BUTTON_STATE state, enum BoardBuilder::MOUSE_BUTTON_TYPE type)
{
	//left mouse button
	if (type == BoardBuilder::LEFT)
	{
		//when press is released
		if (state == BoardBuilder::UP)
		{

		}
		else if (state == BoardBuilder::DOWN)//when initially pressed
		{
			//sets a circular piece at (column,row) with colors r,g,b
			boardBuilder.setPiece(1, (float)column / 10.0f, 1, column, row);
		}
	}
	else//middle or right button
	{
		if (state == BoardBuilder::UP)
		{
		}
		else
		{
			//removes piece at (column,row). Returns true if there exists a piece to be removed
			boardBuilder.removePiece(column, row);
			std::cout << type << "button pressed : " << column << "," << row << " DOWN " << std::endl;
		}
	}
}

int main(int argc, char **argv)
{
	//creates blank window
	boardBuilder.createWindow(argc, argv, 800, 800, "Checker board");
	//initializes board. Board goes from top left to bottom right. Ex. (0,0) -> (9,9) when grid width is 10
	boardBuilder.initBoard(10, gameLoop, inputCallBack);//grid width limit is 10
}