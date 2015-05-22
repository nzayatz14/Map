//============================================================================
// Name        : Map.cpp
// Author      : Nick Zayatz
// Version     :
// Copyright   : 3D Hawaiian Map Project
// Description : Draws a portion of Honolulu and lets you walk around with Arrow keys
//============================================================================

#include <iostream>
#include <vector>
#include <fstream>
#include <cstdlib>
#include "../include/Angel.h"

#define PI 3.1415926

using namespace std;

//shader stuff
GLint projLoc;
GLint mvLoc;
GLint scaleLoc;
GLint positionLoc;
GLint RDLoc;
GLint sunLoc;

//holds each point of the island, diamond, colors of each, and the normals of the island
int numPoints;  //number of iterations to generate the points
vec4 **heights;
vector<vec4> mapPoints;
vector<vec4> mapColors;
const int diamondPoints = 24;
vec4 diamond[diamondPoints];
vec4 diamondColors[diamondPoints];
vec4 *map_normals;

//world coordinates
const GLfloat wld_bot=-2.0, wld_top=2.0;
const GLfloat wld_left=-2.0, wld_right=2.0;
const GLfloat wld_near=0.002, wld_far=20.0;
GLfloat cwld_bot=-2.0, cwld_top=2.0;
GLfloat cwld_left=-2.0, cwld_right=2.0;
GLfloat cwld_near=0.002, cwld_far=20.0;

// Viewing transformation parameters
GLint width, height;
GLfloat theta = 0.0;
GLfloat phi = 0.0;
GLfloat tilt = 0.0;
GLfloat diamondAngle = 0.0;
const GLfloat distanceAboveZ = .05;

//file to be uploaded, and dividing factor
const char* openFile = "honolulu.raw";
const GLfloat divByXY = 150.0;
const GLfloat divByZ = 1500.0;

//identity matrix, diamond rotation matrix, and sun rotation matrix
const mat4 I = mat4(1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1);
mat4 RD = mat4(1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1);
mat4 RSun = mat4(1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1);

//starting light position and rotation amount for the sun
vec4 light_position( 0.5, -12.0, 0.0, 1.0 );
const GLfloat sunAngle = 0.001;

//rotation amount for the camera
const GLfloat  dr = 2.0 * DegreesToRadians;

//camera variables
mat4 p, mv;
vec4 eye;
vec4 at = vec4(0.0, 0.0, 0.0, 1.0);
vec4 up = vec4(0.0, 0.0, 1.0, 0.0);

//starting width and height of the window
int winwidth=512, winheight=512;


void myIdle();
void myReshape(GLint w, GLint h);
void myinit();
void initGLBuffer_and_shaders();
void display();
void keyboard( unsigned char key, int x, int y );
void readData();
double dist(vec2 p1, vec2 p2);
double dist4(vec4 p1, vec4 p2);
vec4 direction(vec4 start, vec4 end);
void pushColor(GLfloat elev);
void createDiamond();
void rotate(vec4 &one, vec4 &two, GLfloat angle);
void rotateDiamond();
vec4 * generateMapNormals();
void resetEye();
void rotateSun();
void printDirections();


int main( int argc, char *argv[] ) {

	readData();

	glutInit( &argc, argv );
	glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize( winwidth, winheight );

	glutCreateWindow( "Honolulu Walk Around" );

#ifndef __APPLE__
	glewExperimental = GL_TRUE;
	glewInit();
#endif

	printDirections();
	createDiamond();
	map_normals = generateMapNormals();
	initGLBuffer_and_shaders();
	myinit();

	glutDisplayFunc(display);
	glutReshapeFunc(myReshape);
	glutKeyboardFunc(keyboard);
	glutIdleFunc(myIdle);

	glutMainLoop();

	return 0;
}


/**
 * update the location of the camera, rotate the diamond, and rotate the light source
 * redraw the display
 *
 * :param: void
 * :returns: void
 */
void myIdle() {
	rotateDiamond();
	rotateSun();
	glutPostRedisplay();
}


/**
 * called when the window is resized.
 *
 * :param: w - new width of the window
 * :param: h - new height of the window
 * :returns: void
 */
