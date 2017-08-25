#include <GL\glew.h>
#include <GL\freeglut.h>
#include <string>
#include <iostream>
#include <vector>

#include "BoardBuilder.h"

//vertex shader
const std::string vertexString =
	"#version 330 core\n"
	"layout(location = 0) in vec3 inPosition;\n"
	"layout(location = 1) in vec3 inColor;\n"
	"out vec3 Color;\n"
	"void main()\n"
	"{\n"
	"gl_Position = vec4(inPosition, 1.0);\n"
	"Color = inColor;\n"
	"}\n\0"
;

//fragment shader
const std::string fragmentString =
	"#version 330 core\n"
	"in vec3 Color;\n"
	"out vec4 FragColor;\n"
	"\n"
	"void main()\n"
	"{\n"
	"FragColor = vec4(Color, 1.0f);\n"
	"}\n\0"
;

typedef void (*mouseInputCallback)(int column, int row, enum BoardBuilder::MOUSE_BUTTON_STATE state, enum BoardBuilder::MOUSE_BUTTON_TYPE type);

mouseInputCallback inputCall;

GLuint shaderProgram;

float windowHeight;
float windowWidth;

const int MAX_GRID_WIDTH = 10;
const float BORDER_RATIO = 0.05f;
const int CIRCLE_PRECISION = 30;

int setupGridWidth;
int gridIndicesSize;
int pieceIndicesSize;

float backgroundColor[3];

struct PlayPiece
{
	PlayPiece(int x, int y, float r, float g, float b)
	{
		this->y = y;
		this->x = x;
		this->r = r;
		this->g = g;
		this->b = b;
	}

	int y;
	int x;
	float r, g, b;
};

std::vector<PlayPiece> pieces;

GLuint VAO1, pieceVBO, pieceVAO, pieceEBO;

std::vector<float> generateGridSectionVertices(int gridNumber)
{
	float borderWidth = (2.0f * BORDER_RATIO) / (setupGridWidth + 1);//there is (n + 1) row and column borders for grid size n
	float boxWidth = (2.0f - (2.0f * BORDER_RATIO)) / setupGridWidth;

	int column = gridNumber % setupGridWidth;
	int row = floor(gridNumber / setupGridWidth);

	std::vector<float> vertices = {
		-1.0f + borderWidth + (column * boxWidth) + (column * borderWidth), -1.0f + borderWidth + (row * boxWidth) + (row * borderWidth), 0.0f,  // bottom left
		-1.0f + borderWidth + (column * boxWidth) + (column * borderWidth), -1.0f + borderWidth + (row * boxWidth) + (row * borderWidth) + boxWidth, 0.0f,  // top left
		-1.0f + borderWidth + (column * boxWidth) + (column * borderWidth) + boxWidth, -1.0f + borderWidth + (row * boxWidth) + (row * borderWidth), 0.0f,  // bottom right
		-1.0f + borderWidth + (column * boxWidth) + (column * borderWidth) + boxWidth, -1.0f + borderWidth + boxWidth + (row * boxWidth) + (row * borderWidth), 0.0f   // top right 
	};

	return vertices;
}

std::vector<float> generateCircleVertices(float x, float y, int precision)
{
	float borderWidth = (2.0f * BORDER_RATIO) / (setupGridWidth + 1);//there is (n + 1) row and column borders for grid size n

	//change back to range of -1, 1
	x = ((x * 2.0f) / glutGet(GLUT_WINDOW_WIDTH)) - 1.0f;
	y = ((y * 2.0f) / glutGet(GLUT_WINDOW_HEIGHT)) - 1.0f;
	
	GLfloat twicePi = 2.0f * 3.14159626f;

	std::vector<float> vertices = {
		x, y, 0
	};

	for (int i = 3; i <= (1 + precision) * 3; i += 3) {
		vertices.push_back(x + ((1.0f / (float)setupGridWidth) * cos((i/3) *  twicePi / precision)));
		vertices.push_back(y + ((1.0f / (float)setupGridWidth) * sin((i / 3) * twicePi / precision)) - (borderWidth / 2.0f));

		vertices.push_back(0);
	}

	return vertices;
}

