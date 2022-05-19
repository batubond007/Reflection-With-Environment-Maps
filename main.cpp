#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>
#include <GL/glew.h>
//#include <OpenGL/gl3.h>   // The GL Header File
#include <GLFW/glfw3.h> // The GLFW header
#include <glm/glm.hpp> // GL Math library header
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> 
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Camera.h"

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

using namespace std;

GLuint gProgram[3];
int gWidth, gHeight;

GLint modelingMatrixLoc[3];
GLint viewingMatrixLoc[3];
GLint projectionMatrixLoc[3];
GLint eyePosLoc[3];

glm::mat4 projectionMatrix;
glm::mat4 viewingMatrix;
glm::mat4 modelingMatrix;
Camera cam;
double lastX, lastY;
double lastYaw, lastPitch;
glm::vec3 lastCamPos;


int activeProgramIndex = 0;

struct Vertex
{
    Vertex(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) { }
    GLfloat x, y, z;
};

struct Texture
{
    Texture(GLfloat inU, GLfloat inV) : u(inU), v(inV) { }
    GLfloat u, v;
};

struct Normal
{
    Normal(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) { }
    GLfloat x, y, z;
};

struct Face
{
	Face(int v[], int t[], int n[]) {
		vIndex[0] = v[0];
		vIndex[1] = v[1];
		vIndex[2] = v[2];
		tIndex[0] = t[0];
		tIndex[1] = t[1];
		tIndex[2] = t[2];
		nIndex[0] = n[0];
		nIndex[1] = n[1];
		nIndex[2] = n[2];
	}
    GLuint vIndex[3], tIndex[3], nIndex[3];
};

vector<Vertex> gVertices;
vector<Texture> gTextures;
vector<Normal> gNormals;
vector<Face> gFaces;

GLfloat* vertexData_1;
GLfloat* normalData_1;
GLuint* indexData_1;

size_t vertexSize_1;
size_t normalSize_1;
size_t indexSize_1;

GLfloat* vertexData_2;
GLfloat* normalData_2;
GLuint* indexData_2;

size_t vertexSize_2;
size_t normalSize_2;
size_t indexSize_2;

GLuint vao_opaque;
GLuint vao_teapot;
GLuint vao_skybox;


unsigned char** Textures;
int width, height, nrChannels;
unsigned int textureID;
unsigned int environmentTexture;

GLuint gVertexAttribBuffer, gIndexBuffer;
GLint gInVertexLoc, gInNormalLoc;
int gVertexDataSizeInBytes, gNormalDataSizeInBytes;

float deltaTime;
double prevTime = glfwGetTime();
static float angle = 0;

// Global Data
vector<std::string> faces   
{
    "right.jpg",
    "left.jpg",
    "top.jpg",
    "bottom.jpg",
    "front.jpg",
    "back.jpg"
};

float skyboxVertices[] =
{
    //   Coordinates
    -1.0f, -1.0f,  1.0f,//        7--------6
     1.0f, -1.0f,  1.0f,//       /|       /|
     1.0f, -1.0f, -1.0f,//      4--------5 |
    -1.0f, -1.0f, -1.0f,//      | |      | |
    -1.0f,  1.0f,  1.0f,//      | 3------|-2
     1.0f,  1.0f,  1.0f,//      |/       |/
     1.0f,  1.0f, -1.0f,//      0--------1
    -1.0f,  1.0f, -1.0f
};

unsigned int skyboxIndices[] =
{
    // Right
    1, 2, 6,
    6, 5, 1,
    // Left
    0, 4, 7,
    7, 3, 0,
    // Top
    4, 5, 6,
    6, 7, 4,
    // Bottom
    0, 3, 2,
    2, 1, 0,
    // Back
    0, 1, 5,
    5, 4, 0,
    // Front
    3, 7, 6,
    6, 2, 3
};

