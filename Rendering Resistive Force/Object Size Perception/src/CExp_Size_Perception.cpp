#include "CExp_Size_Perception.h"
#include "CMCI_sound.h"
//#include "C826_DIO_ADC.h"
//#include <826api.h>
#include "phantom_helper.h"
#include "display_helper.h"


unsigned __stdcall posCtrlThread(void* arg) { return 0; };


unsigned __stdcall audioThread(void* arg);
unsigned __stdcall hapticRenderingThread(void* arg);
HDCallbackCode HDCALLBACK PhantomCallback(void* pUserData);


#define _DBG_REC
const char m_instruction_msg[][4][128] = {
	{{"Insert your insert your index finger and middle finger inside the omni."}, {"Then hit 'Enter' to initialize the experimental setup."}},	//INIT
	{{""}},	//EXP_NULL
	{{"Which force was greater?"},{"left hand : Press '['"},{"right hand : Press ']'"}},	//GET_ANSWER
	{{"Intializing the contact position..."}}, // DEVICE_INIT 
	{{"subject ID:"}, {"Type subject ID and hit 'Enter' to begin training session."}, {""}}, // INFO_INPUT
	{{"Drag two fingers of your right hand along the moving red rectangle on the touch screen"}, {"Press 'n' to go to the next page"}},	// TRAINING_PHASE1
	{{"Training session2"}, {"If the force you feel in your right hand is greater than the force you feel in your left hand, press ']' "}, {"if(right hand < left hand) press '['"}, {"Press 'e' to go to the next page "}},		// TRAINING PHASE2
	{{"Follow Square"}, {"To show moving sqaure, hit 'enter'"}},	//TEST_PRE_EXP
	{{"Trial No. :"}, {"Follow the red rectangle with your finger."}, {"Compare the force felt in the left hand with the force felt in the right hand"}, {"Get Ready to drag. It starts after 3 seconds after pressing the 'Enter'"}},	// EXP_PHASE1
	{{"Trial No. :"}, {"Follow the red rectangle with your finger."}, {"Compare the force felt in the left hand with the force felt in the right hand"}, {"Get Ready to drag. It starts after 3 seconds after pressing the 'Enter'"}},		// EXP_PHASE2
	{{"Trial No. :"}, {"Hit 'F8' first"}, {"Hit 'Enter' to move to feel the next stimulus."}},		// EXP_PHASE3
	{{"Trial No. :"}, {"Follow the red rectangle with your finger."}, {"Compare the force felt in the left hand with the force felt in the right hand"}, {"Get Ready to drag. It starts after 3 seconds after pressing the 'Enter'"}},	// EXP_PHASE4
	{{"Trial No. :"}, {"Follow the red rectangle with your finger."}, {"Compare the force felt in the left hand with the force felt in the right hand"}, {"Get Ready to drag. It starts after 3 seconds after pressing the 'Enter'"}},	// EXP_PHASE5
	{{"Trial No. :"}, {"Hit 'Enter' to type your answer."}},		// EXP_PHASE6
	{{"Trial No. :"}, {"Which object was larger? Type your answer and hit 'Enter'"}, {"Your answer (1: 1st object; 2: 2nd object):"}},	//EXP_ANSWER
	{{"Trial No. :"}, {"Your answer for this trial is that"}, {"Hit 'Enter' to move to the next trial."}},//EXP_ANSWER_CORRECTNESS
	{{""}, {""}, {""}}, // DATA_ANALYSIS.
	{{""},},//WRITE_RESULT,
	{{"Experiment Complete. Thank you for your participation."}}//EXP_DONE,
};

HANDLE cExp_Size_Perception::m_hAudioEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
HANDLE cExp_Size_Perception::m_hPosCtrlEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

const double m_Lin_MaxPos[2] = { 0.00529, 0.00548 };//0.00673;
const double m_Lin_MinPos = 0.0;

static int m_dbg_val[4] = { 0, 0, 0, 0 };
static double m_dbg_dVal[4] = { 0.0, 0.0, 0.0, 0.0 };
static double m_dbg_vec[2][3] = { {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0} };



cExp_Size_Perception::cExp_Size_Perception() : m_num_devices(0)
{
	uint i;
	for (i = 0; i < 2; i++) {
		m_c_displace[i] = 0.0;
		m_c_prev_pos[i] = 0.0;
		m_c_contact_pos[i] = 0.0;
		m_prev_contact[i] = false;
		m_prev_collision_depth[i] = 0.0;
		m_cnt_collision_depth[i] = 0;
		m_sum_collision_depth[i] = 0.0;
		m_curr_max_coll_depth[i] = 0.0;
		m_c_Ierror[i] = 0.0;
		m_c_Perror_old[i] = 0.0;
		m_c_Fprev[i] = 0.0;
	}
	m_exp_phase = EXP_PHASE::INIT;
	m_view_height = -0.04;
	m_textBuf_len = 0;
	m_enable_force = false;
	m_disp_mode = false;
	//	m_thickness = 0.045;
	m_curr_K = 600;
	m_training_feedback = 0;
	m_tip_mass[0] = 0.075;
	m_tip_mass[1] = 0.075;
	m_c_phase = CUTAN_PHASE::C_INIT;
#ifdef _SS_TEST
	m_pSerial_ss = new Serial2(1, CBR_9600);
	m_enable_ss = false;
#endif
#ifdef _VIB_TEST
	m_pSerial_vib ;
	m_enable_vib = false;
#endif
	m_show_dbg_info = false;
	m_PWM_UpdateFreq = 100.0;
	m_ch_timer = 4;
	m_enableTimer = false;
	m_enable_cutaneous_render = false;
	m_c_Pgain[0] = 0.34;//0.42;//0.57;//0.42;
	m_c_Igain[0] = 0.33;//0.09;//0.4;
	m_c_Dgain[0] = 0.002;//0.015;//0.013;
	m_c_Pgain[1] = 0.45;//0.45;
	m_c_Igain[1] = 0.09;//0.09;//0.38;//0.4;
	m_c_Dgain[1] = 0.002;//0.015;//0.013;//0.013;
	m_show_border = false;
	m_show_fingertip = false;
	m_show_surface = false;
	m_cutaneous_stiffness = 1.0;
	m_training_feedback = 0;
	m_training_size = 0;
	m_curr_obj_size = 0.025;
	m_wall_pos[0] = -0.5 * m_curr_obj_size; m_wall_pos[1] = 0.5 * m_curr_obj_size;
	m_border_pos[0] = -0.0325; m_border_pos[1] = 0.0325;
	//	m_border_pos[0] = m_wall_pos[0]-0.0025; m_border_pos[1] = m_wall_pos[1] + 0.0025;	// border_pos should be fixed and should not provide a cue on the size of the object
	m_sphr_radius = 0.01;//0.03;//0.07;
	m_dp_sphr_radius = 0.01;//0.07;
	m_stimuli_ord = 0;
	m_exp_obj_size[0] = 0.04;
	tmp = false;
	square_pos = 0;
	animation_cnt = 0;
}

cExp_Size_Perception::~cExp_Size_Perception()
{
}