void myReshape(GLint w, GLint h) {
	mat4 p;
	glViewport(0,0,w,h);
	winwidth = w;
	winheight = h;

	if(w<=h) {
		cwld_bot = wld_bot*(GLfloat)h/w;
		cwld_top = wld_top*(GLfloat)h/w;
	} else {
		cwld_left = wld_left*(GLfloat)w/h;
		cwld_right = wld_right*(GLfloat)w/h;
	}
	p = Ortho(cwld_left, cwld_right, cwld_bot, cwld_top, cwld_near, cwld_far);
	glUniformMatrix4fv(projLoc, 1, GL_TRUE, p);

}


/**
 * Sets up the initial camera view and perspective
 *
 * :param: void
 * :returns: void
 */
void myinit() {
	mat4 p, mv;
	GLfloat aspect,w,h;

	eye=vec4(3,.3,distanceAboveZ,1.0);
	at = vec4(2.5, 2.5, 0.0, 1.0);
	up = vec4(0.0, 0.0, 1.0, 0.0);

	theta  = PI/4;
	phi    = theta + PI;
	tilt = 0;

	glClearColor( 38/255.0, 194/255.0, 242/255.0, 0.5 );  //clear color is blue
	glColor3f(0.0,0.0,0.0);         //fill color set to black

	//adjust the aspects
	w=cwld_right-cwld_left;
	h=cwld_top-cwld_bot;

	if(w<=h)
		aspect = h/w;
	else
		aspect = w/h;

	/* set up standard orthogonal view with clipping box
  as cube of side 2 centered at origin
  This is default view and these statements could be removed. */
	p = Perspective(45.0,aspect, cwld_near, cwld_far);
	glUniformMatrix4fv(projLoc, 1, GL_TRUE, p);
	mv = LookAt(eye, at, up);
	glUniformMatrix4fv(mvLoc, 1, GL_TRUE, mv);
	glUniformMatrix4fv(RDLoc, 1, GL_FALSE, I);
}


/**
 * initializes aspects of the window, loads buffers, gets locations of parameters in the shader
 *
 * :param: void
 * :returns: void
 */