bool ParseObj(const string& fileName)
{
    fstream myfile;
    gVertices.clear();
    gNormals.clear();
    gFaces.clear();

    // Open the input 
    myfile.open(fileName.c_str(), std::ios::in);

    if (myfile.is_open())
    {
        string curLine;

        while (getline(myfile, curLine))
        {
            stringstream str(curLine);
            GLfloat c1, c2, c3;
            GLuint index[9];
            string tmp;

            if (curLine.length() >= 2)
            {
                if (curLine[0] == 'v')
                {
                    if (curLine[1] == 't') // texture
                    {
                        str >> tmp; // consume "vt"
                        str >> c1 >> c2;
                        gTextures.push_back(Texture(c1, c2));
                    }
                    else if (curLine[1] == 'n') // normal
                    {
                        str >> tmp; // consume "vn"
                        str >> c1 >> c2 >> c3;
                        gNormals.push_back(Normal(c1, c2, c3));
                    }
                    else // vertex
                    {
                        str >> tmp; // consume "v"
                        str >> c1 >> c2 >> c3;
                        gVertices.push_back(Vertex(c1, c2, c3));
                    }
                }
                else if (curLine[0] == 'f') // face
                {
                    str >> tmp; // consume "f"
					char c;
					int vIndex[3],  nIndex[3], tIndex[3];
					str >> vIndex[0]; str >> c >> c; // consume "//"
					str >> nIndex[0]; 
					str >> vIndex[1]; str >> c >> c; // consume "//"
					str >> nIndex[1]; 
					str >> vIndex[2]; str >> c >> c; // consume "//"
					str >> nIndex[2]; 

					assert(vIndex[0] == nIndex[0] &&
						   vIndex[1] == nIndex[1] &&
						   vIndex[2] == nIndex[2]); // a limitation for now

					// make indices start from 0
					for (int c = 0; c < 3; ++c)
					{
						vIndex[c] -= 1;
						nIndex[c] -= 1;
						tIndex[c] -= 1;
					}

                    gFaces.push_back(Face(vIndex, tIndex, nIndex));
                }
                else
                {
                    cout << "Ignoring unidentified line in obj file: " << curLine << endl;
                }
            }

            //data += curLine;
            if (!myfile.eof())
            {
                //data += "\n";
            }
        }

        myfile.close();
    }
    else
    {
        return false;
    }

	assert(gVertices.size() == gNormals.size());

    return true;
}

bool ReadDataFromFile(
    const string& fileName, ///< [in]  Name of the shader file
    string&       data)     ///< [out] The contents of the file
{
    fstream myfile;

    // Open the input 
    myfile.open(fileName.c_str(), std::ios::in);

    if (myfile.is_open())
    {
        string curLine;

        while (getline(myfile, curLine))
        {
            data += curLine;
            if (!myfile.eof())
            {
                data += "\n";
            }
        }

        myfile.close();
    }
    else
    {
        return false;
    }

    return true;
}

GLuint createVS(const char* shaderName)
{
    string shaderSource;

    string filename(shaderName);
    if (!ReadDataFromFile(filename, shaderSource))
    {
        cout << "Cannot find file name: " + filename << endl;
        exit(-1);
    }

    GLint length = shaderSource.length();
    const GLchar* shader = (const GLchar*) shaderSource.c_str();

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &shader, &length);
    glCompileShader(vs);

    char output[1024] = {0};
    glGetShaderInfoLog(vs, 1024, &length, output);
    printf("VS compile log: %s\n", output);

	return vs;
}

GLuint createFS(const char* shaderName)
{
    string shaderSource;

    string filename(shaderName);
    if (!ReadDataFromFile(filename, shaderSource))
    {
        cout << "Cannot find file name: " + filename << endl;
        exit(-1);
    }

    GLint length = shaderSource.length();
    const GLchar* shader = (const GLchar*) shaderSource.c_str();

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &shader, &length);
    glCompileShader(fs);

    char output[1024] = {0};
    glGetShaderInfoLog(fs, 1024, &length, output);
    printf("FS compile log: %s\n", output);

	return fs;
}

void initShaders()
{
	// Create the programs

    gProgram[0] = glCreateProgram();
	gProgram[1] = glCreateProgram();
	gProgram[2] = glCreateProgram();

	// Create the shaders for both programs

    GLuint vs1 = createVS("cubeVert.glsl");
    GLuint fs1 = createFS("cubeFrag.glsl");

	GLuint vs2 = createVS("vert2.glsl");
	GLuint fs2 = createFS("frag2.glsl");

    GLuint vs3 = createVS("mirrorVert.glsl");
    GLuint fs3 = createFS("mirrorFrag.glsl");

	// Attach the shaders to the programs

	glAttachShader(gProgram[0], vs1);
	glAttachShader(gProgram[0], fs1);

	glAttachShader(gProgram[1], vs2);
	glAttachShader(gProgram[1], fs2);

    glAttachShader(gProgram[2], vs3);
    glAttachShader(gProgram[2], fs3);

	// Link the programs

    glLinkProgram(gProgram[0]);
	GLint status;
	glGetProgramiv(gProgram[0], GL_LINK_STATUS, &status);

	if (status != GL_TRUE)
	{
		cout << "Program link failed" << endl;
		exit(-1);
	}

	glLinkProgram(gProgram[1]);
	glGetProgramiv(gProgram[1], GL_LINK_STATUS, &status);

	if (status != GL_TRUE)
	{
		cout << "Program link failed" << endl;
		exit(-1);
	}

    glLinkProgram(gProgram[2]);
    glGetProgramiv(gProgram[2], GL_LINK_STATUS, &status);

    if (status != GL_TRUE)
    {
        cout << "Program link failed" << endl;
        exit(-1);
    }

	// Get the locations of the uniform variables from both programs

	for (int i = 0; i < 3; ++i)
	{
		modelingMatrixLoc[i] = glGetUniformLocation(gProgram[i], "modelingMatrix");
		viewingMatrixLoc[i] = glGetUniformLocation(gProgram[i], "viewingMatrix");
		projectionMatrixLoc[i] = glGetUniformLocation(gProgram[i], "projectionMatrix");
		eyePosLoc[i] = glGetUniformLocation(gProgram[i], "eyePos");
	}
}