int cExp_Size_Perception::handleKeyboard(unsigned char key, char* ret_string)	// pure virtual functions
{
	uint i;
	int ret = 0;
	if (key == 27) {
		m_exp_phase = EXP_PHASE::QUIT_EXP;
		SetEvent(m_hPosCtrlEvent);
		SetEvent(m_hAudioEvent);
		PHANTOM_TOOLS::exitHandler();
		Sleep(100);

		ret = 100;
	}
	else {
		if (m_exp_phase == EXP_PHASE::INIT) {
			if (key == 13) {
				// send skin stretch signals
#ifdef _SS_TEST
				sendArd_ss(true, true);
				Sleep(300);
				sendArd_ss(false, false);
#endif
#ifdef _VIB_TEST
				//sendArd_vib(200, 200);
				//Sleep(500);
				//sendArd_vib(0, 0);
#endif
				///
				if (m_c_phase == CUTAN_PHASE::PRE_CONTACT) {
					m_enable_cutaneous_render = false;//true;
					m_c_phase = CUTAN_PHASE::DEV1_CONTACT;
					//setOutPwm(0, 0.9);
					ret = moveToNextPhase();
				}
			}
			/*	else if(key == '0') {
					PHANTOM_TOOLS::setForce(0, glm::vec3(0, 0, 0));
					PHANTOM_TOOLS::setForce(1, glm::vec3(0, 0, 0));
				}
				else if(key == '1') {
					PHANTOM_TOOLS::setForce(0, glm::vec3(0, 1, 0));
					PHANTOM_TOOLS::setForce(1, glm::vec3(0, 1, 0));
				}
				else if(key == '2') {
					PHANTOM_TOOLS::setForce(0, glm::vec3(0, -1, 0));
					PHANTOM_TOOLS::setForce(1, glm::vec3(0, -1, 0));
				}	*/
				//else if(key == '0') setOutPwm(0, 0.0);
				//else if(key == '1') setOutPwm(0, -0.7);
				//else if(key == '2') setOutPwm(0, 0.7);
				//else if(key == '3') setOutPwm(1, 0.0);
				//else if(key == '4') setOutPwm(1, -0.7);
				//else if(key == '5') setOutPwm(1, 0.7);	
		}
		else if (m_exp_phase == EXP_PHASE::INFO_INPUT) {
			//if (key == '-') tmp = true;
			if (key >= '0' && key <= 'z') {// ASCII 48 ~ 122
				m_txtBuf[m_textBuf_len++] = key;
				m_txtBuf[m_textBuf_len] = NULL;
			}
			else if (key == 8) { // backspace
				if (m_textBuf_len > 0) {
					m_txtBuf[--m_textBuf_len] = NULL;
				}
			}
			else if (key == 13) {
				if (m_textBuf_len == 0) ret = -2;
				else {
					memcpy((void*)m_subjectID, (void*)m_txtBuf, sizeof(char) * m_textBuf_len);
					m_subjectID[m_textBuf_len] = NULL;
					if (strcmp(m_txtBuf, "0000") == 0) {
						printf("test subject %s\n", m_txtBuf);
						m_testSubject = true;
					}
					else {
						char directory_path[128];
						bool directory_check = true;
						sprintf(directory_path, "./z_%s", m_subjectID);
						if (!check_directory(directory_path)) directory_check = false;
						if (!directory_check) {
							MessageBox(NULL, "Failed to locate the logging folder", "Error", MB_OK | MB_ICONERROR);
							m_exp_phase = EXP_PHASE::QUIT_EXP;
							return 100;
						}
						else {
							sprintf(m_rec_filename, "%s/%s_%d_%d.txt", directory_path, m_subjectID, 0, (int)(100.0 * m_exp_obj_size[0]));	// subject_ID exp_type reference_size.txt

						}
					}
					m_textBuf_len = 0;
					ret = moveToNextPhase(ret_string);
				}
			}
			else {
				ret = 102;
				sprintf(ret_string, "Type a valid character (0~z)");
			}
		}
		/*training_phase1->training_phase2 안넘어가서 추가*/
		else if (m_exp_phase == EXP_PHASE::TRAINING_PHASE1) {
			if (key == 'n' || key == 'N') {
				moveToNextPhase(ret_string);
			}
		}
		else if (m_exp_phase == EXP_PHASE::TRAINING_PHASE2) {
			if (key == 'f' || key == 'F') {
				if (m_training_feedback == 0) {
					initTrainingPhase(1);
				}
				else if (m_training_feedback == 1) {
					initTrainingPhase(2);
				}
				else initTrainingPhase(0);
			}
			else if (key == 'e' || key == 'E') {
				ret = moveToNextPhase(ret_string);
			}
			else {
				if (key >= '1' && key <= '4') {
					m_training_size = key - '1';
					initTrainingPhase(m_training_feedback);
				}
			}
		}
		else if (m_exp_phase == EXP_PHASE::TEST_PRE_EXP) {
			if (key == 13) {
				m_show_border = false;
				m_show_fingertip = false;
				m_show_surface = false;
				m_enable_force = false;
				ret = moveToNextPhase(ret_string);
				if (!tmp) {
					square_pos = 0;
					tmp = true;
					animation_start = clock();
				}
				else if (tmp) {
					tmp = false;
					square_pos = 0;
					glViewport(0, 0, (GLsizei)1000, (GLsizei)600);//width, height 하드코딩
				}
			}
		}
		else if (m_exp_phase == EXP_PHASE::EXP_NULL)
		{
			
			if (key == 13) {
				if (!tmp) {
					square_pos = 0;
					tmp = true;
				}
				else if (tmp) {
					tmp = false;
					square_pos = 0;
					glViewport(0, 0, (GLsizei)1000, (GLsizei)600);//width, height 하드코딩
					moveToNextPhase(ret_string);
				}
			}
		}
		else if (m_exp_phase == EXP_PHASE::GET_ANSWER)
		{
			if (key == '[' || key == ']')
			{
				PHANTOM_TOOLS::adjust_force(key);
				if (key == '[') usr_input.push_back(1);
				else if (key == ']') usr_input.push_back(2);
				 moveToNextPhase();
			}

		}
		//임시 추가
		else if (m_exp_phase == EXP_PHASE::EXP_PHASE1 || m_exp_phase == EXP_PHASE::EXP_PHASE2) {
			if (key == 13)
			{
				Sleep(3000);
				moveToNextPhase(ret_string);
				if (!tmp) {
					animation_start = clock();
					square_pos = 0;
					tmp = true;
				}
				else if (tmp) {
					tmp = false;
					square_pos = 0;
					glViewport(0, 0, (GLsizei)1000, (GLsizei)600);//width, height 하드코딩
				}
			}
		}
		//
		else if (m_exp_phase == EXP_PHASE::EXP_PHASE3 || m_exp_phase == EXP_PHASE::EXP_PHASE6 ||
			m_exp_phase == EXP_PHASE::EXP_ANSWER_CORRECTNESS) {
			if (key == 13) {
				moveToNextPhase(ret_string);
			}
		}
		//임시 추가
		else if (m_exp_phase == EXP_PHASE::EXP_PHASE4 || m_exp_phase == EXP_PHASE::EXP_PHASE5) {
			if (key == '[' || key == ']')PHANTOM_TOOLS::adjust_force(key);
			else if (key == 13)
			{
				Sleep(3000);
				moveToNextPhase(ret_string);
				if (!tmp) {
					square_pos = 1000;
					tmp = true;
				}
				else if (tmp) {
					tmp = false;
					square_pos = 1000;
					glViewport(0, 0, (GLsizei)1000, (GLsizei)600);//width, height 하드코딩
				}
			}
		}
		//
		else if (m_exp_phase == EXP_PHASE::EXP_ANSWER) {
			if (key == 13) {
				m_curr_answer = m_txtBuf[0] - '1';
				if ((m_curr_answer != 0 && m_curr_answer != 1) || m_textBuf_len != 1) {
					ret = 102;
					sprintf(ret_string, "Type a valid answer (1 or 2)");
				}
				else {
					m_textBuf_len = 0;
					ret = moveToNextPhase(ret_string);
				}
			}
			else if (key == 8) {
				if (m_textBuf_len == 1) {
					m_textBuf_len = 0;
					m_txtBuf[0] = NULL;
				}
			}
			else if (key == '1' || key == '2') {
				if (m_textBuf_len == 0 || m_textBuf_len == 1) {
					m_textBuf_len = 1;
					m_txtBuf[0] = key; m_txtBuf[1] = NULL;
				}
			}
			else {
				ret = 102;
				sprintf(ret_string, "Type a valid anwer (1 or 2)");
			}
		}
	}

	return ret;
}

void cExp_Size_Perception::initTrainingPhase(uint feedback)
{
	m_show_fingertip = true;
	m_show_border = true;
	m_show_surface = false;
	m_training_feedback = feedback;
	enableCutaneous(false);
	m_enable_force = false;
	m_exp_phase = EXP_PHASE::TRAINING_PHASE1;
}

