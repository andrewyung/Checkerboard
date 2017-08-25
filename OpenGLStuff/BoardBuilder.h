#include <string>

class BoardBuilder
{
public:
	typedef enum MOUSE_BUTTON_STATE
	{
		UP,
		DOWN
	};

	typedef enum MOUSE_BUTTON_TYPE
	{
		LEFT,
		MIDDLE,
		RIGHT
	};

	int initBoard(int gridWidth, void (*gameLoopCallback)(), void (*inputCallback)(int column, int row, BoardBuilder::MOUSE_BUTTON_STATE state, BoardBuilder::MOUSE_BUTTON_TYPE type));
	void createWindow(int argc, char **argv, int height, int width, std::string windowName);

	void setBackgroundColor(float r, float g, float b);
	void setPiece(float r, float g, float b, int column, int row);
	bool removePiece(int column, int row);

private: 
	void redrawAllPieces();
};