void CubeMapVBO() {
    unsigned int skyboxVBO, skyboxEBO;
    glGenVertexArrays(1, &vao_skybox);
    glGenBuffers(1, &skyboxVBO);
    glGenBuffers(1, &skyboxEBO);
    glBindVertexArray(vao_skybox);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), &skyboxIndices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void VBODataFiller1() {
    vertexData_1 = new GLfloat[gVertices.size() * 3];
    normalData_1 = new GLfloat[gNormals.size() * 3];
    indexData_1 = new GLuint[gFaces.size() * 3];

    for (int i = 0; i < gVertices.size(); ++i)
    {
        vertexData_1[3 * i] = gVertices[i].x;
        vertexData_1[3 * i + 1] = gVertices[i].y;
        vertexData_1[3 * i + 2] = gVertices[i].z;
    }

    for (int i = 0; i < gNormals.size(); ++i)
    {
        normalData_1[3 * i] = gNormals[i].x;
        normalData_1[3 * i + 1] = gNormals[i].y;
        normalData_1[3 * i + 2] = gNormals[i].z;
    }

    for (int i = 0; i < gFaces.size(); ++i)
    {
        indexData_1[3 * i] = gFaces[i].vIndex[0];
        indexData_1[3 * i + 1] = gFaces[i].vIndex[1];
        indexData_1[3 * i + 2] = gFaces[i].vIndex[2];
    }
    vertexSize_1 = gVertices.size();
    normalSize_1 = gNormals.size();
    indexSize_1 = gFaces.size();
}

void VBODataFiller2() {
    vertexData_2 = new GLfloat[gVertices.size() * 3];
    normalData_2 = new GLfloat[gNormals.size() * 3];
    indexData_2 = new GLuint[gFaces.size() * 3];

    for (int i = 0; i < gVertices.size(); ++i)
    {
        vertexData_2[3 * i] = gVertices[i].x;
        vertexData_2[3 * i + 1] = gVertices[i].y;
        vertexData_2[3 * i + 2] = gVertices[i].z;
    }

    for (int i = 0; i < gNormals.size(); ++i)
    {
        normalData_2[3 * i] = gNormals[i].x;
        normalData_2[3 * i + 1] = gNormals[i].y;
        normalData_2[3 * i + 2] = gNormals[i].z;
    }

    for (int i = 0; i < gFaces.size(); ++i)
    {
        indexData_2[3 * i] = gFaces[i].vIndex[0];
        indexData_2[3 * i + 1] = gFaces[i].vIndex[1];
        indexData_2[3 * i + 2] = gFaces[i].vIndex[2];
    }

    vertexSize_2 = gVertices.size();
    normalSize_2 = gNormals.size();
    indexSize_2 = gFaces.size();
}

void initVBO1()
{
    glGenVertexArrays(1, &vao_opaque);
    assert(vao_opaque > 0);
    glBindVertexArray(vao_opaque);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	assert(glGetError() == GL_NONE);

	glGenBuffers(1, &gVertexAttribBuffer);
	glGenBuffers(1, &gIndexBuffer);

	assert(gVertexAttribBuffer > 0 && gIndexBuffer > 0);

	glBindBuffer(GL_ARRAY_BUFFER, gVertexAttribBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer);

	gVertexDataSizeInBytes = vertexSize_1 * 3 * sizeof(GLfloat);
	gNormalDataSizeInBytes = normalSize_1 * 3 * sizeof(GLfloat);
	int indexDataSizeInBytes = indexSize_1 * 3 * sizeof(GLuint);
	
	glBufferData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes + gNormalDataSizeInBytes, 0, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, gVertexDataSizeInBytes, vertexData_1);
	glBufferSubData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes, gNormalDataSizeInBytes, normalData_1);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexDataSizeInBytes, indexData_1, GL_STATIC_DRAW);


	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(gVertexDataSizeInBytes));
}