void cExp_Size_Perception::setAudioPhase(int audio_phase)
{
	m_audio_phase = audio_phase;
	SetEvent(m_hAudioEvent);
}
int cExp_Size_Perception::moveToNextPhase(char* ret_string)
{
	int ret = 0, i;
	if (m_exp_phase == EXP_PHASE::INIT) {
		//m_exp_phase = EXP_PHASE::DEVICE_INIT; 원래코드 / 페이지 안넘어가서 수정
		m_exp_phase = EXP_PHASE::INFO_INPUT;
	}
	else if (m_exp_phase == EXP_PHASE::DEVICE_INIT) {
		m_enable_force = false;
		m_exp_phase = EXP_PHASE::INFO_INPUT;
	}
	else if (m_exp_phase == EXP_PHASE::INFO_INPUT) {
		m_show_border = true;
		m_show_fingertip = true;
		m_show_surface = false;
		m_enable_force = false;
#ifdef _SS_TEST
		m_enable_ss = false;
#endif
#ifdef _VIB_TEST
		m_enable_vib = false;
#endif
		enablePositionCtrl(true);
		enableCutaneous(false);
		m_exp_phase = EXP_PHASE::TRAINING_PHASE1;
	}
	else if (m_exp_phase == EXP_PHASE::TRAINING_PHASE1) {
		m_show_border = false;
		m_show_surface = true;
		/*training 부분 일단 주석처리
		if (m_training_feedback == 0) {
			enableCutaneous(true);
		//	m_curr_K = m_exp_K[0];
			m_enable_force = true;
		}
		else if(m_training_feedback == 1) {
			enableCutaneous(false);
			m_enable_force = true;
		}
		else {
			enableCutaneous(true);
			m_enable_force = false;
		}
		if(m_training_size == 0) m_curr_obj_size = 0.025;
		else if(m_training_size == 1) m_curr_obj_size = 0.035;
		else if(m_training_size == 2) m_curr_obj_size = 0.045;
		else m_curr_obj_size = 0.055;
		m_wall_pos[0] = -0.5*m_curr_obj_size;
		m_wall_pos[1] = 0.5*m_curr_obj_size;
		*/
		//	printf("curr_object size = %.5f\n", m_curr_obj_size);
		m_exp_phase = EXP_PHASE::TRAINING_PHASE2;
	}
	else if (m_exp_phase == EXP_PHASE::TRAINING_PHASE2) {
		//m_show_fingertip = true;
		//m_show_border = true;
	//	m_show_surface = false;
		m_curr_trial_no = 1;
		m_stimuli_ord = rand() % 2;
		m_exp_obj_size[1] = m_init_stimulus;
		setAudioPhase(AUDIO_PHASE::PLAY);
		/// collision recording
		for (i = 0; i < 2; i++) {
			m_curr_max_coll_depth[i] = -1.0;
			m_first_max_coll_depth[i] = -1.0;
			m_cnt_collision_depth[i] = 0;
			m_sum_collision_depth[i] = 0.0;
		}
		m_expResult.clear();
		time(&m_expBeginTime);
		time(&m_trialBeginTime);
		enablePositionCtrl(true);
		recordResult(RECORD_TYPE::REC_INIT);
		m_exp_phase = EXP_PHASE::TEST_PRE_EXP;
		//m_exp_phase = EXP_PHASE::EXP_PHASE1;
		m_show_surface = false;
		enableCutaneous(false);
	}
	else if (m_exp_phase == EXP_PHASE::TEST_PRE_EXP) {
		//Sleep(100);
		//tmp = true;
		m_exp_phase = EXP_PHASE::EXP_NULL;
	}
	else if (m_exp_phase == EXP_PHASE::EXP_NULL)
	{
		glViewport(0, 0, (GLsizei)1000, (GLsizei)600);//width, height 하드코딩
		animation_cnt++;
		m_exp_phase = EXP_PHASE::GET_ANSWER;	
	}
	else if (m_exp_phase == EXP_PHASE::GET_ANSWER) {
		if (animation_cnt == 1) {
			m_exp_phase = EXP_PHASE::EXP_PHASE1;
		}
		else if (animation_cnt == 2) {
			m_exp_phase = EXP_PHASE::EXP_PHASE2;
		}
		else if (animation_cnt == 3) {
			m_exp_phase = EXP_PHASE::EXP_PHASE3;
		}
		else if (animation_cnt == 4) {
			m_exp_phase = EXP_PHASE::EXP_PHASE5;
		}
		else if (animation_cnt == 5) {
			m_exp_phase = EXP_PHASE::EXP_PHASE6;
		}
	}
	else if (m_exp_phase == EXP_PHASE::EXP_PHASE1) {
		m_show_border = false;
		//	m_show_surface = true;
		enableCutaneous(true);
		m_enable_force = true;
		m_enable_force = true;
		if (m_stimuli_ord == 0) {	/// 1. cutaneous+force 2. cutaneous+force+skin_stretch
			m_curr_obj_size = m_exp_obj_size[1];
#ifdef _SS_TEST
			m_enable_ss = false;
#endif
#ifdef _VIB_TEST
			m_enable_vib = false;
#endif
		}
		else {				///1. cutaneous+force+skin_stretch 2. cutaneous+force
			m_curr_obj_size = m_exp_obj_size[0];
#ifdef _SS_TEST
			m_enable_ss = true;
#endif
#ifdef _VIB_TEST
			m_enable_vib = true;
#endif
		}
		m_wall_pos[0] = -0.5 * m_curr_obj_size;
		m_wall_pos[1] = 0.5 * m_curr_obj_size;
		m_exp_phase = EXP_PHASE::EXP_NULL;
	}
	else if (m_exp_phase == EXP_PHASE::EXP_PHASE2) {
		/// checks if the contact point is close to the plane
		m_show_fingertip = false;
		m_exp_phase = EXP_PHASE::EXP_NULL;
	}
	else if (m_exp_phase == EXP_PHASE::EXP_PHASE3) {
		//m_show_fingertip = true;
		//m_show_border = true;
		//m_enable_force = false;
		//enableCutaneous(false);
#ifdef _SS_TEST
		m_enable_ss = false;
#endif
#ifdef _VIB_TEST
		m_enable_vib = false;
#endif
		for (i = 0; i < 2; i++) {
			m_mean_coll_depth[i] = m_sum_collision_depth[i] / (double)m_cnt_collision_depth[i];
			m_first_max_coll_depth[i] = m_curr_max_coll_depth[i];
			m_curr_max_coll_depth[i] = -1.0;
			m_cnt_collision_depth[i] = 0;
			m_sum_collision_depth[i] = 0.0;
		}
		m_exp_phase = EXP_PHASE::EXP_PHASE4;
	}
	else if (m_exp_phase == EXP_PHASE::EXP_PHASE4) {
		m_show_border = false;
		//	m_show_surface = true;
		enableCutaneous(true);
		m_enable_force = true;
		if (m_stimuli_ord == 0) {			// 1. cutaneous+force 2. cutaneous+force+skin_stretch
			//enablePositionCtrl(true);
			m_curr_obj_size = m_exp_obj_size[0];
#ifdef _SS_TEST
			m_enable_ss = true;
#endif
#ifdef _VIB_TEST
			m_enable_vib = true;
#endif
		}
		else {		// m_stimuli_ord == 1	1. cutaneous+force+skin_stretch 2. cutaneous+force
			//enablePositionCtrl(false);
			//enableCutaneous(false);
			m_curr_obj_size = m_exp_obj_size[1];
			//m_enable_force = true;
#ifdef _SS_TEST
			m_enable_ss = false;
#endif
#ifdef _VIB_TEST
			m_enable_vib = false;
#endif
		}
		m_wall_pos[0] = -0.5 * m_curr_obj_size;
		m_wall_pos[1] = 0.5 * m_curr_obj_size;
		m_exp_phase = EXP_PHASE::EXP_NULL;
	}
	else if (m_exp_phase == EXP_PHASE::EXP_PHASE5) {
		m_show_surface = false;
		m_show_fingertip = false;
		m_exp_phase = EXP_PHASE::EXP_NULL;
	}
	else if (m_exp_phase == EXP_PHASE::EXP_PHASE6) {
		//	enablePositionCtrl(false);
		enableCutaneous(false);
		m_enable_force = false;
#ifdef _SS_TEST
		m_enable_ss = false;
#endif
#ifdef _VIB_TEST
		m_enable_vib = false;
#endif
		//sendArd_vib(0, 0);
		setAudioPhase(AUDIO_PHASE::PAUSE);
		m_exp_phase = EXP_PHASE::EXP_ANSWER;
	}
	else if (m_exp_phase == EXP_PHASE::EXP_ANSWER) {
		//	enablePositionCtrl(false);
		//		m_enable_force = false;
		m_exp_phase = EXP_PHASE::EXP_ANSWER_CORRECTNESS;
	}
	else if (m_exp_phase == EXP_PHASE::EXP_ANSWER_CORRECTNESS) {
		double next_stimulus;
		int input_answer;
		if (m_stimuli_ord == 0) {
			if (m_curr_answer == 0) input_answer = 0;		// force feedback felt larger
			else input_answer = 1;							// cutaneous+force felt larger
		}
		else {
			if (m_curr_answer == 1) input_answer = 0;		// felt larger
			else input_answer = 1;							// cutaneous+force feedback felt larger
		}
		/// recording trial result
	//	if(!m_testSubject) {
		exp_result curr_result;
		curr_result.trial_begin_time = m_trialBeginTime;
		time(&curr_result.trial_end_time);
		curr_result.trial_no = m_curr_trial_no;

		curr_result.trial_stimulus = m_exp_obj_size[1];//m_exp_K[1];
		curr_result.trial_user_ans = input_answer;	// 0: force object felt larger. 1: (cutaneous+force) object was felt larger
		if (m_stimuli_ord == 0) {
			for (i = 0; i < 2; i++) {
				curr_result.mean_coll_depth[i][0] = m_sum_collision_depth[i] / (double)m_cnt_collision_depth[i];
				curr_result.mean_coll_depth[i][1] = m_mean_coll_depth[i];
				curr_result.max_coll_depth[i][0] = m_curr_max_coll_depth[i];
				curr_result.max_coll_depth[i][1] = m_first_max_coll_depth[i];
			}
		}
		else {
			for (i = 0; i < 2; i++) {
				curr_result.mean_coll_depth[i][1] = m_sum_collision_depth[i] / (double)m_cnt_collision_depth[i];
				curr_result.mean_coll_depth[i][0] = m_mean_coll_depth[i];
				curr_result.max_coll_depth[i][1] = m_curr_max_coll_depth[i];
				curr_result.max_coll_depth[i][0] = m_first_max_coll_depth[i];
			}
		}

		for (int i = 0; i < usr_input.size(); i++) curr_result.omni_force.push_back(usr_input[i]);
		usr_input.clear();
		m_expResult.push_back(curr_result);
		//	}
		recordResult(RECORD_TYPE::REC_TRIAL);
		ret = calcStimulus(m_exp_obj_size[1], input_answer, &next_stimulus);
		if (ret == 0) {
			m_exp_obj_size[1] = next_stimulus;//m_exp_K[1] = next_stimulus;
			m_show_fingertip = true;
			m_show_border = true;
			m_show_surface = false;
			m_stimuli_ord = rand() % 2;
			m_curr_trial_no++;
			time(&m_trialBeginTime);
			for (i = 0; i < 2; i++) {
				m_curr_max_coll_depth[i] = -1.0;
				m_first_max_coll_depth[i] = -1.0;
				m_cnt_collision_depth[i] = 0;
				m_sum_collision_depth[i] = 0.0;
			}
			setAudioPhase(AUDIO_PHASE::PLAY);
			m_exp_phase = EXP_PHASE::EXP_PHASE1;
		}
		else {	// experiment termination
			m_exp_phase = EXP_PHASE::DATA_ANALYSIS;
			dataAnalysis();
			setAudioPhase(AUDIO_PHASE::COMPLETE);
			recordResult(RECORD_TYPE::REC_END);
			m_exp_phase = EXP_PHASE::EXP_DONE;
		}
	}
	return ret;
}