void renderScene(void)
{
	glDisable(GL_DEPTH_TEST);//2d so disable depth buffer
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(backgroundColor[0], backgroundColor[1], backgroundColor[2], 1.0);

	glUseProgram(shaderProgram);
	
	glBindVertexArray(VAO1);
	glDrawElements(GL_TRIANGLES, gridIndicesSize, GL_UNSIGNED_INT, 0);
	
	glBindVertexArray(pieceVAO);
	glDrawElements(GL_TRIANGLES, pieceIndicesSize, GL_UNSIGNED_INT, 0);

	glutSwapBuffers();

	//clear vao binds
	glBindVertexArray(0);
}

void myMouseFunc(int button, int state, int x, int y)
{
	//determine row and columns and store back into x and y
	x = (int)floor((float)x / ((float)(glutGet(GLUT_WINDOW_WIDTH)) / (float)setupGridWidth));
	y = (int)floor((float)y / ((float)(glutGet(GLUT_WINDOW_HEIGHT)) / (float)setupGridWidth));

	switch (button)
	{
		case GLUT_LEFT_BUTTON:
			if (state == GLUT_DOWN) // LEFT-BUTTON DOWN
			{
				inputCall(x, y, BoardBuilder::DOWN, BoardBuilder::LEFT);
			}
			else if (state == GLUT_UP) // LEFT-BUTTON UP
			{
				inputCall(x, y, BoardBuilder::UP, BoardBuilder::LEFT);
			}
			break;
		case GLUT_RIGHT_BUTTON:
			if (state == GLUT_DOWN)
			{
				inputCall(x, y, BoardBuilder::DOWN, BoardBuilder::RIGHT);
			}
			else if (state == GLUT_UP)
			{
				inputCall(x, y, BoardBuilder::UP, BoardBuilder::RIGHT);
			}
			break;
		case GLUT_MIDDLE_BUTTON:
			if (state == GLUT_DOWN)
			{
				inputCall(x, y, BoardBuilder::DOWN, BoardBuilder::MIDDLE);
			}
			else if (state == GLUT_UP)
			{
				inputCall(x, y, BoardBuilder::UP, BoardBuilder::MIDDLE);
			}
			break;
	};
}



void BoardBuilder::setBackgroundColor(float r, float g, float b)
{
	backgroundColor[0] = r;
	backgroundColor[1] = g;
	backgroundColor[2] = b;

	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(backgroundColor[0], backgroundColor[1], backgroundColor[2], 1.0);

	glutPostRedisplay();
}

/*
* returns index of piece at column row. Otherwise -1 is returned;
*/
int pieceExistsAt(int column, int row)
{
	int negatedRow = setupGridWidth - row;
	for (int i = 0; i < pieces.size(); i++)
	{
		if ((int)floor((float)pieces[i].x / ((float)(glutGet(GLUT_WINDOW_WIDTH)) / (float)setupGridWidth)) == column &&
			((int)ceil((float)pieces[i].y / ((float)(glutGet(GLUT_WINDOW_HEIGHT)) / (float)setupGridWidth))) == negatedRow)
		{
			std::cout << "found" << std::endl;
			return i;
			break;
		}
	}
	return -1;
}