void initGLBuffer_and_shaders() {
	GLuint buffer;
	GLuint vao;
	GLuint program;
	GLint vLoc;
	GLint vColor;
	GLint vNormal;
	mat4 p;

	vec4 *points = new vec4[numPoints];
	vec4 *colors = new vec4[numPoints];

	for(int i =0;i<(int)mapPoints.size();i++){
		points[i] = mapPoints.at(i);
		colors[i] = mapColors.at(i);
	}

	// Create a vertex array object
#ifdef __APPLE__
	glGenVertexArraysAPPLE( 1, &vao );
	glBindVertexArrayAPPLE( vao );
#else
	glGenVertexArrays( 1, &vao );
	glBindVertexArray( vao );
#endif


	// Create and initialize a buffer object
	unsigned int vertsBytes = numPoints * sizeof(vec4);
	unsigned int colorBytes = numPoints*sizeof(vec4);
	unsigned int mapNormalBytes = numPoints*sizeof(vec4);
	unsigned int diamondBytes = diamondPoints*sizeof(vec4);
	unsigned int diamondColorBytes = diamondPoints*sizeof(vec4);
	unsigned int buffSize = vertsBytes + colorBytes + diamondBytes + diamondColorBytes + mapNormalBytes;
	glGenBuffers( 1, &buffer );
	glBindBuffer( GL_ARRAY_BUFFER, buffer );
	glBufferData( GL_ARRAY_BUFFER, buffSize, NULL , GL_STATIC_DRAW );
	glBufferSubData( GL_ARRAY_BUFFER, 0, vertsBytes, points );
	glBufferSubData( GL_ARRAY_BUFFER, vertsBytes, diamondBytes, diamond );
	glBufferSubData( GL_ARRAY_BUFFER, vertsBytes + diamondBytes, colorBytes, colors );
	glBufferSubData( GL_ARRAY_BUFFER, vertsBytes + colorBytes + diamondBytes, diamondColorBytes, diamondColors );
	glBufferSubData( GL_ARRAY_BUFFER, vertsBytes + colorBytes + diamondBytes + diamondColorBytes, mapNormalBytes, map_normals);

	// Load shaders and use the resulting shader program
	program = InitShader( "vshader.glsl", "fshader.glsl" );
	glUseProgram( program );

	//Get the locations of transformation matrices from the vshader
	projLoc = glGetUniformLocation(program, "p");
	mvLoc = glGetUniformLocation(program, "mv");
	RDLoc = glGetUniformLocation(program, "RD");
	//Get the locations of the scale and position from the vshader
	//These will be filled as the program runs, the vshader will
	//use these to values to correctly size and position the map.
	scaleLoc = glGetUniformLocation(program, "scale");
	positionLoc = glGetUniformLocation(program, "pos");
	sunLoc = glGetUniformLocation(program, "LightPosition");


	// Initialize the vertex position attribute from the vertex shader
	vLoc = glGetAttribLocation( program, "vPosition" );
	glEnableVertexAttribArray( vLoc );
	glVertexAttribPointer( vLoc, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );

	// Initialize the color position attribute from the vertex shader
	vColor = glGetAttribLocation( program, "vColor" );
	glEnableVertexAttribArray( vColor );
	glVertexAttribPointer( vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(numPoints*sizeof(vec4) + diamondPoints*sizeof(vec4)));


	// Initialize the normal position of each vertex
	vNormal = glGetAttribLocation( program, "vNormal" );
	glEnableVertexAttribArray( vNormal );
	glVertexAttribPointer( vNormal, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(2*numPoints*sizeof(vec4) + 2*diamondPoints*sizeof(vec4)));

	// Initialize shader lighting parameters
	vec4 light_ambient( 0.3, 0.3, 0.3, 1.0 );
	vec4 light_diffuse(0.3, 0.3, 0.3, 1.0 );
	vec4 light_specular( 0.3, 0.3, 0.3, 1.0 );
	vec4 material_ambient( 0.2, 0.0, 0.8, 1.0 );
	vec4 material_diffuse( 1.0, 0.8, 0.0, 1.0 );
	vec4 material_specular( 1.0, 0.0, 1.0, 1.0 );
	float material_shininess = 100.0;
	vec4 ambient_product = light_ambient * material_ambient;
	vec4 diffuse_product = light_diffuse * material_diffuse;
	vec4 specular_product = light_specular * material_specular;
	glUniform4fv( glGetUniformLocation(program, "AmbientProduct"), 1, ambient_product );
	glUniform4fv( glGetUniformLocation(program, "DiffuseProduct"), 1, diffuse_product );
	glUniform4fv( glGetUniformLocation(program, "SpecularProduct"), 1, specular_product );
	glUniform4fv( glGetUniformLocation(program, "LightPosition"), 1, light_position );
	glUniform1f( glGetUniformLocation(program, "Shininess"), material_shininess );

	//enable depth perception
	glEnable(GL_DEPTH_TEST);

	glClearColor( 38/255.0, 194/255.0, 242/255.0, 0.5 ); // blue background
}


/**
 * display function to redraw the display
 *
 * :param: void
 * :returns: void
 */
void display() {

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	//reload the light position
	glUniform4fv( sunLoc, 1, light_position );
	glUniformMatrix4fv(RDLoc, 1, GL_FALSE, I);

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);     // clear the window
	for(int i=0; i<numPoints; i+=3)
		glDrawArrays( GL_TRIANGLES, i, 3);    // draw the map points


	//set the rotation matrix for the diamond
	glUniformMatrix4fv(RDLoc, 1, GL_FALSE, RD);

	//draw the points for the diamond
	for(int i=numPoints; i<numPoints+diamondPoints; i+=3)
		glDrawArrays( GL_TRIANGLES, i, 3);

	glutSwapBuffers();
}


/**
 * Keyboard function to adjust camera movement
 *
 * :param: key - the key that was pressed
 * :param: x - the x-coordinate of the mouse at the time of the keyboard press
 * :param: y - the y-coordinate of the mouse at the time of the keyboard press
 * :returns: void
 *
 */