void cExp_Size_Perception::dataAnalysis()
{
	m_PSE_obj_size = calculatePSE();
	printf("Obj_Size_PSE = %f\n", m_PSE_obj_size);
}

void cExp_Size_Perception::recordResult(int type)
{
	FILE* pFile;
	time_t curr_time, tot_time;
	tm time_tm, time_tm2;
	errno_t err;
	if (type == RECORD_TYPE::REC_INIT) {
		err = _localtime64_s(&time_tm, &m_expBeginTime);//	err = _localtime64_s(&time_tm, &curr_time);
		printf("Subject: %s\n", m_subjectID);
		//	printf("virtual wall width = %.1f cm", m_thickness*100.0);
		//	printf("cutaneous stiffness = %.1f\n", m_cutaneous_stiffness, m_exp_K[0]);
		printf("Experiment began at %02d:%02d:%02d on %04d/%02d/%02d\n", time_tm.tm_hour, time_tm.tm_min, time_tm.tm_sec,
			time_tm.tm_year + 1900, time_tm.tm_mon + 1, time_tm.tm_mday);
		printf("-------------------------------------------------------------------------\n");
		//printf("Trial no.\tObj size(mm)\tanswer(F object larger:0/F+C object larger:1)]\ttrial time (mm:ss)\tLeft)mean_collision_depth(F+C) (mm)\tmean_collision_depth(F) (mm)\tmax_collision_depth(F+C) (mm)\tmax_collision_depth(F)\tRight)mean_collision_depth(F+C) (mm)\tmean_collision_depth(F) (mm)\tmax_collision_depth(F+C) (mm)\tmax_collision_depth(F)\n");
		printf("Trial no.\tOmni force(N)\ttrial time (mm:ss)\tphase1\tphase2\tphase3\tphase4\n");
		printf("-------------------------------------------------------------------------\n");
		pFile = fopen(m_rec_filename, "a");
		if (pFile != NULL && !m_testSubject) {
			fprintf(pFile, "Subject: %s\n", m_subjectID);
			//	fprintf(pFile, "virtual wall width = %.1f cm", m_thickness*100.0);
			//	fprintf(pFile, "cutaneous stiffness = %.1f\n", m_cutaneous_stiffness, m_exp_K[0]);
			fprintf(pFile, "Experiment began at %02d:%02d:%02d on %04d/%02d/%02d\n", time_tm.tm_hour, time_tm.tm_min, time_tm.tm_sec,
				time_tm.tm_year + 1900, time_tm.tm_mon + 1, time_tm.tm_mday);
			fprintf(pFile, "-------------------------------------------------------------------------\n");
			//fprintf(pFile, "Trial no.\tObj size(mm)\tanswer(F object larger:0/F+C object larger:1)]\ttrial time (mm:ss)\tLeft)mean_collision_depth(F+C) (mm)\tmean_collision_depth(F) (mm)\tmax_collision_depth(F+C) (mm)\tmax_collision_depth(F)\tRight)mean_collision_depth(F+C) (mm)\tmean_collision_depth(F) (mm)\tmax_collision_depth(F+C) (mm)\tmax_collision_depth(F)\n");
			//	"Trial no.\tstiffness(N/mm)\tanswer(kinesthetic surface stiffer:0/cutaneous surface stiffer1)]\ttrial time (mm:ss)\tLeft)mean_collision_depth(F+C) (mm)\tmean_collision_depth(F) (mm)\tmax_collision_depth(F+C) (mm)\tmax_collision_depth(F)\tRight)mean_collision_depth(F+C) (mm)\tmean_collision_depth(F) (mm)\tmax_collision_depth(F+C) (mm)\tmax_collision_depth(F)\n");
			fprintf(pFile, "Trial no.\tOmni force(N)\ttrial time (mm:ss)\tphase1\tphase2\tphase3\tphase4\n");
			fprintf(pFile, "-------------------------------------------------------------------------\n");
			fclose(pFile);
		}
	}
	else if (type == RECORD_TYPE::REC_TRIAL) {
		exp_result last_result = m_expResult.back();
		tot_time = difftime(last_result.trial_end_time, last_result.trial_begin_time);
		err = _localtime64_s(&time_tm, &tot_time);
		printf("%d\t%.1f\t%d\t%02d:%02d\t%d\t%d\t%d\t%d\n", last_result.trial_no,
			last_result.trial_stimulus * 1000.0, last_result.trial_user_ans, time_tm.tm_min, time_tm.tm_sec,
			/*last_result.mean_coll_depth[0][0] * 1000.0, last_result.mean_coll_depth[0][1] * 1000.0,
			last_result.max_coll_depth[0][0] * 1000.0, last_result.mean_coll_depth[0][1] * 1000.0,
			last_result.mean_coll_depth[1][0] * 1000.0, last_result.mean_coll_depth[1][1] * 1000.0,
			last_result.max_coll_depth[1][0] * 1000.0, last_result.mean_coll_depth[1][1] * 1000.0);*/
			last_result.omni_force[0],
			last_result.omni_force[1],
			last_result.omni_force[2],
			last_result.omni_force[3]);
		pFile = fopen(m_rec_filename, "a");
		if (pFile != NULL && !m_testSubject) {
			fprintf(pFile, "%d\t%.1f\t%d\t%02d:%02d\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n", last_result.trial_no,
				last_result.trial_stimulus * 1000.0, last_result.trial_user_ans, time_tm.tm_min, time_tm.tm_sec,
				last_result.mean_coll_depth[0][0] * 1000.0, last_result.mean_coll_depth[0][1] * 1000.0,
				last_result.max_coll_depth[0][0] * 1000.0, last_result.mean_coll_depth[0][1] * 1000.0,
				last_result.mean_coll_depth[1][0] * 1000.0, last_result.mean_coll_depth[1][1] * 1000.0,
				last_result.max_coll_depth[1][0] * 1000.0, last_result.mean_coll_depth[1][1] * 1000.0);
			fclose(pFile);
		}
	}
	else {	// RECORD_TYPE::REC_END
		time(&curr_time);
		err = _localtime64_s(&time_tm, &curr_time);
		tot_time = difftime(curr_time, m_expBeginTime);
		err = _localtime64_s(&time_tm2, &tot_time);
		/////
		printf("===================================\n");
		printf("Experiment ended at %02d:%02d:%02d on %04d/%02d/%02d\n", time_tm.tm_hour,
			time_tm.tm_min, time_tm.tm_sec, time_tm.tm_year + 1900, time_tm.tm_mon + 1, time_tm.tm_mday);
		printf("Total experiment time: %02d:%02d\n", time_tm2.tm_min, time_tm2.tm_sec);
		printf("===================================\n");
		printf("object size PSE: %.3f mm\n", m_PSE_obj_size * 1000.0); //	printf("stiffness PSE: %.2f N/mm\n", m_PSE_K);
		printf("===================================\n");
		pFile = fopen(m_rec_filename, "a");
		if (pFile != NULL && !m_testSubject) {
			fprintf(pFile, "===================================\n");
			fprintf(pFile, "Experiment ended at %02d:%02d:%02d on %04d/%02d/%02d\n", time_tm.tm_hour,
				time_tm.tm_min, time_tm.tm_sec, time_tm.tm_year + 1900, time_tm.tm_mon + 1, time_tm.tm_mday);
			fprintf(pFile, "Total experiment time: %02d:%02d\n", time_tm2.tm_min, time_tm2.tm_sec);
			fprintf(pFile, "===================================\n");
			fprintf(pFile, "object size PSE: %.3f mm\n", m_PSE_obj_size * 1000);//	fprintf(pFile, "stiffness PSE: %.2f N/mm\n", m_PSE_K);
			fprintf(pFile, "===================================\n");
			fclose(pFile);
		}
	}
}

