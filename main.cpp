// CS3241 Assignment 2: Let there be light
#include <cmath>
#include <iostream>
#include <cstdlib>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include "GL/glut.h"
#elif __APPLE__
#include <OpenGL/gl.h>
#include <GLUT/GLUT.h>
#endif

using namespace std;

const double kPi = 3.14159265358979323846;

// global variable

bool m_Smooth = false;
bool m_Highlight = false;
GLfloat angle = 0;   /* in degrees */
GLfloat angle2 = 0;   /* in degrees */
GLfloat zoom = 1.0;
int mouseButton = 0;
int moving, startx, starty;

#define NO_OBJECT 4;
int current_object = 0;

using namespace std;

double dotProduct(double a[3], double b[3])
{
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

void crossProduct(double a[3], double b[3], double result[3])
{
    result[0] = a[1]*b[2] - a[2]*b[1];
    result[1] = a[2]*b[0] - a[0]*b[2];
    result[2] = a[0]*b[1] - a[1]*b[0];
}

void heartNormalize(double direction[3])
{
    double length = sqrt(dotProduct(direction, direction));
	if (length == 0.0f) return;
    for (int i = 0; i < 3; i++) direction[i] /= length;
}

void setHeartMaterial(double red, double green, double blue)
{
    GLfloat diffuse[4] = {red, green, blue, 1};
	GLfloat ambient[4] = {red * 0.38, green * 0.38, blue * 0.38, 1};
    GLfloat emission[] = {0, 0, 0, 1};

    glColor3d(red, green, blue);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);
}

// cubic Bezier interpolation of one coordinate using four control values
double cubicBezier(double a, double b, double c, double d, double t)
{
    double s = 1 - t;
    return s*s*s*a + 3*s*s*t*b + 3*s*t*t*c + t*t*t*d;
}

// t runs from the top to the tip; u runs around the body.
void heartBodyPoint(double t, double u, double point[3])
{
    double s = sin(t), c = cos(t);
    double lower = (1-c) / 2;
    double rib = 1 + 0.011*sin(13*u + 2.5*t)*s*s;		// muscle texture

    double grooveAngle = 1.04 + 0.19*t;
    double delta = atan2(sin(u-grooveAngle), cos(u-grooveAngle));
    double groove = 0.023*exp(-delta*delta / 0.012)*s;	

    point[0] = 0.88*s*(0.83 + 0.24*c)*cos(u)*rib + 0.24*lower*lower - groove*cos(u);
    point[1] = 1.18*c - 0.25;
    point[2] = 0.61*s*(0.92 + 0.08*c)*sin(u)*rib - groove*sin(u);
}

void heartBodyNormal(double t, double u, double normal[3])
{
    // nearby points give two tangents; use them to find point normal
    if (t < 0.001) t = 0.001;
    if (t > kPi-0.001) t = kPi-0.001;
    double left[3], right[3], above[3], below[3], around[3], down[3];
    heartBodyPoint(t, u-0.0005, left);
    heartBodyPoint(t, u+0.0005, right);
    heartBodyPoint(t-0.0005, u, above);
    heartBodyPoint(t+0.0005, u, below);
    for (int i = 0; i < 3; i++) {
        around[i] = right[i] - left[i];
        down[i] = below[i] - above[i];
    }
    crossProduct(around, down, normal);
    heartNormalize(normal);
}

void drawHeartBodyVertex(double t, double u)
{
    double point[3], normal[3];
    heartBodyPoint(t, u, point);
    heartBodyNormal(t, u, normal);
    glNormal3dv(normal);
    glVertex3dv(point);
}

void drawHeartBody()
{
    int rows = 48, columns = 72;
    glBegin(GL_TRIANGLES);
    for (int j = 0; j < rows; j++) {
        double t0 = kPi*j/rows, t1 = kPi*(j+1)/rows;
        for (int i = 0; i < columns; i++) {
            double u0 = 2*kPi*i/columns, u1 = 2*kPi*(i+1)/columns;
            drawHeartBodyVertex(t0, u0);
            drawHeartBodyVertex(t0, u1);
            drawHeartBodyVertex(t1, u1);
            drawHeartBodyVertex(t0, u0);
            drawHeartBodyVertex(t1, u1);
            drawHeartBodyVertex(t1, u0);
        }
    }
    glEnd();
}

