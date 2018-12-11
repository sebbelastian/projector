#include "ros/ros.h"
#include "std_msgs/String.h"
#include "geometry_msgs/Twist.h"
#include <GL/freeglut.h>
#include <stdlib.h>
#include <stdio.h>
#include <png.h>
#include <math.h>
#include "stb_image.h"
#include <string>
#include <iostream>
#include <fstream>
using namespace std;

//The direction of the robot movement
enum Direction {straight, sharpLeft, sharpRight, softLeft, softRight};
Direction direction = straight;	//The current direction of the robot

//coordinates for the rotation
GLfloat angleX, angleY, angleZ;

//coordinates for the translation 
GLdouble transX, transY, transZ;

//settings for the camera view (focal view)
GLdouble fW, fH, neardist, fardist;

//selections for transform request (via keyboard)
char transformChosen;//s=scale, t=translate, r=rotate
char axisChosen;	//which axis to transform: x/y/z

//For texture loading and changing
int nrTextures = 5;
GLuint texture[5];

void loadGlobalVar(void);
void saveGlobalVar(void);

//Initializations for OpenGL
void initWorld(void);
void initTexture(void);

//GLUT callback functions
void display(void);
void reshape(int w, int h);
void keyboard(unsigned char key, int x, int y);
void keyboardSpecial(int key, int x, int y);
void ReadMsgTimer(int value);

//ROS Callback func that handles the subscribed messages
void cmd_velCallback(const geometry_msgs::Twist::ConstPtr& msg);


int main(int argc, char **argv)
{
	//initialize ROS, subscribe to the cmd_vel topic and register a callback
	ros::init(argc, argv, "projector"); //this node is named "projector"
	ros::NodeHandle n;	//as long as this handle is alive, the node is also alive
	ros::Subscriber sub = n.subscribe("unicorn/cmd_vel", 1, cmd_velCallback);
	
	//initialize glut
	glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 800); //represent the projector's native resolution
    glutCreateWindow("projector"); //The title on the window
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE,GLUT_ACTION_CONTINUE_EXECUTION);

	//set the clearing color used in between frames
    glClearColor(0.0, 0.0, 0.0, 0.0);
    
    //register glut callback functions
    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(keyboardSpecial);
    glutTimerFunc(500, ReadMsgTimer, 0);

	//Load file to initialize global variables (for world and camera)
	
	
    //initialize the textures and the world
    initTexture();
	initWorld();
	
	//enter the glut loop that waits for events
    glutMainLoop();

	saveGlobalVar();
	
	return 0;
}

//ROS Callback func that handles the subscribed messages
void cmd_velCallback(const geometry_msgs::Twist::ConstPtr& msg)
{  
	float linear = msg->linear.x;
	float turn = msg->angular.z;
	
	if (linear > 0) //if moving forward
	{	
		if (turn < -0.2 && turn > -0.5)
		{
			ROS_INFO("z = [%f], turning soft right", msg->angular.z );	
			direction = softRight;
		}
		else if (turn > 0.2 && turn < 0.5)
		{
			ROS_INFO("z = [%f], turning soft left", msg->angular.z );	
			direction = softLeft;
		}
		else if (turn <= -0.5)
		{
			ROS_INFO("z = [%f], turning sharp right", msg->angular.z );	
			direction = sharpRight;
		}
		else if (turn >= 0.5)
		{
			ROS_INFO("z = [%f], turning sharp left", msg->angular.z );
			direction = sharpLeft;
		}
		else //if turn is too small, consider it as straight
		{
			ROS_INFO("x = [%f], moving straight", msg->linear.x );		
			direction = straight;
		}
	}
	
	glutPostRedisplay();
}