int cExp_Size_Perception::getCurrInstructionText(char pDestTxt[3][128])
{
	int ret = 0, len[4], i;
	for (i = 0; i < 4; i++) len[i] = strlen(m_instruction_msg[m_exp_phase][i]);
	if (len[0] == 0 && len[1] == 0) ret = -1;
	else {
		/// msg 1
		if (m_exp_phase == EXP_PHASE::INFO_INPUT) {
			sprintf_s(pDestTxt[0], "%s %s", m_instruction_msg[m_exp_phase][0], m_txtBuf);
		}
		else if (m_exp_phase == EXP_PHASE::EXP_NULL) {
			sprintf_s(pDestTxt[0], "%s", m_instruction_msg[m_exp_phase][0], m_txtBuf);
		}
		else if (m_exp_phase == EXP_PHASE::GET_ANSWER) {
			sprintf_s(pDestTxt[0], "%s", m_instruction_msg[m_exp_phase][0], m_txtBuf);
		}
		else if (m_exp_phase == EXP_PHASE::TEST_PRE_EXP) {
			sprintf_s(pDestTxt[0], "%s %s", m_instruction_msg[m_exp_phase][0], m_txtBuf);
		}
		else if (m_exp_phase >= EXP_PHASE::EXP_PHASE1 && m_exp_phase <= EXP_PHASE::EXP_ANSWER_CORRECTNESS) {
			sprintf_s(pDestTxt[0], "%s %d", m_instruction_msg[m_exp_phase][0], m_curr_trial_no);
		}
		else if (m_exp_phase == EXP_PHASE::TRAINING_PHASE2) {
			if (m_training_feedback == 0)
				sprintf_s(pDestTxt[0], "%s Object Perception Experiment", m_instruction_msg[m_exp_phase][0]);
			else if (m_training_feedback == 1)
				sprintf_s(pDestTxt[0], "%s Force", m_instruction_msg[m_exp_phase][0]);
			else
				sprintf_s(pDestTxt[0], "%s Cutaneous", m_instruction_msg[m_exp_phase][0]);
		}
		else
		{
			strcpy_s(pDestTxt[0], m_instruction_msg[m_exp_phase][0]);
			pDestTxt[0][len[0]] = NULL;
		}
		/// msg 2
		if (m_exp_phase == EXP_PHASE::EXP_ANSWER_CORRECTNESS) {
			int curr_answer = m_txtBuf[0] - '0';
			sprintf(pDestTxt[1], "%s %s", m_instruction_msg[m_exp_phase][1], (m_curr_answer == 0) ? "'the 1st surface was larger.'" : "'the 2nd surface was larger.'");
		}
		else {
			strcpy_s(pDestTxt[1], m_instruction_msg[m_exp_phase][1]);
			pDestTxt[1][len[1]] = NULL;
		}
		/// msg 3
		if (m_exp_phase == EXP_PHASE::EXP_ANSWER) {
			char pTxt[4];
			sprintf(pDestTxt[2], "%s", m_instruction_msg[m_exp_phase][2]);
			if (m_textBuf_len == 1) {
				sprintf(pTxt, "%d", (int)(m_txtBuf[0] - '0'));
				strcat(pDestTxt[2], pTxt);
			}
		}
		//else if(m_exp_phase == EXP_PHASE::TRAINING_PHASE2) {
		//	if(m_training_feedback == 0) {
		//		strcpy_s(pDestTxt[2], m_instruction_msg[m_exp_phase][3]);
		//		pDestTxt[2][len[3]] = NULL; 
		//	}
		//	else {
		//		strcpy_s(pDestTxt[2], m_instruction_msg[m_exp_phase][2]);
		//		pDestTxt[2][len[2]] = NULL;
		//	}
		//}
		else {
			strcpy_s(pDestTxt[2], m_instruction_msg[m_exp_phase][2]);
			pDestTxt[2][len[2]] = NULL;
		}
		/// msg 4
		//if(m_exp_phase == EXP_PHASE::TRAINING_PHASE2 && m_training_feedback == 0) {
		//	pDestTxt[3][0] = NULL;
		//}
		//else {
		strcpy_s(pDestTxt[3], m_instruction_msg[m_exp_phase][3]);
		pDestTxt[3][len[3]] = NULL;
		//	}
	}
	return ret;
}