void drawHeartEllipsoid(double x, double y, double z,
                        double width, double height, double depth,
                        double red, double green, double blue, double tilt)
{
    setHeartMaterial(red, green, blue);
    glPushMatrix();
    glTranslated(x, y, z);
    glRotated(tilt * 180 / kPi, 0, 0, 1);
    glScaled(width, height, depth);
    glutSolidSphere(1, 36, 24);
    glPopMatrix();
}

// draw one vertex on a circular cross section of a blood vessel
void drawHeartTubeVertex(double center[3], double tangent[3],
                         double across[3], double up[3], double radius,
                         double slope, double angle, double size, double sign)
{
    double point[3], normal[3];
    for (int i = 0; i < 3; i++) {
        double radial = across[i]*cos(angle) + up[i]*sin(angle);
        point[i] = center[i] + radial*radius*size;
        normal[i] = sign*(radial - tangent[i]*slope*size);
    }
    heartNormalize(normal);
    glNormal3dv(normal);
    glVertex3dv(point);
}

void drawHeartRimVertex(double center[3], double across[3], double up[3],
                        double radius, double angle)
{
    glVertex3d(center[0] + radius*(across[0]*cos(angle) + up[0]*sin(angle)),
               center[1] + radius*(across[1]*cos(angle) + up[1]*sin(angle)),
               center[2] + radius*(across[2]*cos(angle) + up[2]*sin(angle)));
}

// join up to 37 circular cross sections into one curved tube
void drawHeartTube(double points[][3], double radii[], int count,
                   double red, double green, double blue, bool hollow, int sides)
{
    if (count < 2 || count > 37) return;
    double tangent[37][3], across[37][3], up[37][3], slope[37];
    for (int i = 0; i < count; i++) {
        int before = i-1, after = i+1;
        if (before < 0) before = 0;
        if (after >= count) after = count-1;
        for (int k = 0; k < 3; k++)
            tangent[i][k] = points[after][k] - points[before][k];
        double distance = sqrt(dotProduct(tangent[i], tangent[i]));
        if (distance < 0.000001) distance = 0.000001;
        slope[i] = (radii[after] - radii[before]) / distance;
        heartNormalize(tangent[i]);

        // keep neighbouring circles aligned as the tube bends
        if (i > 0) {
            double projection = dotProduct(across[i-1], tangent[i]);
            for (int k = 0; k < 3; k++)
                across[i][k] = across[i-1][k] - projection*tangent[i][k];
        }
        if (i == 0 || dotProduct(across[i], across[i]) < 0.00000001) {
            double reference[] = {0, 1, 0};
            if (fabs(tangent[i][1]) >= 0.9) {
                reference[0] = 1; reference[1] = 0;
            }
            crossProduct(tangent[i], reference, across[i]);
        }
        heartNormalize(across[i]);
        crossProduct(tangent[i], across[i], up[i]);
    }

    int walls = 1;
    if (hollow) walls = 2;
    for (int wall = 0; wall < walls; wall++) {
        double size = 1, sign = 1, shade = 1;
        if (wall == 1) { size = 0.65; sign = -1; shade = 0.32; }
        setHeartMaterial(red*shade, green*shade, blue*shade);
        glBegin(GL_QUADS);
        for (int i = 0; i < count-1; i++) {
            for (int j = 0; j < sides; j++) {
                double a = 2*kPi*j/sides, b = 2*kPi*(j+1)/sides;
                int ring[] = {i, i, i+1, i+1};
                double angles[] = {a, b, b, a};
                for (int k = 0; k < 4; k++) {
                    int corner = k;
                    if (wall == 1) corner = 3-k;
                    int r = ring[corner];
                    drawHeartTubeVertex(points[r], tangent[r], across[r], up[r],
                                         radii[r], slope[r], angles[corner], size, sign);
                }
            }
        }
        glEnd();
    }

    // close the ends, leaving an opening when drawing a hollow vessel
    double shade = 1, inner = 0;
    if (hollow) { shade = 1.1; inner = 0.65; }
    setHeartMaterial(red*shade, green*shade, blue*shade);
    for (int end = 0; end < 2; end++) {
        int r = 0;
        double sign = -1;
        if (end == 1) { r = count-1; sign = 1; }
        glBegin(GL_QUADS);
        glNormal3d(sign*tangent[r][0], sign*tangent[r][1], sign*tangent[r][2]);
        for (int j = 0; j < sides; j++) {
            double a = 2*kPi*j/sides, b = 2*kPi*(j+1)/sides;
            if (end == 0) { double swap = a; a = b; b = swap; }
            drawHeartRimVertex(points[r], across[r], up[r], radii[r], a);
            drawHeartRimVertex(points[r], across[r], up[r], radii[r], b);
            drawHeartRimVertex(points[r], across[r], up[r], radii[r]*inner, b);
            drawHeartRimVertex(points[r], across[r], up[r], radii[r]*inner, a);
        }
        glEnd();
    }
}

