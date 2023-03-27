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

Serial* SP = new Serial("\\\\.\\COM3");

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
	///phantom position
	printf("%f", fabs(PHANTOM_TOOLS::get_phantom_x_position()));
	printf("\n");
	glutPostRedisplay();
	if (m_expObjSize.m_exp_phase == EXP_PHASE_LRA)	//right hand
	{
		
			if (m_expObjSize.square_pos == 200)
			{
				double now = clock();
				while (clock() - now < 3000);
				//Sleep(3000);
				m_expObjSize.square_pos += 4;
			}
			m_expObjSize.square_pos += 4;
			/*
			tablet size : 34cm X 19cm
			1000 pixel => 270mm
			square_pos += 4 : 2.744sec
			=> 98.396 mm/sec
			약 100mm/sec
			*/
			//0126 수정 - LRA작동 직사각형 위치에 따라 작동하도록 변경

			if (m_expObjSize.square_pos >= 500 && m_expObjSize.square_pos <= 510)
			{
				if(m_expObjSize.m_curr_trial_no == 1)SP->WriteData("1", 255);
				else if (m_expObjSize.m_curr_trial_no == 2)SP->WriteData("2", 255);
				else if (m_expObjSize.m_curr_trial_no == 3)SP->WriteData("3", 255);
				else if (m_expObjSize.m_curr_trial_no == 4)SP->WriteData("4", 255);
				else if (m_expObjSize.m_curr_trial_no == 5)SP->WriteData("5", 255);
				else if (m_expObjSize.m_curr_trial_no == 6)SP->WriteData("6", 255);
				else if (m_expObjSize.m_curr_trial_no == 7)SP->WriteData("7", 255);
				else if (m_expObjSize.m_curr_trial_no == 8)SP->WriteData("8", 255);
				else if (m_expObjSize.m_curr_trial_no == 9)SP->WriteData("9", 255);
			}
	
			if (m_expObjSize.square_pos == 800)
			{
				m_expObjSize.animation_end = clock();
				m_expObjSize.tmp = false;
				if (m_expObjSize.lra_first) m_expObjSize.tmp = true;
				m_expObjSize.square_pos = 300;
				m_expObjSize.moveToNextPhase();
				//printf("소요시간: %lf\n", (double)(m_expObjSize.animation_end - m_expObjSize.animation_start) / CLOCKS_PER_SEC);
			}
	}
	else if (m_expObjSize.m_exp_phase == EXP_PHASE_TOUCH)	//left hand
	{
		if (m_expObjSize.draw_phantom_position_square)
		{

		}
		if (m_expObjSize.square_pos == 200)
		{
			m_expObjSize.square_pos += 4;
		}
		m_expObjSize.square_pos += 4;
		if (m_expObjSize.square_pos == 800)
		{
			m_expObjSize.square_pos = 200;
			//printf("소요시간: %lf\n", (double)(m_expObjSize.animation_end - m_expObjSize.animation_start) / CLOCKS_PER_SEC);
		}

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