void cExp_Size_Perception::sub_display()
{
	char pTxt[4][128];
	int i;
	DISP_TOOLS::setupGraphicsState();
	if (0 != getCurrInstructionText(pTxt)) {
		for (i = 0; i < 4; i++) pTxt[i][0] = NULL;
	}
	/*if (m_disp_mode) {
		PHANTOM_TOOLS::renderTools();
		glColor3f(0, 1, 0);
		DISP_TOOLS::DrawSphere(m_prev_proxy[0], 0.003);
		DISP_TOOLS::DrawSphere(m_prev_proxy[1], 0.003);
		DISP_TOOLS::DrawSphere(m_sphere_center[0], m_sphr_radius);//+glm::vec3(m_sphr_radius-0.01, 0, 0), 0.01);
		DISP_TOOLS::DrawSphere(m_sphere_center[1], m_sphr_radius);//+glm::vec3(-m_sphr_radius+0.01, 0, 0), 0.01);
		glColor3f(0, 0, 1);
	}
	*/
	glColor3f(0, 0, 0);
	DISP_TOOLS::Draw_Text(pTxt[0], -0.15f, m_view_height + 0.08f, 0.f);	//	DISP_TOOLS::Draw_Text(pTxt[0], -7.2f, 3.8f, 0.f);
	DISP_TOOLS::Draw_Text(pTxt[1], -0.15f, m_view_height + 0.07f, 0.f);	// 	DISP_TOOLS::Draw_Text(pTxt[1], -7.2f, 3.2f, 0.f);
	DISP_TOOLS::Draw_Text(pTxt[2], -0.15f, m_view_height + 0.06f, 0.f);	// 	DISP_TOOLS::Draw_Text(pTxt[2], -7.2f, 2.6f, 0.f);
	DISP_TOOLS::Draw_Text(pTxt[3], -0.15F, m_view_height + 0.05f, 0.f);
	////////////
		/*
	#ifdef _DBG_REC
		if(m_show_dbg_info) {
			sprintf(pTxt[0], " debug (int): %d, %d, %d, %d", m_dbg_val[0], m_dbg_val[1], m_dbg_val[2], m_dbg_val[3]);
			DISP_TOOLS::Draw_Text(pTxt[0], -0.15f, m_view_height+0.03f, 0.f);
			sprintf(pTxt[1], " debug (double): %.3f, %.3f, %.3f, %.3f", m_dbg_dVal[0], m_dbg_dVal[1], m_dbg_dVal[2], m_dbg_dVal[3]);
			DISP_TOOLS::Draw_Text(pTxt[1], -0.15f, m_view_height+0.02f, 0.f);
			sprintf(pTxt[0], "dvec(0): (%.3f, %.3f, %.3f) dvec(1): (%.3f, %.3f, %.3f)", m_dbg_vec[0][0], m_dbg_vec[0][1], m_dbg_vec[0][2],
				m_dbg_vec[1][0], m_dbg_vec[1][1], m_dbg_vec[1][2]);
			DISP_TOOLS::Draw_Text(pTxt[0], -0.15f, m_view_height+0.01f, 0.f);
		}
	#endif
		glColor3f(0, 1, 0);
		/// draw fingertip
		if(m_show_fingertip) {
		//	DISP_TOOLS::DrawSphere(m_sphere_center[0]+glm::vec3(m_dp_sphr_radius-0.01, 0, 0), 0.01);
		//	DISP_TOOLS::DrawSphere(m_sphere_center[1]+glm::vec3(-m_dp_sphr_radius+0.01, 0, 0), 0.01);
			DISP_TOOLS::DrawSphere(m_dp_sphere_center[0], m_dp_sphr_radius);	// 03/10/2019
			DISP_TOOLS::DrawSphere(m_dp_sphere_center[1], m_dp_sphr_radius);	// 03/10/2019
			//DISP_TOOLS::DrawSphere(m_prev_contact_pt+glm::vec3(0, 0.01, 0), 0.01);
	//		glColor3f(1, 0, 0);
	//		DISP_TOOLS::DrawSphere(m_prev_contact_pt[0], m_dp_sphr_radius*0.1);	// 03/10/2019
	//		DISP_TOOLS::DrawSphere(m_prev_contact_pt[1], m_dp_sphr_radius*0.1);	// 03/10/2019m_prev_contact_pt[0]
		}
		glBegin(GL_LINES);
		if(m_show_border) {
			glColor3f(1, 0, 0);
		//	glVertex3f(-9.0, m_border_height, 0);
		//	glVertex3f(9.0, m_border_height, 0);
			glVertex3f(m_border_pos[0], 9.0, 0); glVertex3f(m_border_pos[0], -9.0, 0);
			glVertex3f(m_border_pos[1], 9.0, 0); glVertex3f(m_border_pos[1], -9.0, 0);
		}
		if(m_show_surface) {
			glColor3f(0, 0, 0);
			glVertex3f(m_wall_pos[0], 9.0, 0); glVertex3f(m_wall_pos[0], -9.0, 0);
			glVertex3f(m_wall_pos[1], 9.0, 0); glVertex3f(m_wall_pos[1], -9.0, 0);
		//	glVertex3f(-9.0, m_plane_height, 0);
		//	glVertex3f(9.0, m_plane_height, 0);
		}
		glEnd();*/
	if (tmp) {
		glColor3f(1, 0, 0);
		DISP_TOOLS::DrawSquare(square_pos);
		/*printf("animation 나와라");
		//glutDisplayFunc(DISP_TOOLS::RenderScene);
		glutReshapeFunc(DISP_TOOLS::ChangeSize);
		printf("renderscene입력후");
		DISP_TOOLS::TimerFunction(300);
		printf("timerfunction다음");
		if (DISP_TOOLS::cnt == 1000) {
			tmp = false;
		}
		*/

	}
}

void cExp_Size_Perception::handleSpecialKeys(int key, int x, int y)
{
	switch (key) {
	case GLUT_KEY_F3:
		if (!m_show_dbg_info) {
			m_show_dbg_info = true;
		}
		else m_show_dbg_info = false;
		break;
	case GLUT_KEY_F4:
		if (!m_show_surface) m_show_surface = true;
		else m_show_surface = false;
		break;
	case GLUT_KEY_F6:
		if (m_disp_mode) {
			m_disp_mode = false;
			DISP_TOOLS::setProjectionState(false);
		}
		else {
			m_disp_mode = true;
			DISP_TOOLS::setProjectionState(true);
		}
		break;
	case GLUT_KEY_F7:
		if (!m_enable_force) {
			m_enable_force = true;
		}
		else {
			m_enable_force = false;
			enablePositionCtrl(false);
		}
		break;
	case GLUT_KEY_F8:
		PHANTOM_TOOLS::change_direction();
		printf("방향 바꿈");
		

	}
}
/*
void cExp_Size_Perception::sendArd_ss(bool val1, bool val2)
{
	char tx_buf[4], buf[4];
	memset(tx_buf, 0x00, 4);
	sprintf_s(buf, "%c%c", val1 ? '1' : '0', val2 ? '1' : '0');
	memcpy(tx_buf, buf, 2);
	tx_buf[2] = '\n';
	m_pSerial_ss->Send(tx_buf, 3);
}

void cExp_Size_Perception::sendArd_vib(int val1, int val2)
{
	char tx_buf[7], buf[7];
	memset(tx_buf, 0x00, 7);
	sprintf_s(buf, "%03d%03d", val1, val2);
	memcpy(tx_buf, buf, 6);
	m_pSerial_vib->Send(tx_buf, 6);
}
*/

int cExp_Size_Perception::calcContact(glm::vec3 contact_force[2])
{
	int ret = 0, i;
	bool ret_proj, curr_contact = false;
	glm::vec3 min_dist_pt, curr_contact_pt, contact_vec;
	for (i = 0; i < 2; i++) {
		curr_contact = false;
		ret_proj = find_projection(m_sphere_center[i], glm::vec3(m_wall_pos[i], 0, 0), glm::vec3(m_wall_pos[i], 1, 0), glm::vec3(m_wall_pos[i], 0, 1), min_dist_pt);
		if (i == 0) {
			curr_contact_pt = m_sphere_center[0] + glm::vec3(m_sphr_radius, 0, 0);
			if (m_sphere_center[0].x + m_sphr_radius >= m_wall_pos[0]) curr_contact = true;
		}
		else {
			curr_contact_pt = m_sphere_center[1] + glm::vec3(-m_sphr_radius, 0, 0);
			if (m_sphere_center[1].x - m_sphr_radius <= m_wall_pos[1]) curr_contact = true;
		}
		if (curr_contact) contact_vec = min_dist_pt - curr_contact_pt;
		else contact_vec = glm::vec3(0, 0, 0);
		m_prev_proxy[i] = min_dist_pt;
		m_prev_contact_pt[i] = curr_contact_pt;
		m_prev_contact[i] = curr_contact;
		if (m_prev_contact[i]) m_prev_collision_depth[i] = sqrt(contact_vec[0] * contact_vec[0] + contact_vec[1] * contact_vec[1] + contact_vec[2] * contact_vec[2]);
		contact_force[i] = m_curr_K * contact_vec;
	}
	return ret;
}

int cExp_Size_Perception::calcContact2(glm::vec3 contact_force[2], int contact_state[2])
{
	int ret = 0, i;
	bool ret_proj, curr_contact = false;
	glm::vec3 min_dist_pt, curr_contact_pt, contact_vec;
	for (i = 0; i < 2; i++) {
		curr_contact = false;
		contact_state[i] = 0;
		ret_proj = find_projection(m_sphere_center[i], glm::vec3(m_wall_pos[i], 0, 0), glm::vec3(m_wall_pos[i], 1, 0), glm::vec3(m_wall_pos[i], 0, 1), min_dist_pt);
		if (i == 0) {
			curr_contact_pt = m_sphere_center[0] + glm::vec3(m_sphr_radius, 0, 0);
			if (m_sphere_center[0].x + m_sphr_radius >= m_wall_pos[0]) curr_contact = true;
		}
		else {
			curr_contact_pt = m_sphere_center[1] + glm::vec3(-m_sphr_radius, 0, 0);
			if (m_sphere_center[1].x - m_sphr_radius <= m_wall_pos[1]) curr_contact = true;
		}
		if (curr_contact) contact_vec = min_dist_pt - curr_contact_pt;
		else contact_vec = glm::vec3(0, 0, 0);
		m_prev_proxy[i] = min_dist_pt;
		m_prev_contact_pt[i] = curr_contact_pt;
		if (curr_contact != m_prev_contact[i]) {
			if (!curr_contact) contact_state[i] = 1;
			else contact_state[i] = 2;
		}
		m_prev_contact[i] = curr_contact;
		if (m_prev_contact[i]) m_prev_collision_depth[i] = sqrt(contact_vec[0] * contact_vec[0] + contact_vec[1] * contact_vec[1] + contact_vec[2] * contact_vec[2]);
		contact_force[i] = m_curr_K * contact_vec;
	}
	return ret;
}