// four control points describe the bend; the two radii control the taper
void drawHeartVessel(double x0, double y0, double z0,
                     double x1, double y1, double z1,
                     double x2, double y2, double z2,
                     double x3, double y3, double z3,
                     double startRadius, double endRadius,
                     double red, double green, double blue)
{
    double points[37][3], radii[37];
    for (int i = 0; i <= 36; i++) {
        double t = i / 36.0;
        points[i][0] = cubicBezier(x0, x1, x2, x3, t);
        points[i][1] = cubicBezier(y0, y1, y2, y3, t);
        points[i][2] = cubicBezier(z0, z1, z2, z3, t);
        radii[i] = startRadius + (endRadius-startRadius)*t;
    }
    drawHeartTube(points, radii, 37, red, green, blue, true, 20);
}

// surface vessels use (t,u) coordinates so they follow the heart's contour.
void drawHeartCoronary(double t0, double u0, double t1, double u1,
                       double t2, double u2, double t3, double u3,
                       double width, double red, double green, double blue)
{
    double points[33][3], radii[33];
    for (int i = 0; i <= 32; i++) {
        double fraction = i / 32.0;
        double t = cubicBezier(t0, t1, t2, t3, fraction);
        double u = cubicBezier(u0, u1, u2, u3, fraction);
        double normal[3];
        radii[i] = width*(1 - 0.72*fraction);
        heartBodyPoint(t, u, points[i]);
        heartBodyNormal(t, u, normal);
        for (int k = 0; k < 3; k++) points[i][k] += normal[k]*radii[i]*0.45;
    }
    drawHeartTube(points, radii, 33, red, green, blue, false, 10);
}


