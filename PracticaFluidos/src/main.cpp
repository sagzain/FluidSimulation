#include <stdlib.h>
#include <stdio.h>
#include <glut.h>
#include <cmath> 
#include "solver.h"

/* global variables */
static int N;
static float dt, diff, visc;
static float force, source;
static bool dvel;

static int win_id;
static int win_x, win_y;	//Window Size.
static int mouse_down[3];	//Mouse button states.
static int omx, omy, mx, my;

static Solver solver;

/*
----------------------------------------------------------------------
OpenGL specific drawing routines
----------------------------------------------------------------------
*/
static void PreDisplay(void)
{
	glViewport(0, 0, win_x, win_y);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, 1.0, 0.0, 1.0);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

static void PostDisplay(void)
{
	glutSwapBuffers();
}

static void DrawVelocity(void)
{
//TODO
	int i, j;
	float x, y, h, xv, yv;

	h = 1.0f / N;

	glLineWidth(1.0f);

	glBegin(GL_LINES);

	for (i = 1; i <= N; i++) {
		x = (i - 0.5f) * h;					//Dividing space by cell space.
		for (j = 1; j <= N; j++) {
			y = (j - 0.5f) * h;
			xv = solver.u[XY_TO_ARRAY(i, j)];
			yv = solver.v[XY_TO_ARRAY(i, j)];
			glColor3f((1 + (xv / std::abs(xv + yv))) * 0.5f, (1 + (yv / std::abs(xv + yv))) * 0.5f, 0.0f);
			glVertex2f(x, y);
			glVertex2f(x + xv, y + yv);
		}
	}

	glEnd();
}

static void DrawDensity(void)
{
//TODO
	int i, j;
	float x, y, h, d00, d01, d10, d11;

	h = 1.0f / N;

	glBegin(GL_QUADS);

	for (i = 0; i <= N; i++) {
		x = (i - 0.5f) * h;
		for (j = 0; j <= N; j++) {
			y = (j - 0.5f) * h;

			d00 = solver.dens[XY_TO_ARRAY(i, j)];
			d01 = solver.dens[XY_TO_ARRAY(i, j + 1)];
			d10 = solver.dens[XY_TO_ARRAY(i + 1, j)];
			d11 = solver.dens[XY_TO_ARRAY(i + 1, j + 1)];

			glColor3f(d00, d00, d00); glVertex2f(x, y);
			glColor3f(d10, d10, d10); glVertex2f(x + h, y);
			glColor3f(d11, d11, d11); glVertex2f(x + h, y + h);
			glColor3f(d01, d01, d01); glVertex2f(x, y + h);
		}
	}

	glEnd();
}

/*
----------------------------------------------------------------------
relates mouse movements to forces and sources
----------------------------------------------------------------------
*/
static void AddInteractionFromUI()
{
	int i, j;

	if (!mouse_down[0] && !mouse_down[2]) return;

	i = (int)((mx / (float)win_x)*N + 1);
	j = (int)(((win_y - my) / (float)win_y)*N + 1);

	if (i<1 || i>N || j<1 || j>N) return;

	if (mouse_down[GLUT_LEFT_BUTTON]) {
		solver.AddVelocity(i, j, force * (mx - omx), force * (omy - my));
	}

	if (mouse_down[GLUT_RIGHT_BUTTON]) {
		solver.AddDensity(i, j, source);
	}

	omx = mx;
	omy = my;

	return;
}

/*
----------------------------------------------------------------------
GLUT callback routines
----------------------------------------------------------------------
*/

static void KeyFunc(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 'c':
	case 'C':
		solver.ClearData();
		break;

	case 'q':
	case 'Q':
		solver.FreeData();
		exit(0);
		break;

	case 'v':
	case 'V':
		dvel = !dvel;
		break;
	}
}

static void MouseFunc(int button, int state, int x, int y)
{
	omx = mx = x;
	omx = my = y;

	mouse_down[button] = state == GLUT_DOWN;
}

static void MotionFunc(int x, int y)
{
	mx = x;
	my = y;
}

static void ReshapeFunc(int width, int height)
{
	glutSetWindow(win_id);
	glutReshapeWindow(width, height);

	win_x = width;
	win_y = height;
}

static void IdleFunc(void)
{
	solver.ClearPrevData(); //Clean last step forces
	AddInteractionFromUI();	//Add Forces and Densities

	solver.Solve();			//Calculate the next step

	glutSetWindow(win_id);
	glutPostRedisplay();
}

static void DisplayFunc(void)
{
	PreDisplay();

	if (dvel) DrawVelocity();
	else		DrawDensity();

	PostDisplay();
}


/*
----------------------------------------------------------------------
open_glut_window --- open a glut compatible window and set callbacks
----------------------------------------------------------------------
*/

static void OpenGlutWindow(void)
{
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);

	glutInitWindowPosition(0, 0);
	glutInitWindowSize(win_x, win_y);
	win_id = glutCreateWindow("Alias | wavefront");

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glutSwapBuffers();

	PreDisplay();

	glutKeyboardFunc(KeyFunc);
	glutMouseFunc(MouseFunc);
	glutMotionFunc(MotionFunc);
	glutReshapeFunc(ReshapeFunc);
	glutIdleFunc(IdleFunc);
	glutDisplayFunc(DisplayFunc);
}


/*
----------------------------------------------------------------------
main --- main routine
----------------------------------------------------------------------
*/

int main(int argc, char ** argv)
{
	glutInit(&argc, argv);

	if (argc != 1 && argc != 6) {
		fprintf(stderr, "usage : %s N dt diff visc force source\n", argv[0]);
		fprintf(stderr, "where:\n"); \
			fprintf(stderr, "\t N      : grid resolution\n");
		fprintf(stderr, "\t dt     : time step\n");
		fprintf(stderr, "\t diff   : diffusion rate of the density\n");
		fprintf(stderr, "\t visc   : viscosity of the fluid\n");
		fprintf(stderr, "\t force  : scales the mouse movement that generate a force\n");
		fprintf(stderr, "\t source : amount of density that will be deposited\n");
		exit(1);
	}

	if (argc == 1) {
		N = 64;
		dt = 0.1f;
		diff = 0.0001f;
		visc = 0.0f;
		force = 5.0f;
		source = 100.0f;
		fprintf(stderr, "Using defaults : N=%d dt=%g diff=%g visc=%g force = %g source=%g\n",
			N, dt, diff, visc, force, source);
	}
	else {
		N = atoi(argv[1]);
		dt = atof(argv[2]);
		diff = atof(argv[3]);
		visc = atof(argv[4]);
		force = atof(argv[5]);
		source = atof(argv[6]);
	}

	printf("\n\nHow to use this demo:\n\n");
	printf("\t Add densities with the right mouse button\n");
	printf("\t Add velocities with the left mouse button and dragging the mouse\n");
	printf("\t Toggle density/velocity display with the 'v' key\n");
	printf("\t Clear the simulation by pressing the 'c' key\n");
	printf("\t Quit by pressing the 'q' key\n");

	dvel = false;
	
	solver.Init(N, dt, diff, visc);

	if (!solver.AllocateData()) exit(1);

	win_x = 512;
	win_y = 512;
	OpenGlutWindow();

	glutMainLoop();

	exit(0);
}