void keyboard( unsigned char key, int x, int y ) {

	GLfloat aspect,w,h;

	switch( key ) {
	case 033: // Escape Key
	case 'q': case 'Q':{
		exit( EXIT_SUCCESS );
		break;

		//I and K move the user forward and backward in the X-Y directions
	}case 'i': case 'I':{
		vec4 direct = direction(eye,at);
		eye[0] = eye[0] + (4.0/width)*direct[0];
		eye[1] = eye[1] + (4.0/height)*direct[1];
		at[0] = at[0] + (4.0/width)*direct[0];
		at[1] = at[1] + (4.0/height)*direct[1];

		break;
	}case 'k': case 'K':{
		vec4 direct = direction(eye,at);
		eye[0] = eye[0] - (4.0/width)*direct[0];
		eye[1] = eye[1] - (4.0/height)*direct[1];
		at[0] = at[0] - (4.0/width)*direct[0];
		at[1] = at[1] - (4.0/height)*direct[1];

		break;

		//J and L rotate the user around the point they are looking at
	}case 'j': case 'J':{
		theta -= dr;
		phi = theta+PI;
		rotate(at,eye,theta);

		break;
	}case 'l': case 'L':{
		theta += dr;
		phi = theta+PI;
		rotate(at,eye,theta);

		break;

		//A and D turn the user either left or right respectively
	}case 'a': case 'A':{
		phi += dr;
		theta = phi-PI;
		rotate(eye,at,phi);

		break;
	}case 'd': case 'D':{
		phi -= dr;
		theta = phi-PI;
		rotate(eye,at,phi);

		break;

		//W and S turn the camera up and down
	}case 's': case 'S':{
		tilt -= dr;
		at[2] = tilt;
		break;
	}case 'w': case 'W':{
		tilt += dr;
		at[2] = tilt;
		break;

	}case ' ':{  // reset values to their defaults
		resetEye();
		break;
	}default:
		break;
	}


	//adjust the z position so the camera always sits above the land that the user is standing on
	int zPos = (int)(eye[1]*divByXY);
	int zPos2 = (int)(eye[0]*divByXY);

	if((zPos >=0 && zPos < height) && (zPos2 >=0 && zPos2 < width)){
		eye[2] = heights[zPos][zPos2][2] + distanceAboveZ;
	}else{
		eye[2] = distanceAboveZ;
	}

	//adjust the aspects
	w=cwld_right-cwld_left;
	h=cwld_top-cwld_bot;

	if(w<=h)
		aspect = h/w;
	else
		aspect = w/h;

	//replace the camera and at
	p = Perspective(45.0,aspect,cwld_near, cwld_far);
	glUniformMatrix4fv(projLoc, 1, GL_TRUE, p);

	mv = LookAt(eye, at, up);
	glUniformMatrix4fv(mvLoc, 1, GL_TRUE, mv);

	//redraw the scene
	glutPostRedisplay();
}


/**
 * Reads in the map data into a vector
 *
 * :param: void
 * :returns: void
 */
void readData(){
	ifstream in;
	int elev;

	in.open(openFile);
	in>>width;
	in>>height;

	//set the x value light position based on the width of the file
	light_position.x = width/(divByXY*2);

	//create 2D array to read values into
	heights = new vec4*[height];

	for(int i = 0;i<height;i++){
		heights[i] = new vec4[width];
	}

	//load data into the 2D array
	for(int i = 0;i<height;i++){
		for(int j = 0;j<width;j++){
			in>>elev;
			heights[i][j] = vec4(j/divByXY,i/divByXY,elev/divByZ,1.0);
		}
	}

	in.close();

	//put the data in the 2D array into a vector to make the triangles along the surface
	for(int i = 0;i<height-1;i++){
		for(int j = 0;j<width-1;j++){
			mapPoints.push_back(heights[i][j]);
			mapPoints.push_back(heights[i][j+1]);
			mapPoints.push_back(heights[i+1][j]);

			mapPoints.push_back(heights[i+1][j]);
			mapPoints.push_back(heights[i][j+1]);
			mapPoints.push_back(heights[i+1][j+1]);

			pushColor(heights[i][j][2]);
			pushColor(heights[i+1][j][2]);
			pushColor(heights[i][j+1][2]);

			pushColor(heights[i+1][j][2]);
			pushColor(heights[i][j+1][2]);
			pushColor(heights[i+1][j+1][2]);

			numPoints+=6;
		}
	}
}