void drawCompositeHeart(float scale) {
    if(scale<=0) return;
    GLint oldMode; glGetIntegerv(GL_MATRIX_MODE,&oldMode);
    glPushAttrib(GL_ENABLE_BIT|GL_LIGHTING_BIT|GL_CURRENT_BIT);
    glDisable(GL_COLOR_MATERIAL); glEnable(GL_NORMALIZE); // preserve the S and H display settings.
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glScalef(scale,scale,scale);

    // Rear great vessels and descending aortic segment.
    drawHeartVessel(.30f,1.60f,-.12f,.67f,1.48f,-.27f,.42f,.75f,-.48f,
           .30f,.36f,-.45f,.18f,.16f,0.70, 0.29, 0.26);
    drawHeartVessel(-.49f,.58f,-.16f,-.63f,.91f,-.23f,-.54f,1.35f,-.25f,
           -.57f,1.55f,-.26f,.16f,.14f,0.66, 0.27, 0.25);
    drawHeartVessel(.38f,.61f,-.15f,.63f,.75f,-.10f,.83f,.97f,-.12f,
           .91f,1.12f,-.08f,.12f,.105f,0.70, 0.29, 0.26);
    drawHeartVessel(.46f,.40f,-.18f,.68f,.52f,-.15f,.91f,.67f,-.13f,
           1.02f,.78f,-.08f,.11f,.085f,0.70, 0.29, 0.26);

    setHeartMaterial(0.66, 0.27, 0.25); drawHeartBody();
    // overlapping atria and auricles make the upper silhouette irregular.
    drawHeartEllipsoid(-.40f,.56f,.03f,.37f,.40f,.37f,.64f,.25f,.24f,-.25f);
    drawHeartEllipsoid(.39f,.55f,.04f,.35f,.30f,.34f,.72f,.31f,.28f,.35f);
    drawHeartEllipsoid(-.38f,.52f,.32f,.24f,.31f,.16f,.69f,.28f,.26f,-.38f);
    drawHeartEllipsoid(.40f,.51f,.32f,.23f,.18f,.14f,.73f,.32f,.29f,.42f);

    // Aortic arch and its three upward branches.
    drawHeartVessel(-.30f,.59f,-.02f,-.83f,1.41f,-.09f,-.51f,1.90f,.02f,
           .30f,1.60f,-.12f,.22f,.18f,0.70, 0.29, 0.26);
    drawHeartVessel(-.42f,1.58f,0,-.52f,1.86f,0,-.63f,2.03f,.02f,
           -.70f,2.17f,.03f,.09f,.065f,0.70, 0.29, 0.26);
    drawHeartVessel(-.18f,1.70f,-.05f,-.21f,1.94f,-.04f,-.25f,2.17f,-.04f,
           -.36f,2.28f,-.03f,.105f,.082f,0.70, 0.29, 0.26);
    drawHeartVessel(.04f,1.69f,-.10f,.03f,1.99f,-.12f,.14f,2.24f,-.10f,
           .13f,2.43f,-.06f,.085f,.060f,0.70, 0.29, 0.26);
    drawHeartVessel(.075f,2.12f,-.09f,.21f,2.15f,-.10f,.24f,2.25f,-.08f,
           .26f,2.31f,-.06f,.048f,.034f,0.70, 0.29, 0.26);

    // Prominent pulmonary trunk, with a cut opening facing the viewer.
    drawHeartVessel(.11f,.58f,.26f,.18f,1.02f,.35f,-.02f,1.16f,.54f,
           -.20f,1.04f,.81f,.19f,.16f,.74f,.33f,.29f);
    drawHeartVessel(.12f,.90f,.30f,.43f,1.14f,.33f,.63f,1.05f,.26f,
           .74f,.84f,.20f,.13f,.11f,.72f,.30f,.27f);

    // coronary network follows the surface rather than floating above it
    drawHeartCoronary(.66f,.40f,.88f,1.12f,.90f,2.02f,.75f,2.75f,.037f,0.73, 0.34, 0.30);
    drawHeartCoronary(.84f,1.18f,1.35f,1.18f,2.05f,1.62f,2.78f,1.55f,.034f,0.51, 0.19, 0.18);
    drawHeartCoronary(1.12f,1.23f,1.30f,.91f,1.49f,.58f,1.86f,.44f,.021f,0.73, 0.34, 0.30);
    drawHeartCoronary(1.55f,1.38f,1.69f,1.02f,1.98f,.85f,2.31f,.68f,.019f,0.73, 0.34, 0.30);
    drawHeartCoronary(1.34f,1.30f,1.55f,1.78f,1.78f,2.09f,2.25f,2.18f,.023f,0.73, 0.34, 0.30);
    drawHeartCoronary(.91f,2.09f,1.22f,2.25f,1.88f,2.35f,2.38f,2.03f,.026f,0.51, 0.19, 0.18);
    drawHeartCoronary(1.42f,2.28f,1.58f,2.56f,1.71f,2.78f,1.98f,2.83f,.015f,0.73, 0.34, 0.30);
    // fine longitudinal muscle ridges, varied in length and curvature.
    for(int i=0;i<9;++i) {
        float u=.40f+i*.30f;
        drawHeartCoronary(1.04f,u,1.45f,u-.08f,2.02f,u+.12f,
                 2.50f-(i%3)*.12f,u+.18f,.0065f,.69f,.29f,.27f);
    }
    glPopMatrix(); glMatrixMode(oldMode); glPopAttrib();
}

void setupLighting()
{
	glShadeModel(GL_SMOOTH);
	glEnable(GL_NORMALIZE);

	// lights, material properties
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
	double theta = i * kPi / n;
	double phi = j * kPi / n;

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
		double u0 = 2.0 * kPi * i / around;
		double u1 = 2.0 * kPi * (i + 1) / around;
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
		// Center the heart, including its tall vessels, in the existing view.
		 glTranslatef(0.0f, -0.425f, 0.0f);
		 drawCompositeHeart(0.85f);
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
	cout << "1: Sphere, 2: Mobius strip, 3: Anatomical heart" << endl;
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