void cExp_Size_Perception::init()
{
	////826 DAQ board initialization


	///// Threads initialization;
	//_beginthreadex(NULL, 0, NULL, (void*)this, 0, NULL);
	//_beginthreadex(NULL, 0, posCtrlThread, (void*)this, 0, NULL);
	_beginthreadex(NULL, 0, audioThread, (void*)this, 0, NULL);
	///// PHANToM initialization
	PHANTOM_TOOLS::initHD();
	//m_num_devices = PHANTOM_TOOLS::addDevice((char*)"Default Device", PHANTOM_TOOLS::TYPE::OMNI_GRIPPER, 0.0583);//0.0583);//0.0533);//0.01244);//0.0522);//PHANTOM_TOOLS::TYPE::CUSTOM);
	//m_num_devices = PHANTOM_TOOLS::addDevice((char*)"Touch2", PHANTOM_TOOLS::TYPE::OMNI_GRIPPER, 0.0583);//0.0583);//0.0533);//0.01244);//0.0522);	//PHANTOM_TOOLS::addDevice("Default Device", PHANTOM_TOOLS::TYPE::CUSTOM);
	/*PHANTOM_TOOLS::initDevice(PhantomCallback, this);
	PHANTOM_TOOLS::setOffset(0, glm::vec3(0.08452+0.006, 0, 0), glm::vec3(0, 0, 0));
	PHANTOM_TOOLS::setOffset(1, glm::vec3(-0.08452-0.006, 0, 0), glm::vec3(0, 0, 0));
//	PHANTOM_TOOLS::setOffset(0, glm::vec3(0.08452+0.006, 0, 0), glm::vec3(0, 0, 0));
//	PHANTOM_TOOLS::setOffset(1, glm::vec3(-0.08452-0.006, 0, 0), glm::vec3(0, 0, 0));
	_beginthreadex(NULL, 0, hapticRenderingThread, (void*)this, 0, NULL);
	*/
	///// fingertip surface computation
//	m_sphere_center_l[0] = glm::vec3(-m_dp_sphr_radius * cos(65.0*DTOR), 0, 0.01303 - m_sphr_radius * sin(65.0*DTOR));
//	m_sphere_center_l[1] = glm::vec3(m_dp_sphr_radius*cos(65.0*DTOR), 0, 0.01303 - m_sphr_radius * sin(65.0*DTOR));
	m_sphere_center_l[0] = glm::vec3(0.1912 - m_sphr_radius * sin(65.0 * DTOR), 0, 0.01409 - m_sphr_radius * cos(65.0 * DTOR));//glm::vec3(m_sphr_radius*cos(65.0*DTOR), 0, 0.02144 - m_sphr_radius*sin(65.5*DTOR));
	m_sphere_center_l[1] = glm::vec3(-0.01912 + m_sphr_radius * sin(65.0 * DTOR), 0, 0.01102 - m_sphr_radius * cos(65.0 * DTOR));//glm::vec3(-m_sphr_radius*cos(65.0*DTOR), 0, 0.01899 - m_sphr_radius*sin(65.0*DTOR));
	m_dp_sphere_center_l[0] = glm::vec3(0.01912 - m_dp_sphr_radius * sin(65.0 * DTOR), 0, 0.01409 - m_dp_sphr_radius * cos(65.0 * DTOR));;
	m_dp_sphere_center_l[1] = glm::vec3(-0.01912 + m_dp_sphr_radius * sin(65.0 * DTOR), 0, 0.01102 - m_dp_sphr_radius * cos(65.0 * DTOR));;
	DISP_TOOLS::setProjectionState(false);
	DISP_TOOLS::setCameraVariables(0, glm::vec3(0, m_view_height, 0.3));
	DISP_TOOLS::setCameraVariables(1, glm::vec3(0, m_view_height, 0));
	glClearColor(1, 1, 1, 1);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);
	///// Cutaneous interface initializing routines ...
	m_enable_cutaneous_render = false;
	enablePositionCtrl(true);
	Sleep(500);
	moveToPos_c(0, 5.0);
	Sleep(500);
	moveToPos_c(1, 4.5);
	Sleep(500);
	m_c_phase = CUTAN_PHASE::PRE_CONTACT;
	/////
	enablePositionCtrl(false);
	//setOutPwm(0, 0.0); 이거때문에 링크에러?
	//setOutPwm(1, 0.0);
	////
	srand(time(NULL));
	setExpVariables(0.06, 3, 0.02, 12, 0.0025);
}

void cExp_Size_Perception::moveToPos_c(uint idx, double target_pos)
{
	m_c_Ierror[idx] = 0.f;	m_c_Perror_old[idx] = 0.f;
	m_c_ref_pos[idx] = target_pos;
}
void cExp_Size_Perception::enableCutaneous(bool enable)
{
	uint i;
	if (enable) {
		for (i = 0; i < 2; i++) {
			m_c_Ierror[i] = 0.f;	m_c_Perror_old[i] = 0.f;
		}
		m_enable_cutaneous_render = true;
		//	enablePositionCtrl(true);
	}
	else {
		m_enable_cutaneous_render = false;
		//	enablePositionCtrl(false);
	}
}

void cExp_Size_Perception::enablePositionCtrl(bool enable)
{
	uint i;
	if (enable) {
		for (i = 0; i < 2; i++) {
			m_c_Ierror[i] = 0.f;	m_c_Perror_old[i] = 0.f;
		}
		m_enableTimer = true;
		SetEvent(m_hPosCtrlEvent);
	}
	else {
		m_enableTimer = false;
	}
}