//Loads png images into memory and initializes textures with them
void initTexture(void)
{
    int width, height, nrChannels;
    char pic_0[] = "/home/lex/catkin_ws/src/UNICORN_2018/unicorn_projector/src//Textures/straight.png";
    char pic_1[] = "/home/lex/catkin_ws/src/UNICORN_2018/unicorn_projector/src//Textures/sharpLeft.png";
    char pic_2[] = "/home/lex/catkin_ws/src/UNICORN_2018/unicorn_projector/src//Textures/sharpRight.png";
    char pic_3[] = "/home/lex/catkin_ws/src/UNICORN_2018/unicorn_projector/src//Textures/softLeft.png";
    char pic_4[] = "/home/lex/catkin_ws/src/UNICORN_2018/unicorn_projector/src//Textures/softRight.png";
    
    char* pic_names[nrTextures];
    pic_names[0] = pic_0;
    pic_names[1] = pic_1;
    pic_names[2] = pic_2;
	pic_names[3] = pic_3;
	pic_names[4] = pic_4;
    
    unsigned char* pix_data[nrTextures];
    
    glGenTextures(3, texture);

	for (int i = 0; i < nrTextures; i++)
	{
	    glBindTexture(GL_TEXTURE_2D, texture[i]);
    
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	  
		//Without this, images are loaded up-side down
		stbi_set_flip_vertically_on_load(true);  
		pix_data[i] = stbi_load(pic_names[i], &width, &height, &nrChannels, 0);

		//if image has been loaded successfully
		if (pix_data[i])
		{
			if (nrChannels == 3)
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pix_data[i]);
			
			else if (nrChannels == 4)
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pix_data[i]);
			
				
			stbi_image_free(pix_data[i]);
		}
		else
		{
			ROS_INFO("ERR image %d not loaded!", i);
			exit(1);
		}
	}
	
    glEnable(GL_TEXTURE_2D);
    glShadeModel(GL_FLAT);
}
void saveGlobalVar(void)
{
	fstream myfile;
	myfile.open ("/home/lex/catkin_ws/src/UNICORN_2018/unicorn_projector/src/parameters.txt", ios::out|ios::trunc);
	if (myfile.is_open())
	{
		myfile << angleX << "\n";
		myfile << angleY << "\n";
		myfile << angleZ << "\n";
		myfile << transX << "\n";
		myfile << transY << "\n";
		myfile << transZ << "\n";
		myfile << neardist << "\n";
		myfile << fardist << "\n";
		myfile << transformChosen << "\n";
		myfile << axisChosen << "\n";
		
		myfile.close();
	}
	else
	{
		cout << "Unable to open the file!\n"; 
		exit (2);
	}
}

void loadGlobalVar(void)
{	
	string line;
  	fstream myfile ("/home/lex/catkin_ws/src/UNICORN_2018/unicorn_projector/src/parameters.txt", ios::in);
  	if (myfile.is_open())
  	{	
  		getline(myfile,line);
  		angleX = strtof(line.c_str(), 0);
  		  		cout << angleX << '\n';
  		getline(myfile,line);
		angleY = strtof(line.c_str(), 0);
		  		cout << angleY << '\n';
		getline(myfile,line);
		angleZ = strtof(line.c_str(), 0);
		  		cout << angleZ << '\n';
		getline(myfile,line);
		transX = strtod(line.c_str(), 0);
		  		cout << transX << '\n';
		getline(myfile,line);
		transY = strtod(line.c_str(), 0);
		  		cout << transY << '\n';
		getline(myfile,line);
		transZ = strtod(line.c_str(), 0);
		  		cout << transZ << '\n';
		getline(myfile,line);
		neardist = strtod(line.c_str(), 0);
				cout << neardist << '\n';
		getline(myfile,line);
		fardist = strtod(line.c_str(), 0);
				cout << fardist << '\n';
		getline(myfile,line);
		transformChosen = line.at(0);
				cout << transformChosen << '\n';
		getline(myfile,line);
		axisChosen = line.at(0);
				cout << axisChosen << '\n';
    	
    	myfile.close();
  	}

  	else
  	{
  		cout << "Unable to open the file!\n";
  		exit (2);
	}
}

