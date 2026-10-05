// CS3241 Assignment 2: Let there be light
#include <cmath>
#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#include "GL/glut.h"
#define M_M_PI 3.141592654
#elif __APPLE__
#include <OpenGL/gl.h>
#include <GLUT/GLUT.h>
#endif

using namespace std;

// global variable

bool m_Smooth = FALSE;
bool m_Highlight = FALSE;
GLfloat angle = 0;   /* in degrees */
GLfloat angle2 = 0;   /* in degrees */
GLfloat zoom = 1.0;
int mouseButton = 0;
int moving, startx, starty;

#define NO_OBJECT 4;
int current_object = 0;

using namespace std;

void setupLighting()
{
	glShadeModel(GL_SMOOTH);
	glEnable(GL_NORMALIZE);

	// Lights, material properties
	GLfloat	ambientProperties[] = { 0.7f, 0.7f, 0.7f, 1.0f };
	GLfloat	diffuseProperties[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	GLfloat	specularProperties[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat lightPosition[] = { -100.0f,100.0f,100.0f,1.0f };

	glClearDepth(1.0);

	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientProperties);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseProperties);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specularProperties);
	glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, 0.0);

	// Default : lighting
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHTING);

}

void spherePoint(int i, int j, int n) {
	double theta = i * M_M_PI / n;
	double phi = j * M_M_PI / n;

	double x = sin(theta) * sin(phi);
	double y = cos(theta) * sin(phi);
	double z = cos(phi);

	glNormal3d(x,y,z);
	glVertex3d(x,y,z);
}

void drawSphere(double r)
{
	glScalef(r, r, r);
	glEnable(GL_NORMALIZE);		// ensure normals are unit length
	
	int i, j;
	int n = 20;
	for (i = 0; i < 2 * n; i++)
		for (j = 0; j < n; j++)
		{
			glBegin(GL_POLYGON);
			spherePoint(i, j, n);
			spherePoint(i + 1, j, n);
			spherePoint(i + 1, j + 1, n);
			spherePoint(i, j + 1, n);
			glEnd();
		}

}

// u goes around the ring; v goes across the strip's width.
void mobiusPoint(double u, double v, double radius)
{
	double c = cos(u), s = sin(u);
	double ch = cos(u / 2.0), sh = sin(u / 2.0);
	double r = radius + v * ch;

	// tangents dP/du and dP/dv; their cross product is the normal.
	double ux = -r * s - 0.5 * v * sh * c;
	double uy =  r * c - 0.5 * v * sh * s;
	double uz =  0.5 * v * ch;
	double vx = ch * c, vy = ch * s, vz = sh;
	glNormal3d(uy * vz - uz * vy,
		uz * vx - ux * vz, ux * vy - uy * vx);
	glVertex3d(r * c, r * s, v * sh);
}

void drawMobius(double radius, double halfWidth)
{
	int around = 100;
	int across = 16;

	// triangles used so that every rendered face is planar
	glBegin(GL_TRIANGLES);
	for (int i = 0; i < around; ++i) {
		double u0 = 2.0 * M_PI * i / around;
		double u1 = 2.0 * M_PI * (i + 1) / around;
		for (int j = 0; j < across; ++j) {
			double v0 = -halfWidth + 2.0 * halfWidth * j / across;
			double v1 = -halfWidth + 2.0 * halfWidth * (j + 1) / across;
			mobiusPoint(u0, v0, radius);
			mobiusPoint(u1, v0, radius);
			mobiusPoint(u1, v1, radius);
			mobiusPoint(u0, v0, radius);
			mobiusPoint(u1, v1, radius);
			mobiusPoint(u0, v1, radius);
		}
	}
	glEnd();
}

void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glShadeModel(m_Smooth ? GL_SMOOTH : GL_FLAT);
	
	GLfloat specularOn[]  = {1.0f, 1.0f, 1.0f, 1.0f};
	GLfloat specularOff[] = {0.0f, 0.0f, 0.0f, 1.0f};
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,
		m_Highlight ? specularOn : specularOff);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50.0f);
	
	// for Mobius strip: light both front and back-facing triangles
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, current_object == 1 ? GL_TRUE : GL_FALSE);

	glPushMatrix();
	glTranslatef(0, 0, -6);

	glRotatef(angle2, 1.0, 0.0, 0.0);
	glRotatef(angle, 0.0, 1.0, 0.0);

	glScalef(zoom, zoom, zoom);

	switch (current_object) {
	case 0:
		drawSphere(1);
		break;
	case 1:
		drawMobius(1.0, 0.4);
		break;
	case 2:
		// draw your first composite object here
		break;
	case 3:
		// draw your second composite object here
		break;
	default:
		break;
	};
	glPopMatrix();
	glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y)
{
	switch (key) {
	case 'p':
	case 'P':
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		break;
	case 'w':
	case 'W':
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		break;
	case 'v':
	case 'V':
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
		break;
	case 's':
	case 'S':
		m_Smooth = !m_Smooth;
		break;
	case 'h':
	case 'H':
		m_Highlight = !m_Highlight;
		break;

	case '1':
	case '2':
	case '3':
	case '4':
		current_object = key - '1';
		break;

	case 'Q':
	case 'q':
		exit(0);
		break;

	default:
		break;
	}

	glutPostRedisplay();
}



void mouse(int button, int state, int x, int y)
{
	if (state == GLUT_DOWN) {
		mouseButton = button;
		moving = 1;
		startx = x;
		starty = y;
	}
	if (state == GLUT_UP) {
		mouseButton = button;
		moving = 0;
	}
}

void motion(int x, int y)
{
	if (moving) {
		if (mouseButton == GLUT_LEFT_BUTTON)
		{
			angle = angle + (x - startx);
			angle2 = angle2 + (y - starty);
		}
		else zoom += ((y - starty) * 0.001);
		startx = x;
		starty = y;
		glutPostRedisplay();
	}

}

int main(int argc, char** argv)
{
	cout << "CS3241 Lab 3" << endl << endl;

	cout << "1-4: Draw different objects" << endl;
	cout << "1: Sphere, 2: Mobius strip" << endl;
	cout << "S: Toggle Smooth Shading" << endl;
	cout << "H: Toggle Highlight" << endl;
	cout << "W: Draw Wireframe" << endl;
	cout << "P: Draw Polygon" << endl;
	cout << "V: Draw Vertices" << endl;
	cout << "Q: Quit" << endl << endl;

	cout << "Left mouse click and drag: rotate the object" << endl;
	cout << "Right mouse click and drag: zooming" << endl;

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(600, 600);
	glutInitWindowPosition(50, 50);
	glutCreateWindow("CS3241 Assignment 3");
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glutDisplayFunc(display);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutKeyboardFunc(keyboard);
	setupLighting();
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	glMatrixMode(GL_PROJECTION);
	gluPerspective( /* field of view in degree */ 40.0,
		/* aspect ratio */ 1.0,
		/* Z near */ 1.0, /* Z far */ 80.0);
	glMatrixMode(GL_MODELVIEW);
	glutMainLoop();

	return 0;
}