void initVBO2()
{
    glGenVertexArrays(1, &vao_teapot);
    assert(vao_teapot > 0);
    glBindVertexArray(vao_teapot);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    assert(glGetError() == GL_NONE);

    glGenBuffers(1, &gVertexAttribBuffer);
    glGenBuffers(1, &gIndexBuffer);

    assert(gVertexAttribBuffer > 0 && gIndexBuffer > 0);

    glBindBuffer(GL_ARRAY_BUFFER, gVertexAttribBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer);

    gVertexDataSizeInBytes = vertexSize_2 * 3 * sizeof(GLfloat);
    gNormalDataSizeInBytes = normalSize_2 * 3 * sizeof(GLfloat);
    int indexDataSizeInBytes = indexSize_2 * 3 * sizeof(GLuint);

    glBufferData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes + gNormalDataSizeInBytes, 0, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, gVertexDataSizeInBytes, vertexData_2);
    glBufferSubData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes, gNormalDataSizeInBytes, normalData_2);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexDataSizeInBytes, indexData_2, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(gVertexDataSizeInBytes));
}

void initTextures() {
    Textures = new unsigned char* [6];

    for (int i = 0; i < 6; i++)
    {
        Textures[i] = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
    }

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    for (unsigned int i = 0; i < 6; i++)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, Textures[i]
        );
        stbi_image_free(Textures[i]);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &environmentTexture);

    glBindTexture(GL_TEXTURE_CUBE_MAP, environmentTexture);

    for (int i = 0; i < 6; i++)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0, GL_RGB, width, width, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL
        );
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void init() 
{
    ParseObj("obj/armadillo.obj");
    VBODataFiller1();
    initVBO1();
    ParseObj("obj/teapot.obj");
    VBODataFiller2();
    initVBO2();

    CubeMapVBO();

    glEnable(GL_DEPTH_TEST);
    initShaders();
    initTextures();
}

void drawModel()
{
	glBindBuffer(GL_ARRAY_BUFFER, gVertexAttribBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(gVertexDataSizeInBytes));

	glDrawElements(GL_TRIANGLES, gFaces.size() * 3, GL_UNSIGNED_INT, 0);
}