void BoardBuilder::setPiece(float r, float g, float b, int column, int row)
{
	float borderWidth = (2.0f * BORDER_RATIO) / (setupGridWidth + 1) * glutGet(GLUT_WINDOW_WIDTH);//there is (n + 1) row and column borders for grid size n
	float boxWidth = (((2.0f - (2.0f * BORDER_RATIO)) / setupGridWidth) / 2) * glutGet(GLUT_WINDOW_WIDTH);

	int negatedRow = setupGridWidth - row;
	float x = (column * (glutGet(GLUT_WINDOW_WIDTH) / setupGridWidth)) + (boxWidth / 2) + (borderWidth / 4);

	//to the nearest column minus half of a box width plus a border width
	float y = (negatedRow * (glutGet(GLUT_WINDOW_HEIGHT) / setupGridWidth)) - (glutGet(GLUT_WINDOW_HEIGHT) / setupGridWidth / 2) + ((borderWidth / (setupGridWidth + 1)));

	removePiece(column, row);

	pieces.push_back(PlayPiece(x, y, r, g, b));

	redrawAllPieces();
}

bool BoardBuilder::removePiece(int column, int row)
{
	int atIndex;
	if ((atIndex = pieceExistsAt(column, row)) != -1)
	{
		pieces.erase(pieces.begin() + atIndex);

		redrawAllPieces();

		return true;
	}
	return false;
}

void BoardBuilder::redrawAllPieces()
{
	glBindVertexArray(pieceVAO);

	if (pieces.empty())
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pieceEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);//passing in vector(&el[0]) doesnt work for ebo? no indices are sent for rendering

		return;
	}

	std::vector<float> allVertices;
	allVertices.reserve(pieces.size() * (2 + CIRCLE_PRECISION) * 3 * 2);
	for (int k = 0; k < pieces.size(); k++)
	{
		std::vector<float> vertices = generateCircleVertices(pieces[k].x, pieces[k].y, CIRCLE_PRECISION);

		for (int i = vertices.size(); i > 0; i -= 3)
		{
			vertices.insert(vertices.begin() + i, pieces[k].b);
			vertices.insert(vertices.begin() + i, pieces[k].g);
			vertices.insert(vertices.begin() + i, pieces[k].r);
		}

		allVertices.insert(allVertices.end(), vertices.begin(), vertices.end());
	}

	//std::cout << std::endl << allVertices.size() << std::endl;
	glBindBuffer(GL_ARRAY_BUFFER, pieceVBO);
	glBufferData(GL_ARRAY_BUFFER, allVertices.size() * sizeof(float), &allVertices[0], GL_STATIC_DRAW);

	//interpretation
	//position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);
	glEnableVertexAttribArray(0);
	//color
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));//3 offset in bytes with a stride of 6 in bytes
	glEnableVertexAttribArray(1);

	std::vector<float> indices;
	indices.reserve(CIRCLE_PRECISION * 3 * pieces.size());
	for (int k = 0; k < pieces.size(); k++)
	{
		for (int i = 0; i < CIRCLE_PRECISION * 3; i += 3)
		{
			indices.push_back(k * (CIRCLE_PRECISION + 2));//center
			indices.push_back((i / 3) + 1 + (k * (CIRCLE_PRECISION + 2)));
			indices.push_back((i / 3) + 2 + (k * (CIRCLE_PRECISION + 2)));
		}
	}

	unsigned int indicesArray[CIRCLE_PRECISION * 3 * MAX_GRID_WIDTH * MAX_GRID_WIDTH];

	for (int i = 0; i < indices.size(); i++)
	{
		indicesArray[i] = indices[i];
	}

	pieceIndicesSize = indices.size();

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pieceEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesArray), indicesArray, GL_STATIC_DRAW);//passing in vector(&el[0]) doesnt work for ebo? no indices are sent for rendering

	glutPostRedisplay();
}

void resize(int width, int height) {
	// force window size
	glutReshapeWindow(windowWidth, windowHeight);
}

void BoardBuilder::createWindow(int argc, char **argv, int height, int width, std::string windowName)
{
	windowHeight = height;
	windowWidth = width;

	//create window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition((GLUT_SCREEN_WIDTH / 2) - (width / 2), (GLUT_SCREEN_HEIGHT / 2) - (height / 2));
	glutInitWindowSize(width, height);

	glutCreateWindow(windowName.c_str());

	glewInit();
	if (glewIsSupported("GL_VERSION_4_5"))
	{
		std::cout << " GLEW Version is 4.5\n ";
	}
	else
	{
		std::cout << "GLEW 4.5 not supported\n ";
	}

	glutReshapeFunc(resize);
}

