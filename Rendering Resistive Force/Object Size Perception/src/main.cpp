#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
//#include <826api.h>
#include <GL/glut.h>
#include <process.h>
#include "geometry_helper.h"
#include "display_helper.h"
#include "CExp_Size_Perception.h"
#include "phantom_helper.h"

#define _TITLE "Object Size Perception Experiment"

void keyboard(unsigned char key, int x, int y);
void display();
void myInit();
void specialKeys(int key, int x, int y);
void idle();

const int WIDTH = 1000;//400;
const int HEIGHT = 600;//400;
int width = WIDTH, height = HEIGHT;

cExp_Size_Perception m_expObjSize;

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_ALPHA);
	glutInitWindowSize(width, height);
	glutCreateWindow(_TITLE);
	glutDisplayFunc(display);
	glutReshapeFunc(DISP_TOOLS::reshapeSubFunc);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(specialKeys);
	glutIdleFunc(idle);
	//glutMouseFunc(DISP_TOOLS::mouseDownSubFunc);//onMouseDown);
	//glutMotionFunc(DISP_TOOLS::mouseMoveSubFunc);//onMouseMove);
	myInit();
	glutMainLoop();
	
	atexit(PHANTOM_TOOLS::exitHandler);
	
	return 0;
}

void keyboard(unsigned char key, int x, int y)
{
	char pTxt[256];
	int key_ret;

	key_ret = m_expObjSize.handleKeyboard(key, pTxt);
	if (key_ret == 100) {
		exit(0);
	}
	else if (key_ret == 102) {	// Error message box
		MessageBox(NULL, pTxt, "Error", MB_OK | MB_ICONERROR);
	}

}

void display()
{
	m_expObjSize.sub_display();
	glFlush();
	glutSwapBuffers();
}
void myInit()
{
	m_expObjSize.init();
}

void specialKeys(int key, int x, int y)
{
	m_expObjSize.handleSpecialKeys(key, x, y);
}

time_t start;
void idle()
{
	Sleep(10);
	glutPostRedisplay();
	if (m_expObjSize.m_exp_phase == EXP_NULL)
	{
		if (m_expObjSize.animation_cnt < 3)
		{
			m_expObjSize.square_pos += 4;
			/*
			1000 pixel => 270mm
			square_pos += 4 : 2.744sec
			=> 98.396 mm/sec
			약 100mm/sec
			*/
			if (m_expObjSize.square_pos == 1000)
			{
				m_expObjSize.animation_end = clock();
				m_expObjSize.tmp = false;
				m_expObjSize.square_pos = 0;
				m_expObjSize.moveToNextPhase();
				printf("소요시간: %lf\n", (double)(m_expObjSize.animation_end - m_expObjSize.animation_start) / CLOCKS_PER_SEC);
			}
		}
		else if(m_expObjSize.animation_cnt == 3 || m_expObjSize.animation_cnt == 4)
		{	
			m_expObjSize.square_pos -= 4;
			if (m_expObjSize.square_pos == 0)
			{
				m_expObjSize.tmp = false;
				m_expObjSize.square_pos = 1000;
				m_expObjSize.moveToNextPhase();
			}
		}
		else if (m_expObjSize.animation_cnt == 5)m_expObjSize.animation_cnt = 1;
		
	}

	/*glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPushMatrix();
	Sleep(1);
	glViewport(x, 0, 100, 100);
	glColor3f(1.0, 1.0, 0.0); //
	glBegin(GL_POLYGON); // 입력요소 기본정의

	glVertex3f(-0.5, -0.5, 0.0);
	glVertex3f(+0.5, -0.5, 0.0);
	glVertex3f(+0.5, 0.5, 0.0);
	glVertex3f(-0.5, 0.5, 0.0);
	glEnd();

	glPopMatrix();

	glutSwapBuffers();

	x++;
	*/
}