int cExp_Size_Perception::updateAdc()
{
	double read_val, curr_pos;
	double alpha = 0.9;
	int i;
	int ret;
	if (ret == 0) {
		/// potentiometer position 0 (Omni 1)

	//	curr_pos = (read_val-2.82)/2.18*6.39;
		curr_pos = (read_val - 3.11) / 1.89 * 5.74;	// Feb 22nd 2019
		/*
#ifdef _DBG_REC

		m_dbg_dVal[0] = curr_pos;//read_val;
#endif
		m_c_displace[0] = alpha*m_c_prev_pos[0]+(1.0-alpha)*curr_pos;
		m_c_prev_pos[0] = m_c_displace[0];
		//// FSR read (Omni 1)

		m_FSR_V[0] = read_val;
#ifdef _DBG_REC
		m_dbg_dVal[1] = read_val;
#endif
		//// potentiometer position 0 (Omni 2)

//		curr_pos = (read_val-3.09)/1.91*5.29;	// 2017-08-31
		curr_pos = (read_val-3.73)/1.27*4.81;	// Feb 22nd 2019
#ifdef _DBG_REC
		m_dbg_dVal[2] = curr_pos;//read_val;
#endif
		m_c_displace[1] = alpha*m_c_prev_pos[1]+(1.0-alpha)*curr_pos;
		m_c_prev_pos[1] = m_c_displace[1];
		//// FSR read (Omni 2)

		m_FSR_V[1] = read_val;
#ifdef _DBG_REC
		m_dbg_dVal[3] = read_val;
#endif*/
////////////
		if (m_exp_phase == EXP_PHASE::DEVICE_INIT) {
			if (m_c_phase == CUTAN_PHASE::DEV1_CONTACT) {
				if (m_FSR_V[0] >= 0.1) {//15){//12){//0.05) {
					//setOutPwm(0, 0.0);
					m_c_contact_pos[0] = m_c_displace[0];
					m_c_phase = CUTAN_PHASE::DEV2_CONTACT;//INITIALIZED;
					printf("Cutaneous 1 initialized. Cutaneous contact position %f\n", m_c_contact_pos[0]);
					/// change contact sphere center position
					//setOutPwm(1, 0.95);
				}
			}
			else if (m_c_phase == CUTAN_PHASE::DEV2_CONTACT) {
				if (m_FSR_V[1] >= 0.1) {//15){//08){//0.05) {
					//setOutPwm(1, 0.0);
					m_c_contact_pos[1] = m_c_displace[1];
					m_c_phase = CUTAN_PHASE::INITIALIZED;
					printf("Cutaneous 2 initialized. Cutaneous contact position %f\n", m_c_contact_pos[1]);
					m_sphere_center_l[0] = glm::vec3(0.01912 - m_sphr_radius * sin(65.0 * DTOR), 0, 0.01409 + (0.001 * m_c_contact_pos[0]) - m_sphr_radius * cos(65.0 * DTOR));//glm::vec3(m_sphr_radius*cos(65.0*DTOR), 0, 0.02144 - m_sphr_radius*sin(65.5*DTOR));
					m_sphere_center_l[1] = glm::vec3(-0.01912 + m_sphr_radius * sin(65.0 * DTOR), 0, 0.01102 + (0.001 * m_c_contact_pos[1]) - m_sphr_radius * cos(65.0 * DTOR));
					m_dp_sphere_center_l[0] = glm::vec3(0.01912 - m_dp_sphr_radius * sin(65.0 * DTOR), 0, 0.01409 + (0.001 * m_c_contact_pos[0]) - m_dp_sphr_radius * cos(65.0 * DTOR));//glm::vec3(m_sphr_radius*cos(65.0*DTOR), 0, 0.02144 - m_sphr_radius*sin(65.5*DTOR));
					m_dp_sphere_center_l[1] = glm::vec3(-0.01912 + m_dp_sphr_radius * sin(65.0 * DTOR), 0, 0.01102 + (0.001 * m_c_contact_pos[1]) - m_dp_sphr_radius * cos(65.0 * DTOR));
					enablePositionCtrl(true);
					//moveToPos_c(m_c_contact_pos+2.0);//6.4);
					//Sleep(500);
					//m_c_Ierror = 0.f;	m_c_Perror_old = 0.f;
				//	m_enable_cutaneous_render = false;
					enableCutaneous(false);
					Sleep(500);
					ret = moveToNextPhase();
					//m_exp_phase = EXP_PHASE::INFO_INPUT;
				}
			}
		}
		else if (m_exp_phase == EXP_PHASE::TRAINING_PHASE1 || m_exp_phase == EXP_PHASE::EXP_PHASE1 || m_exp_phase == EXP_PHASE::EXP_PHASE4) {
			if ((m_prev_contact_pt[0].x <= m_border_pos[0]) && (m_prev_contact_pt[1].x >= m_border_pos[1])) {
				moveToNextPhase();
				//	printf("border check\n");
			}
		}
		else if (m_exp_phase == EXP_PHASE::EXP_PHASE2 || m_exp_phase == EXP_PHASE::EXP_PHASE5) {
			//	if((m_prev_contact_pt[0].x >= (m_wall_pos[0]-0.003)) || (m_prev_contact_pt[1].x <= (m_wall_pos[1]+0.003))) {
			if ((m_prev_contact_pt[0].x > (m_border_pos[0] + 0.002)) && (m_prev_contact_pt[1].x < (m_border_pos[1] - 0.002))) {
				//if(m_prev_contact_pt.y <= (m_plane_height + 0.005)) {
				moveToNextPhase();
				//	printf("proximity check\n");
			}
		}
	}
	return ret;
}


////////////////////////////////////////////////////////////////////////////////////////
// A callback function for the control of PHANToM type force-feedback interfaces
// WARNING: Do not include computationally heavy routine, e.g. a complex collision detection.
////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////
// Haptic rendering thread. Calculates the collision detection and thus the contact force.
//////////////////////////////////////////////////////////////////////////////////////
/*unsigned __stdcall hapticRenderingThread(void* arg)
{
	int i, ret;
#ifdef _DBG_REC
	int j;
	int contact_state[2];
	glm::vec3 e_pos;
#endif
#ifdef _VIB_TEST
	int input_val[2];
#endif
	glm::vec3 contact_force[2];
	glm::vec4 sphere_center, g_sphere_center, dp_sphere_center;
	glm::mat4 endeffectorTrans[2];
	cExp_Size_Perception* pExp = (cExp_Size_Perception*)arg;
	do {
		///////////////
		//PHANTOM_TOOLS::updateTransformation();
		///////////////
		for (i = 0; i < 2; i++) {
			sphere_center = glm::vec4(pExp->m_sphere_center_l[i].x, pExp->m_sphere_center_l[i].y, pExp->m_sphere_center_l[i].z, 1.0);
			dp_sphere_center = glm::vec4(pExp->m_dp_sphere_center_l[i].x, pExp->m_dp_sphere_center_l[i].y, pExp->m_dp_sphere_center_l[i].z, 1.0);
			//endeffectorTrans[i] = PHANTOM_TOOLS::getEndEffectorTansformation(i);
			g_sphere_center = endeffectorTrans[i] * sphere_center;
			pExp->m_sphere_center[i] = glm::vec3(g_sphere_center);
			g_sphere_center = endeffectorTrans[i] * dp_sphere_center;
			pExp->m_dp_sphere_center[i] = glm::vec3(g_sphere_center);
#ifdef _DBG_REC
			//e_pos = PHANTOM_TOOLS::getEndEffectorPosition(i);
			for (j = 0; j < 3; j++) {
				m_dbg_vec[i][j] = e_pos[j];
			}
#endif
		}
		// force rendering
//#ifndef _SS_TEST
//		ret = pExp->calcContact(contact_force);
//#else
		ret = pExp->calcContact2(contact_force, contact_state);

		//#endif
		for (i = 0; i < 2; i++) {
			if (pExp->m_prev_contact[i]) {
				pExp->m_cnt_collision_depth[i]++;
				pExp->m_sum_collision_depth[i] += pExp->m_prev_collision_depth[i];
				if (pExp->m_prev_collision_depth[i] > pExp->m_curr_max_coll_depth[i])  pExp->m_curr_max_coll_depth[i] = pExp->m_prev_collision_depth[i];
			}
		}
		//// force feedback
		if (pExp->m_enable_force) {
			contact_force[0][1] = 9.8 * pExp->m_tip_mass[0];
			contact_force[1][1] = 9.8 * pExp->m_tip_mass[1];
			//PHANTOM_TOOLS::setForce(0, contact_force[0]);
			//PHANTOM_TOOLS::setForce(1, contact_force[1]);
			//	printf("force (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f)\n", contact_force[0][0], contact_force[0][1], contact_force[0][2],
			//		contact_force[1][0], contact_force[1][1], contact_force[1][2]);
		}
		else {
			//PHANTOM_TOOLS::setForce(0, glm::vec3(0, 9.8 * pExp->m_tip_mass[0], 0));
			//PHANTOM_TOOLS::setForce(1, glm::vec3(0, 9.8 * pExp->m_tip_mass[1], 0));
		}

		if (contact_state[0] != 0 || contact_state[1] != 0) {	// change from the previous state
#ifdef _SS_TEST
			if (pExp->m_enable_ss) pExp->sendArd_ss(pExp->m_prev_contact[0], pExp->m_prev_contact[1]);
#endif
#ifdef _VIB_TEST
			if (pExp->m_enable_vib) {
				for (i = 0; i < 2; i++) {
					if (!pExp->m_prev_contact[i]) input_val[i] = 0;
					else input_val[i] = 255;
				}
				pExp->sendArd_vib(input_val[0], input_val[1]);
			}
#endif
		}
	} while (pExp->m_exp_phase != EXP_PHASE::QUIT_EXP);
	return 0;
}*/



unsigned __stdcall audioThread(void* arg)
{
	cExp_Size_Perception* pExp = (cExp_Size_Perception*)arg;
	cMCI_sound fanfare("Fanfare.wav"), noise("WhiteNoise_15min.mp3");
	int curr_audio_phase, prev_audio_phase;
	prev_audio_phase = AUDIO_PHASE::AUDIO_INIT;
	curr_audio_phase = pExp->m_audio_phase;
	do {
		curr_audio_phase = pExp->m_audio_phase;
		if (prev_audio_phase != curr_audio_phase) {
			switch (curr_audio_phase) {
			case AUDIO_PHASE::PLAY:
				noise.play(15000);
				break;
			case AUDIO_PHASE::PAUSE:
				noise.pause();
				break;
			case AUDIO_PHASE::STOP:
				noise.stop();
				break;
			case AUDIO_PHASE::COMPLETE:
				noise.stop();
				fanfare.play();
				break;
			}
			prev_audio_phase = curr_audio_phase;
			WaitForSingleObject(pExp->m_hAudioEvent, INFINITE);
		}
	} while (pExp->m_exp_phase != EXP_PHASE::QUIT_EXP);
	return 0;
}