int BoardBuilder::initBoard(int gridWidth, void (*gameLoopCallback)(), void(*inputCallback)(int column, int row, enum BoardBuilder::MOUSE_BUTTON_STATE state, enum BoardBuilder::MOUSE_BUTTON_TYPE type))
{
	setupGridWidth = gridWidth;

	const GLchar *vertexSource = vertexString.c_str();
	const GLchar *fragmentSource = fragmentString.c_str();

	GLuint vertexShader;
	vertexShader = glCreateShader(GL_VERTEX_SHADER);

	GLuint fragmentShader;
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);

	int  success;
		
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	//shader compile error check
	if (!success)
	{
		char infoLog[512];
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "Vertex compilation error: \n" << infoLog << std::endl;

		return 1;
	}

	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);

	glGetProgramiv(fragmentShader, GL_LINK_STATUS, &success);
	//shader compile error check
	if (!success)
	{
		char infoLog[512];
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "Fragment compilation error: \n" << infoLog << std::endl;

		return 1;
	}

	shaderProgram = glCreateProgram();

	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	//shader program compile error check
	if (!success)
	{
		char infoLog[512];
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "Shader program linking error: \n" << infoLog << std::endl;

		return 1;
	}

	glUseProgram(shaderProgram);

	//cleanup shader objects now that they're linked with shader program
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	//setup vertex buffer object (vertices in buffer for opengl)
	GLuint VBO1, EBO1;
	
	glGenVertexArrays(1, &VAO1);
	glGenBuffers(1, &EBO1);
	glGenBuffers(1, &VBO1);

	glBindVertexArray(VAO1);

	std::vector<float> verticeArray;

	for (int i = 0; i < (gridWidth * gridWidth); i++)
	{
		std::vector<float> vertices = generateGridSectionVertices(i);
		for (int k = 0; k < 12 * 2; k += 6)
		{
			//position
			verticeArray.push_back(vertices[(k / 2)]);
			verticeArray.push_back(vertices[(k / 2) + 1]);
			verticeArray.push_back(vertices[(k / 2) + 2]);
			//color	
			verticeArray.push_back(((i + ((i / gridWidth) % 2)) % 2) - 0.3f);
			verticeArray.push_back(((i + ((i / gridWidth) % 2)) % 2) - 0.3f);
			verticeArray.push_back(((i + ((i / gridWidth) % 2)) % 2) - 0.3f);//checker board pattern
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, VBO1);
	glBufferData(GL_ARRAY_BUFFER, verticeArray.size() * sizeof(float), &verticeArray[0], GL_STATIC_DRAW);
	//interpretation data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	unsigned int indices[MAX_GRID_WIDTH * MAX_GRID_WIDTH * 6];

	int indexNumber = 0;
	for (int i = 0; i < gridWidth * gridWidth * 6; i += 6)
	{
		indices[i] = indexNumber++;

		indices[i + 1] = indexNumber;
		indices[i + 3] = indexNumber++;

		indices[i + 2] = indexNumber;
		indices[i + 4] = indexNumber++;

		indices[i + 5] = indexNumber++;

		//example of indices
		//0, 1, 2,
		//1, 2, 3
		
	};

	gridIndicesSize = gridWidth * gridWidth * 6;

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO1);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);


	//second VBO
	glGenVertexArrays(1, &pieceVAO);
	glGenBuffers(1, &pieceVBO);
	glGenBuffers(1, &pieceEBO);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	//mouse input callback
	glutMouseFunc(myMouseFunc);

	//general callbacks
	glutDisplayFunc(renderScene);

	//idle callbacks used as game loop for event driven input
	glutIdleFunc(gameLoopCallback);

	//set input callback
	inputCall = inputCallback;

	glutMainLoop();

	return 0;
}