/**
 * Calculates the distance between 2 points.
 *
 * :param: p1 - the first point
 * :param: p2 - the second point
 * :returns: the distance between the points
 * */
double dist(vec2 p1, vec2 p2){
	return sqrt((p2.y-p1.y)*(p2.y-p1.y) + (p2.x-p1.x)*(p2.x-p1.x));
}


/**
 * Calculates the direction vector between 2 points.
 *
 * :param: start - the first point
 * :param: end - the second point
 * :returns: the normalized direction vector pointing from start to end
 * */
vec4 direction(vec4 start, vec4 end){
	vec4 direct = end-start;
	direct = direct/dist4(start, end);
	direct[3] = 1.0;

	return direct;
}


/**
 * Calculates the distance between 2 4D points.
 *
 * :param: p1 - the first point
 * :param: p2 - the second point
 * :returns: the distance between the points
 * */
double dist4(vec4 p1, vec4 p2){
	return sqrt((p2.y-p1.y)*(p2.y-p1.y) + (p2.x-p1.x)*(p2.x-p1.x) + (p2.z-p1.z)*(p2.z-p1.z));
}


/**
 * Adds a color to a point based on the elevation level of that point.
 *
 * :param: elev - the elevation of the point
 * :returns: void
 * */
void pushColor(GLfloat elev){

	elev = elev*divByZ;

	//if elev is 0, its water, if its 1-4 its sand, if its 1-199 its trees, if its 120-599 its mountain terrain, if its 600 or up its snow
	if(elev < 1){
		mapColors.push_back(vec4(0, 0, 207/255.0, 0.5));
	}else if (elev<5){
		mapColors.push_back(vec4(210/255.0, 180/255.0, 140/255.0,1.0));
	}else if (elev<200){
		mapColors.push_back(vec4(6/255.0, 112/255.0, 2/255.0,1.0));
	}else if (elev<600){
		mapColors.push_back(vec4(98/255.0, 66/255.0, 1/255.0,1.0));
	}else{
		mapColors.push_back(vec4(1.0,1.0,1.0,1.0));
	}
}


/**
 * Rotates point two about point 1
 *
 * :param: one - the first point
 * :param: two - the second point
 * :param: angle - the angle that is being adjusted by during the rotation
 * :returns: void
 * */
void rotate(vec4 &one, vec4 &two, GLfloat angle){
	vec4 direct = direction(one,two);
	vec4 oldOne = one;
	GLfloat dis = dist(vec2(one[0],one[1]), vec2(two[0],two[1]));

	direct = direct*dist4(two,one);
	direct[3] = 1;

	//move to origin
	one = vec4(0,0,0,1);
	two = direct;

	//perform rotation
	two[0] = dis*cos(angle);
	two[1] = dis*sin(angle);

	//get the new direction after the rotation and move the points accordingly in the new direction
	vec4 newDirect = direction(two,one);
	newDirect = newDirect*dist4(two,one);

	one = oldOne;
	two = one-newDirect;
	two[3] = 1;
}


/**
 * Generate the points needed for the diamond in the sky
 *
 * :param: void
 * :returns: void
 * */
void createDiamond(){
	vec4 pt[6];

	//beginning 6 points
	pt[0] = vec4(0,-.025, 1.025, 1);
	pt[1] = vec4(0, .025, 1.025, 1);
	pt[2] = vec4(0, .025, .975, 1);
	pt[3] = vec4(0, -.025, .975, 1);
	pt[4] = vec4(.075, 0, 1, 1);
	pt[5] = vec4(-.075, 0, 1, 1);

	int count = 0;

	//generate the triangles for the diamond so they are all facing outward
	for(int i = 0;i<diamondPoints;i++){
		if((i+1) % 3 == 2 && i<=11){
			diamond[i] = pt[4];
		}else if((i+1) % 3 == 0 && i>11){
			diamond[i] = pt[5];
		}else{
			diamond[i] = pt[count];
			if((i+1) % 3 == 1){
				count++;
				if (count>3)
					count = 0;
			}

		}
	}

	//generate random colors for each of the points
	for(int i = 0;i<diamondPoints;i++){
		int rand1 = rand()%256;
		int rand2 = rand()%256;
		int rand3 = rand()%256;
		diamondColors[i] = vec4(rand1/255.0,rand2/255.0,rand3/255.0,1.0);
	}
}