void initWorld(void)
{	
	loadGlobalVar();	//Initializes the global variables
		
   	const GLdouble pi = 3.1415926535897932384626433832795;   
    int w=1280, h=800; 	//set width and hight of the camera view
    GLdouble fovY = 60; //set focal view of the camera in the Y direction
   	GLdouble aspect = double(w)/h; //the aspect ratio of the camera
   	
   	glMatrixMode(GL_PROJECTION);	//Changes made shall affect the projection (camera view)
    glLoadIdentity();
    
   	//gluPerspective(fovY, aspect, neardist, fardist); //an alternative to the glFrustum()
   		
    //convert the perspective parameters to fit the glFrustum()
    fH = tan(fovY/360*pi)*neardist;	//fH = tan((fovY / 2)/180*pi)*neardist;
    fW = fH * aspect;				//fW, fH are focal width and height
    
    //set the view of the camera
   	glFrustum(-fW, fW, -fH, fH, neardist, fardist);
    glMatrixMode(GL_MODELVIEW);		//done setting the camera view, switch back to the model view
}

void display(void)
{	
	//Change to the texture that represents the correct direction
	glBindTexture(GL_TEXTURE_2D, texture[direction]);
	
	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
 	glTranslatef(transX, transY, transZ); //MUST come BEFORE the rotation!
    glRotatef(angleX, 1,0,0);
    glRotatef(angleY, 0,1,0);
    glRotatef(angleZ, 0,0,1);
	
 	
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBegin(GL_POLYGON /*QUADS*/ );
        //(0,0) is the center of the view window
        glTexCoord2f(0.0, 0.0); glVertex3f( -1.0, -1.0, 0.0);//Bottom-left
        glTexCoord2f(1.0, 0.0); glVertex3f( 1.0, -1.0, 0.0); //Bottom-right
        glTexCoord2f(1.0, 1.0); glVertex3f( 1.0, 1.0, 0.0);  //Top-right
        glTexCoord2f(0.0, 1.0); glVertex3f( -1.0, 1.0, 0.0); //Top-left
    glEnd();
    glutSwapBuffers();
}

void ReadMsgTimer(int value)
{
	ros::spinOnce();	//take the ros messages from the message queue
	glutTimerFunc(500, ReadMsgTimer, 0); //reset the timer
}

void keyboardSpecial(int key, int x, int y)
{
	switch (transformChosen)
	{
	case 'r':
		switch (axisChosen)
		{
		case 'x':
			if (key == GLUT_KEY_UP)
				angleX += 0.5;
				
			else if (key == GLUT_KEY_DOWN)
				angleX -= 0.5;
			break;
		case 'y':
			if (key == GLUT_KEY_UP)
				angleY += 0.5;
			else if (key == GLUT_KEY_DOWN)
				angleY -= 0.5;
			break;
		case 'z':
			if (key == GLUT_KEY_UP)
				angleZ += 0.5;
			else if (key == GLUT_KEY_DOWN)
				angleZ -= 0.5;
			break;
		}
		break;
	case 't':
		switch (axisChosen)
		{
		case 'x':
			if (key == GLUT_KEY_UP)
				transX += 0.1;
			else if (key == GLUT_KEY_DOWN)
				transX -= 0.1;
			break;
		case 'y':
			if (key == GLUT_KEY_UP)
				transY += 0.1;
			else if (key == GLUT_KEY_DOWN)
				transY -= 0.1;
			break;
		case 'z':
			if (key == GLUT_KEY_UP)
				transZ += 0.1;
			else if (key == GLUT_KEY_DOWN)
				transZ -= 0.1;
			break;
		}
		break;
	}
	
	glutPostRedisplay(); //Trigger the display callback
}

void keyboard(unsigned char key, int x, int y)
{
	if (key == 'r' || key == 's' || key == 't')
		transformChosen = key;	
	
	else if (key == 'x' || key == 'y' || key == 'z')
		axisChosen = key;
	else if (key == 'q')
		glutLeaveMainLoop();
}

void reshape(int w, int h)
{
    glViewport(0, 0, w, h);
}