void DrawCubeMap()
{
    glm::mat4 view = glm::mat4(glm::mat3(cam.GetViewMatrix()));

    glUseProgram(gProgram[0]);
    glUniformMatrix4fv(projectionMatrixLoc[0], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    glUniformMatrix4fv(viewingMatrixLoc[0], 1, GL_FALSE, glm::value_ptr(view));

    glDepthMask(GL_FALSE);

    glBindVertexArray(vao_skybox);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
}

void DrawOpaque()
{
    float angleRad = (float)(angle / 180.0) * M_PI;

    // First Object

    float x1 = glm::sin(angleRad) * 5;
    float y1 = glm::cos(angleRad) * 5;

    glm::mat4 matT_1 = glm::translate(glm::mat4(1.0), glm::vec3(x1, y1, -20.0f));   // same as above but more clear
    glm::mat4 matRy_1 = glm::rotate<float>(glm::mat4(1.0), (-180. / 180.) * M_PI, glm::vec3(0.0, 1.0, 0.0));
    glm::mat4 matRz_1 = glm::rotate<float>(glm::mat4(1.0), angleRad, glm::vec3(0.0, 0.0, 1.0));
    modelingMatrix = matT_1 * matRy_1 * matRz_1;

    // Set the active program and the values of its uniform variables
    glUseProgram(gProgram[1]);
    glUniformMatrix4fv(projectionMatrixLoc[1], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    glUniformMatrix4fv(viewingMatrixLoc[1], 1, GL_FALSE, glm::value_ptr(cam.GetViewMatrix()));
    glUniformMatrix4fv(modelingMatrixLoc[1], 1, GL_FALSE, glm::value_ptr(modelingMatrix));
    glUniform3fv(eyePosLoc[1], 1, glm::value_ptr(cam.Position));

    glBindVertexArray(vao_opaque);
    glDrawElements(GL_TRIANGLES, indexSize_1 * 3, GL_UNSIGNED_INT, 0);

    // Second Object

    float x2 = glm::sin(angleRad ) * 10;
    float z2 = glm::cos(angleRad ) * 10 - 20;
    glm::mat4 matT_2 = glm::translate(glm::mat4(1.0), glm::vec3(x2, 0, z2));   // same as above but more clear
    glm::mat4 matRy_2 = glm::rotate<float>(glm::mat4(1.0), (-180. / 180.) * M_PI, glm::vec3(0.0, 1.0, 0.0));
    glm::mat4 matRy_21 = glm::rotate<float>(glm::mat4(1.0), angleRad * 2, glm::vec3(0.0, 1.0, 0.0));
    modelingMatrix = matT_2 * matRy_2 * matRy_21;


    // Set the active program and the values of its uniform variables

    glUseProgram(gProgram[1]);
    glUniformMatrix4fv(projectionMatrixLoc[1], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    glUniformMatrix4fv(viewingMatrixLoc[1], 1, GL_FALSE, glm::value_ptr(cam.GetViewMatrix()));
    glUniformMatrix4fv(modelingMatrixLoc[1], 1, GL_FALSE, glm::value_ptr(modelingMatrix));
    glUniform3fv(eyePosLoc[1], 1, glm::value_ptr(cam.Position));

    glBindVertexArray(vao_opaque);
    glDrawElements(GL_TRIANGLES, indexSize_1 * 3, GL_UNSIGNED_INT, 0);

    angle += 0.4f;
}

void SwitchCameraFace(int i) {
    if (i == 0)
    {
        //cam.SetRotation(0, 0);//Right
        cam.Right = glm::vec3(0, 0, 1);
        cam.Up = glm::vec3(0, -1, 0);
        cam.Front = glm::vec3(1, 0, 0);
    }
    else if (i == 1)
    {
        //cam.SetRotation(0, 180);//left
        cam.Right = glm::vec3(0, 0, -1);
        cam.Up = glm::vec3(0, -1, 0);
        cam.Front = glm::vec3(-1, 0, 0);
    }
    else if (i == 2)
    {
        //cam.SetRotation(90, 270);//top
        cam.Right = glm::vec3(1, 0, 0);
        cam.Up = glm::vec3(0, 0, 1);
        cam.Front = glm::vec3(0, 1, 0);
    }
    else if (i == 3)
    {
        //cam.SetRotation(-90, 270);//bot
        cam.Right = glm::vec3(1, 0, 0);
        cam.Up = glm::vec3(0, 0, -1);
        cam.Front = glm::vec3(0, -1, 0);
    }
    else if (i == 4)
    {
        //cam.SetRotation(0, 90);//front
        cam.Right = glm::vec3(1, 0, 0);
        cam.Up = glm::vec3(0, -1, 0);
        cam.Front = glm::vec3(0, 0, 1);
    }
    else if (i == 5)
    {
        //cam.SetRotation(0, -90);//back
        cam.Right = glm::vec3(-1, 0, 0);
        cam.Up = glm::vec3(0, -1, 0);
        cam.Front = glm::vec3(0, 0, -1);
    }

}

void DrawTeaPod() {
    unsigned int framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // Set the list of draw buffers.
    GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers

    // The depth buffer
    GLuint depthrenderbuffer;
    glGenRenderbuffers(1, &depthrenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, width);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);
    
    
    glViewport(0,0, width, width);
    lastYaw = cam.Yaw;
    lastPitch = cam.Pitch;
    lastCamPos = cam.Position;

    for (int i = 0; i < 6; i++)
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, environmentTexture, 0);
        SwitchCameraFace(i);
        cam.Position = glm::vec3(0, 0, -20);
        float fovyRad = (float)(90.0 / 180.0) * M_PI;
        projectionMatrix = glm::perspective(fovyRad, 1.0f, 1.0f, 100.0f);

        glClearColor(0, 0, 0, 1);
        glClearDepth(1.0f);
        glClearStencil(0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        DrawCubeMap();
        DrawOpaque();
        glTextureBarrier();
    }

    // Always check that our framebuffer is ok
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        return;

    glDeleteRenderbuffers(1, &framebuffer);
    glDeleteRenderbuffers(1, &depthrenderbuffer);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    ///////////////

    glClearColor(0, 0, 0, 1);
    glClearDepth(1.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    cam.SetRotation(lastPitch, lastYaw);
    cam.Position = lastCamPos;
    float fovyRad = (float)(45.0 / 180.0) * M_PI;
    projectionMatrix = glm::perspective(fovyRad, 1.0f, 1.0f, 100.0f);
    glViewport(0, 0, gWidth, gHeight);

    DrawCubeMap();
    DrawOpaque();

    modelingMatrix = glm::translate(glm::mat4(1.0), glm::vec3(0, 0, -20));
    glm::mat4 matRy = glm::rotate<float>(glm::mat4(1.0), (float)(angle / 180.0) * M_PI / 2, glm::vec3(0.0, 1.0, 0.0));
    modelingMatrix = modelingMatrix * matRy;

    glUseProgram(gProgram[2]);
    glUniformMatrix4fv(viewingMatrixLoc[2], 1, GL_FALSE, glm::value_ptr(cam.GetViewMatrix()));
    glUniformMatrix4fv(projectionMatrixLoc[2], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    glUniformMatrix4fv(modelingMatrixLoc[2], 1, GL_FALSE, glm::value_ptr(modelingMatrix));
    glUniform3fv(eyePosLoc[2], 1, glm::value_ptr(cam.Position));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, environmentTexture);

    glBindVertexArray(vao_teapot);
    glDrawElements(GL_TRIANGLES, indexSize_2 * 3, GL_UNSIGNED_INT, 0);
}

void reshape(GLFWwindow* window, int w, int h)
{
    w = w < 1 ? 1 : w;
    h = h < 1 ? 1 : h;

    gWidth = w;
    gHeight = h;

    glViewport(0, 0, w, h);

	float fovyRad = (float) (45.0 / 180.0) * M_PI;
	projectionMatrix = glm::perspective(fovyRad, 1.0f, 1.0f, 100.0f);

	viewingMatrix = glm::mat4(1);
}

void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    else if (key == GLFW_KEY_G && action == GLFW_PRESS)
    {
        //glShadeModel(GL_SMOOTH);
        activeProgramIndex = 0;
    }
    else if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        //glShadeModel(GL_SMOOTH);
        activeProgramIndex = 1;
    }
    else if (key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        //glShadeModel(GL_FLAT);
    }

    // MOVEMENTS
    else if (key == GLFW_KEY_W && action == GLFW_PRESS)
    {
        cam.ProcessKeyboard(FORWARD, deltaTime * 20);
    }
    else if (key == GLFW_KEY_S && action == GLFW_PRESS)
    {
        cam.ProcessKeyboard(BACKWARD, deltaTime * 20);
    }
    else if (key == GLFW_KEY_D && action == GLFW_PRESS)
    {
        cam.ProcessKeyboard(RIGHT, deltaTime * 20);
    }
    else if (key == GLFW_KEY_A && action == GLFW_PRESS)
    {
        cam.ProcessKeyboard(LEFT, deltaTime * 20);
    }
    else if (key == GLFW_KEY_Q && action == GLFW_PRESS)
    {
        cam.ProcessKeyboard(UP, deltaTime * 20);
    }
    else if (key == GLFW_KEY_E && action == GLFW_PRESS)
    {
        cam.ProcessKeyboard(DOWN, deltaTime * 20);
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
    if (state == GLFW_PRESS)
    {
        cam.ProcessMouseMovement(xpos - lastX, ypos - lastY);
    }
    lastX = xpos;
    lastY = ypos;
}

void CalculateTime() {
    deltaTime = glfwGetTime() - prevTime;
    prevTime = glfwGetTime();
}

void mainLoop(GLFWwindow* window)
{
    while (!glfwWindowShouldClose(window))
    {
        CalculateTime();
        DrawTeaPod();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

int main(int argc, char** argv)   // Create Main Function For Bringing It All Together
{
    GLFWwindow* window;
    if (!glfwInit())
    {
        exit(-1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    int width = 640, height = 480;
    window = glfwCreateWindow(width, height, "Simple Example", NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Initialize GLEW to setup the OpenGL Function pointers
    if (GLEW_OK != glewInit())
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return EXIT_FAILURE;
    }

    char rendererInfo[512] = {0};
    strcpy(rendererInfo, (const char*) glGetString(GL_RENDERER));
    strcat(rendererInfo, " - ");
    strcat(rendererInfo, (const char*) glGetString(GL_VERSION));
    glfwSetWindowTitle(window, rendererInfo);

    init();

    glfwSetKeyCallback(window, keyboard);
    glfwSetWindowSizeCallback(window, reshape);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    reshape(window, width, height); // need to call this once ourselves
    mainLoop(window); // this does not return unless the window is closed

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