/**
 * Rotates the diamond about the Z axis
 *
 * :param: void
 * :returns: void
 * */
void rotateDiamond(){
	diamondAngle += .01;

	RD = mat4(cos(diamondAngle), -sin(diamondAngle), 0, 0,
			sin(diamondAngle), cos(diamondAngle), 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1);
}


/**
 * Generates the normals of the map for lighting
 *
 * :param: void
 * :returns: and array of normal vectors
 * */
vec4 * generateMapNormals() {
	vec4 *normals = new vec4[numPoints];
	for(int i=0;i<numPoints;i+=3){
		vec4 v = mapPoints.at(i+1) - mapPoints.at(i);
		vec4 w = mapPoints.at(i+2) - mapPoints.at(i);

		vec4 normTemp;

		//calculate the normal for each triangle
		normTemp.x = (v.y * w.z) - (v.z*w.y);
		normTemp.y = (v.z * w.x) - (v.x*w.z);
		normTemp.z = (v.x * w.y) - (v.y*w.x);

		normTemp = normTemp/(normTemp.x*normTemp.x + normTemp.y*normTemp.y + normTemp.z*normTemp.z);

		normals[i] = normTemp;
		normals[i+1] = normTemp;
		normals[i+2] = normTemp;
	}

	return normals;
}


/**
 * Resets the values of the sight.
 *
 * :param: void
 * :returns: void
 * */
void resetEye(){
	theta  = PI/4;
	phi    = theta + PI;
	tilt = 0;

	at[0] = width/divByXY - .5;
	at[1] = width/divByXY - .5;
	at[2] = 0;

	eye[0] = width/divByXY;
	eye[1] = width/divByXY;
	eye[2] = distanceAboveZ;
}


/**
 * Rotates the sun about the X axis
 *
 * :param: void
 * :returns: void
 * */
void rotateSun(){

	RSun = mat4(1, 0, 0, 0,
			0, cos(sunAngle), -sin(sunAngle), 0,
			0, sin(sunAngle), cos(sunAngle), 0,
			0, 0, 0, 1);

	light_position = RSun * light_position;


	//change the background color depending on the time of day
	if( light_position.z < -7.5){
		glClearColor( 0/255.0, 24/255.0, 72/255.0, 0.5 );
	}else if (light_position.z <-7.1){
		glClearColor( 48/255.0, 24/255.0, 96/255.0, 0.5 );
	}else if (light_position.z <-6.7){
		glClearColor( 72/255.0, 48/255.0, 120/255.0, 0.5 );
	}else if (light_position.z <-6.3){
		glClearColor( 96/255.0, 72/255.0, 120/255.0, 0.5 );
	}else if (light_position.z <-6){
		glClearColor( 144/255.0, 96/255.0, 144/255.0, 0.5 );
	}else if (light_position.z <-5.6){
		glClearColor( 164/255.0, 53/255.0, 186/255.0, 0.5 );
	}else if (light_position.z <-5.3){
		glClearColor( 186/255.0, 53/255.0, 133/255.0, 0.5 );
	}else if (light_position.z <-5){
		glClearColor( 237/255.0, 130/255.0, 98/255.0, 0.5 );
	}else{
		glClearColor( 38/255.0, 194/255.0, 242/255.0, 0.5 );
	}
}


/**
 * print the directions of how to operate the program
 *
 * :param: void
 * :returns: void
 */
void printDirections(){
	cout<<endl<<"Welcome to my Honolulu map!"<<endl;
	cout<<"Keyboard Controls:"<<endl;
	cout<<"I - Walk Forward"<<endl;
	cout<<"K - Walk Backward"<<endl;
	cout<<"J - Rotate Left"<<endl;
	cout<<"L - Rotate Right"<<endl<<endl;
	cout<<"W - Look Up"<<endl;
	cout<<"S - Look Down"<<endl;
	cout<<"A - Turn Left"<<endl;
	cout<<"D - Turn Right"<<endl<<endl;
	cout<<"Space Bar Resets the Scene"<<endl;
	cout<<"Q or Esc Quits the Program"<<endl;
}
