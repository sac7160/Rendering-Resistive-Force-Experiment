#include "CExp_Size_Perception.h"
#include "CMCI_sound.h"
#include "phantom_helper.h"
#include "display_helper.h"

#include <random>

unsigned __stdcall audioThread(void* arg);

#define _DBG_REC
const char m_instruction_msg[][4][128] = {
	{{"Insert your insert your index finger and middle finger inside the omni."}, {"Then hit 'Enter' to initialize the experimental setup."}},	//INIT
	{{""}},	//EXP_PHASE_LRA
	{{"Which force was greater?"},{"left hand : Press '1'"},{"right hand : Press '2'"},{"if you want to feel again, hit'F9"}},	//GET_ANSWER
	{{"Intializing the contact position..."}}, // DEVICE_INIT 
	{{"subject ID:"}, {"Type subject ID and hit 'Enter' to begin training session."}, {""}}, // INFO_INPUT
	{{"Drag two fingers of your right hand along the moving red rectangle on the touch screen"}, {"Press 'n' to go to the next page"}},	// TRAINING_PHASE1
	{{"Training session2"}, {"If the force you feel in your right hand is greater than the force you feel in your left hand, press '2' "}, {"if(right hand < left hand) press '1'"}, {"Press 'e' to go to the next page "}},		// TRAINING PHASE2
	{{"Follow Square"}, {"To show moving sqaure, hit 'enter'"}},	//TEST_PRE_EXP
	{{"Trial No. :"} ,{}},	//EXP_PHASE_PRE_EXP
	{{"Trial No. :"}, {"Move your left hand from left to right."}},	//EXP_PHASE_TOUCH
	{{"Trial No. :"}, {"Change the direction the rectangle moves"}, {"Hit 'F8'"}},		// EXP_PHASE3
	{{"Trial No. :"}, {"Follow the red rectangle with your finger."}, {"Compare the force felt in the left hand with the force felt in the right hand"}, {"Get Ready to drag. It starts after 3 seconds after pressing the 'Enter'"}},	// EXP_PHASE4
	{{"Trial No. :"}, {"Follow the red rectangle with your finger."}, {"Compare the force felt in the left hand with the force felt in the right hand"}, {"Get Ready to drag. It starts after 3 seconds after pressing the 'Enter'"}},	// EXP_PHASE5
	{{"Trial No. :"}, {"Hit 'Enter' to move to next phase."}},		// EXP_PHASE6
	{{"Trial No. :"}, {"Change the direction the rectangle moves"}, {"Hit 'F8'"}},	//EXP_ANSWER
	{{"Trial No. :"},  {"Hit 'Enter' to move to the next trial."}},//EXP_ANSWER_CORRECTNESS
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
	m_exp_phase = EXP_PHASE::INIT;
	m_view_height = -0.04;
	m_textBuf_len = 0;
	m_tip_mass = 9.8 * 0.003;
	PHANTOM_TOOLS::set_tip_mass(m_tip_mass);
	finish_trial = false;

#ifdef _SS_TEST
	m_enable_ss = false;
#endif
#ifdef _VIB_TEST
	m_enable_vib = false;
#endif

	tmp = false;
	draw_phantom_position_square = false;
	square_pos = 300;
	animation_cnt = 0;
	direction_changed = false;
	lra_first = false;
}

cExp_Size_Perception::~cExp_Size_Perception()
{
}

int cExp_Size_Perception::gen_random_num()
{
	random_device rd;
	mt19937 gen(rd());
	uniform_int_distribution<int> dis(0, 1);
	int ret = dis(gen);
	return ret;
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
				ret = moveToNextPhase();
			}
		}
		else if (m_exp_phase == EXP_PHASE::INFO_INPUT) {
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
							sprintf(m_rec_filename, "%s/%s_%d.txt", directory_path, m_subjectID, 0);	// subject_ID exp_type reference_size.txt
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
		else if (m_exp_phase == EXP_PHASE::TRAINING_PHASE1) {
			if (key == 'n' || key == 'N') {
				moveToNextPhase(ret_string);
			}
		}
		else if (m_exp_phase == EXP_PHASE::TRAINING_PHASE2) {
			if (key == 'f' || key == 'F') {
				initTrainingPhase(0);
			}
			else if (key == 'e' || key == 'E') {
				ret = moveToNextPhase(ret_string);
			}
		}
		else if (m_exp_phase == EXP_PHASE::TEST_PRE_EXP) {
			if (key == 13) {
				ret = moveToNextPhase(ret_string);
				if (!tmp) {
					square_pos = 300;
					tmp = true;
					animation_start = clock();
				}
				/*else if (tmp) {
					tmp = false;
					square_pos = 0;
					glViewport(0, 0, (GLsizei)1000, (GLsizei)600);//width, height 하드코딩
				}*/
			}
		}
		else if (m_exp_phase == EXP_PHASE::EXP_PHASE_PRE_EXP) {
			if (key == 13)
			{
				if (lra_first)
				{
					if (!tmp) {
						animation_start = clock();
						square_pos = 300;
						tmp = true;
					}
					else if (tmp) {
						tmp = false;
						square_pos = 300;
						glViewport(0, 0, (GLsizei)1000, (GLsizei)600);//width, height 하드코딩
					}
				}
				else tmp = true;
				ret = moveToNextPhase(ret_string);
			}
			
		}
		else if (m_exp_phase == EXP_PHASE::GET_ANSWER)
		{
			int tmp;

			if (key == '1' || key == '2')
			{
				cout << PHANTOM_TOOLS::get_kStiffness() << endl;
				if (key == '1') {
					usr_input.push_back(1);
					force_change.push_back(PHANTOM_TOOLS::get_kStiffness());
					tmp = calcStimulus(PHANTOM_TOOLS::get_kStiffness(), 0);	//next stimulus 삭제하기
				}
				else if (key == '2')
				{
					usr_input.push_back(2);
					force_change.push_back(PHANTOM_TOOLS::get_kStiffness());
					tmp = calcStimulus(PHANTOM_TOOLS::get_kStiffness(), 1);
				}
			
				if (tmp) finish_trial = true;
				moveToNextPhase();
			}

		}
		else if (m_exp_phase == EXP_PHASE::EXP_PHASE_TOUCH)
		{
			if (key == 13)
			{
				//Sleep(3000);
				moveToNextPhase(ret_string);
				
				if (!lra_first)
				{
					
						animation_start = clock();
						square_pos = 300;
						tmp = true;
				
				}
				else tmp = false;
				draw_phantom_position_square = false;
				glViewport(0, 0, (GLsizei)1000, (GLsizei)600);
			}
		}
		else if (m_exp_phase == EXP_PHASE::EXP_PHASE3) {
			if (key == 13)
			{
				if (direction_changed) ret = moveToNextPhase(ret_string);
				else if (!direction_changed) sprintf(ret_string, "Hit 'F8' first!");
			}
		}
		else if (m_exp_phase == EXP_PHASE::EXP_PHASE6 ||
			m_exp_phase == EXP_PHASE::EXP_ANSWER_CORRECTNESS) {
			if (key == 13) {
				moveToNextPhase(ret_string);
			}
		}
		else if (m_exp_phase == EXP_PHASE::EXP_PHASE4 || m_exp_phase == EXP_PHASE::EXP_PHASE5) {
			if (key == '1' || key == '2')PHANTOM_TOOLS::adjust_force(key);
			else if (key == 13)
			{
				//Sleep(3000);
				moveToNextPhase(ret_string);
				if (!tmp) {
					square_pos = 700;
					tmp = true;
				}
				else if (tmp) {
					tmp = false;
					square_pos = 700;
					glViewport(0, 0, (GLsizei)1000, (GLsizei)600);//width, height 하드코딩
				}
			}
		}
		else if (m_exp_phase == EXP_PHASE::EXP_ANSWER) {
			if (key == 13)
			{
				if (direction_changed) ret = moveToNextPhase(ret_string);
				else if (!direction_changed) sprintf(ret_string, "Hit 'F8' first!");
			}
		}
	}
	return ret;
}

void cExp_Size_Perception::initTrainingPhase(uint feedback)
{
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
		m_exp_phase = EXP_PHASE::INFO_INPUT;
	}
	else if (m_exp_phase == EXP_PHASE::DEVICE_INIT) {

		m_exp_phase = EXP_PHASE::INFO_INPUT;
	}
	else if (m_exp_phase == EXP_PHASE::INFO_INPUT) {

#ifdef _SS_TEST
		m_enable_ss = false;
#endif
#ifdef _VIB_TEST
		m_enable_vib = false;
#endif
		m_exp_phase = EXP_PHASE::TRAINING_PHASE1;
	}
	else if (m_exp_phase == EXP_PHASE::TRAINING_PHASE1) {

		m_exp_phase = EXP_PHASE::TRAINING_PHASE2;
	}
	else if (m_exp_phase == EXP_PHASE::TRAINING_PHASE2) {
		m_curr_trial_no = 1;
		setAudioPhase(AUDIO_PHASE::PLAY);

		m_expResult.clear();
		time(&m_expBeginTime);
		time(&m_trialBeginTime);
		recordResult(RECORD_TYPE::REC_INIT);
		m_exp_phase = EXP_PHASE::TEST_PRE_EXP;
		//m_exp_phase = EXP_PHASE::EXP_PHASE1;
	}
	else if (m_exp_phase == EXP_PHASE::TEST_PRE_EXP) {
		//Sleep(100);
		//tmp = true;
		m_exp_phase = EXP_PHASE::EXP_PHASE_LRA;
	}
	else if (m_exp_phase == EXP_PHASE::EXP_PHASE_PRE_EXP)
	{
		if (lra_first == true) m_exp_phase = EXP_PHASE::EXP_PHASE_LRA;
		else if (lra_first == false) {
			m_exp_phase = EXP_PHASE::EXP_PHASE_TOUCH;
			draw_phantom_position_square = true;
		}
	}
	else if (m_exp_phase == EXP_PHASE::EXP_PHASE_LRA)
	{
		glViewport(0, 0, (GLsizei)1000, (GLsizei)600);//width, height 하드코딩
		animation_cnt++;


		//////////////////////////////////////////////////////////////////////////////////////////
		if (animation_cnt == 1) m_exp_phase = EXP_PHASE::EXP_PHASE_PRE_EXP;
		else if (lra_first == false) m_exp_phase = EXP_PHASE::GET_ANSWER;
		else if (lra_first == true) {
			m_exp_phase = EXP_PHASE::EXP_PHASE_TOUCH;
			draw_phantom_position_square = true;
		}
			//////////////////////////////////////////////////////////////////////////////////////////
	}
	else if (m_exp_phase == EXP_PHASE::GET_ANSWER) {
		//////////////////////////////////////////////////////////////////////////////////////////
		if (gen_random_num() == 0) lra_first = false;
		else lra_first = true;

		if (finish_trial) {
			m_exp_phase = EXP_PHASE::EXP_PHASE6;
		}
		else {	
			m_exp_phase = EXP_PHASE::EXP_PHASE_PRE_EXP;
		}
		//////////////////////////////////////////////////////////////////////////////////////////
		/*0209 코드 수정 방향 왼->오 고정, alternation관련 실험 끝내는 프로세스 추가
		else if (animation_cnt == 7) {
			m_exp_phase = EXP_PHASE::EXP_PHASE3;
		}
		else if (animation_cnt == 8) {
			m_exp_phase = EXP_PHASE::EXP_PHASE5;
		}
		else if (animation_cnt == 9) {
			m_exp_phase = EXP_PHASE::EXP_PHASE6;
		}*/
	}
	else if (m_exp_phase == EXP_PHASE::EXP_PHASE_TOUCH) {
		if (lra_first == false) m_exp_phase = EXP_PHASE::EXP_PHASE_LRA;
		else if (lra_first == true) m_exp_phase = EXP_PHASE::GET_ANSWER;
	}
	else if (m_exp_phase == EXP_PHASE::EXP_PHASE3) {
		direction_changed = false;
		m_exp_phase = EXP_PHASE::EXP_PHASE4;
	}
	else if (m_exp_phase == EXP_PHASE::EXP_PHASE4) {
		m_exp_phase = EXP_PHASE::EXP_PHASE_LRA;
	}
	else if (m_exp_phase == EXP_PHASE::EXP_PHASE5) {
		m_exp_phase = EXP_PHASE::EXP_PHASE_LRA;
	}
	else if (m_exp_phase == EXP_PHASE::EXP_PHASE6) {	//EXP_PHASE6 = trial 끝내는 phase
#ifdef _SS_TEST
		m_enable_ss = false;
#endif
#ifdef _VIB_TEST
		m_enable_vib = false;
#endif
		//sendArd_vib(0, 0);
		setAudioPhase(AUDIO_PHASE::PAUSE);
		m_exp_phase = EXP_PHASE::EXP_ANSWER_CORRECTNESS;
	}
	//else if (m_exp_phase == EXP_PHASE::EXP_ANSWER) {
		//	enablePositionCtrl(false);

		//direction_changed = false;
		//m_exp_phase = EXP_PHASE::EXP_ANSWER_CORRECTNESS;
	//}
	else if (m_exp_phase == EXP_PHASE::EXP_ANSWER_CORRECTNESS) {
		double next_stimulus;
		int input_answer;

		/// recording trial result
	//	if(!m_testSubject) {
		exp_result curr_result;
		curr_result.trial_begin_time = m_trialBeginTime;
		time(&curr_result.trial_end_time);
		curr_result.trial_no = m_curr_trial_no;


		for (int i = 0; i < usr_input.size(); i++) curr_result.omni_force_user_input.push_back(usr_input[i]);
		for (int i = 0; i < force_change.size(); i++) curr_result.omni_force_change.push_back(force_change[i]);
		force_change.clear();
		usr_input.clear();
		m_expResult.push_back(curr_result);

		//recordResult(RECORD_TYPE::REC_TRIAL);

		if (m_curr_trial_no != 9) {
			m_curr_trial_no++;
			time(&m_trialBeginTime);
			setAudioPhase(AUDIO_PHASE::PLAY);
			dataAnalysis();
			recordResult(RECORD_TYPE::REC_TRIAL); //추가
			/*trial 초기화*/
			m_curr_alt_phase = 0;
			m_curr_alt_no = 0;
			m_alt_val.clear();
			PHANTOM_TOOLS::init_force(m_curr_trial_no);
			finish_trial = false;
			animation_cnt = 1;

			m_exp_phase = EXP_PHASE::EXP_PHASE_PRE_EXP;
		}
		else {
			m_exp_phase = EXP_PHASE::DATA_ANALYSIS;
			dataAnalysis();
			recordResult(RECORD_TYPE::REC_TRIAL);//추가
			setAudioPhase(AUDIO_PHASE::COMPLETE);
			recordResult(RECORD_TYPE::REC_END);
			m_exp_phase = EXP_PHASE::EXP_DONE;
		}
	}
	return ret;
}

void cExp_Size_Perception::dataAnalysis()
{
	trial_result = calculatePSE();
	printf("result = %fN\n", trial_result);
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

		printf("Experiment began at %02d:%02d:%02d on %04d/%02d/%02d\n", time_tm.tm_hour, time_tm.tm_min, time_tm.tm_sec,
			time_tm.tm_year + 1900, time_tm.tm_mon + 1, time_tm.tm_mday);
		printf("-----------------------------------------------------------------------------------------------------------------\n");
		//printf("Trial no.\tObj size(mm)\tanswer(F object larger:0/F+C object larger:1)]\ttrial time (mm:ss)\tLeft)mean_collision_depth(F+C) (mm)\tmean_collision_depth(F) (mm)\tmax_collision_depth(F+C) (mm)\tmax_collision_depth(F)\tRight)mean_collision_depth(F+C) (mm)\tmean_collision_depth(F) (mm)\tmax_collision_depth(F+C) (mm)\tmax_collision_depth(F)\n");
		printf("Trial no.\t\t\ttrial time (mm:ss)\t\t\tuser input\n");
		printf("-----------------------------------------------------------------------------------------------------------------\n");
		pFile = fopen(m_rec_filename, "a");
		if (pFile != NULL && !m_testSubject) {
			fprintf(pFile, "Subject: %s\n", m_subjectID);
			fprintf(pFile, "Experiment began at %02d:%02d:%02d on %04d/%02d/%02d\n", time_tm.tm_hour, time_tm.tm_min, time_tm.tm_sec,
				time_tm.tm_year + 1900, time_tm.tm_mon + 1, time_tm.tm_mday);
			fprintf(pFile, "---------------------------------------------------------------------------------------------------------------\n");
			//fprintf(pFile, "Trial no.\tObj size(mm)\tanswer(F object larger:0/F+C object larger:1)]\ttrial time (mm:ss)\tLeft)mean_collision_depth(F+C) (mm)\tmean_collision_depth(F) (mm)\tmax_collision_depth(F+C) (mm)\tmax_collision_depth(F)\tRight)mean_collision_depth(F+C) (mm)\tmean_collision_depth(F) (mm)\tmax_collision_depth(F+C) (mm)\tmax_collision_depth(F)\n");
			//	"Trial no.\tstiffness(N/mm)\tanswer(kinesthetic surface stiffer:0/cutaneous surface stiffer1)]\ttrial time (mm:ss)\tLeft)mean_collision_depth(F+C) (mm)\tmean_collision_depth(F) (mm)\tmax_collision_depth(F+C) (mm)\tmax_collision_depth(F)\tRight)mean_collision_depth(F+C) (mm)\tmean_collision_depth(F) (mm)\tmax_collision_depth(F+C) (mm)\tmax_collision_depth(F)\n");
			fprintf(pFile, "Trial no.\t\t\ttrial time (mm:ss)\t\t\tuser input\n");
			fprintf(pFile, "---------------------------------------------------------------------------------------------------------------\n");
			fclose(pFile);
		}
	}
	else if (type == RECORD_TYPE::REC_TRIAL) {
		exp_result last_result = m_expResult.back();
		tot_time = difftime(last_result.trial_end_time, last_result.trial_begin_time);
		err = _localtime64_s(&time_tm, &tot_time);
		printf("\n");
		printf("%d\t time : \t%02d:%02d", last_result.trial_no, time_tm.tm_min, time_tm.tm_sec);
		for (int i = 0; i < last_result.omni_force_user_input.size(); i++) printf("\t%d->", last_result.omni_force_user_input[i]);
		printf("\n");

		/*printf("%d\t %.1f -> %.1f -> %.1f -> %.1f -> %.1f -> %.1f -> %.1f -> %.1f      \t%02d:%02d   \t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n \t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", last_result.trial_no,
			last_result.omni_force_change[0],
			last_result.omni_force_change[1],
			last_result.omni_force_change[2],
			last_result.omni_force_change[3],
			last_result.omni_force_change[4],
			last_result.omni_force_change[5],
			last_result.omni_force_change[6],
			last_result.omni_force_change[7],

			time_tm.tm_min, time_tm.tm_sec,

			last_result.omni_force_user_input[0],
			last_result.omni_force_user_input[1],
			last_result.omni_force_user_input[2],
			last_result.omni_force_user_input[3],
			last_result.omni_force_user_input[4],
			last_result.omni_force_user_input[5],
			last_result.omni_force_user_input[6],
			last_result.omni_force_user_input[7]);*/
		pFile = fopen(m_rec_filename, "a");
		if (pFile != NULL && !m_testSubject) {
			/*fprintf(pFile, "%d\t %.1f -> %.1f -> %.1f -> %.1f -> %.1f -> %.1f -> %.1f -> %.1f      \t%02d:%02d   \t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", last_result.trial_no,
				last_result.omni_force_change[0],
				last_result.omni_force_change[1],
				last_result.omni_force_change[2],
				last_result.omni_force_change[3],
				last_result.omni_force_change[4],
				last_result.omni_force_change[5],
				last_result.omni_force_change[6],
				last_result.omni_force_change[7],

				time_tm.tm_min, time_tm.tm_sec,

				last_result.omni_force_user_input[0],
				last_result.omni_force_user_input[1],
				last_result.omni_force_user_input[2],
				last_result.omni_force_user_input[3],
				last_result.omni_force_user_input[4],
				last_result.omni_force_user_input[5],
				last_result.omni_force_user_input[6],
				last_result.omni_force_user_input[7]);
				*/
			fprintf(pFile, "%d\t time : \t%02d:%02d", last_result.trial_no, time_tm.tm_min, time_tm.tm_sec);
			for (int i = 0; i < last_result.omni_force_user_input.size(); i++) fprintf(pFile, "\t%d->", last_result.omni_force_user_input[i]);
			fprintf(pFile, "\n");
			fprintf(pFile, "trial_result : %f", trial_result);
			fclose(pFile);
		}
	}
	else {	// RECORD_TYPE::REC_END
		time(&curr_time);
		err = _localtime64_s(&time_tm, &curr_time);
		tot_time = difftime(curr_time, m_expBeginTime);
		err = _localtime64_s(&time_tm2, &tot_time);
		/////
		printf("==================================================\n");
		printf("Experiment ended at %02d:%02d:%02d on %04d/%02d/%02d\n", time_tm.tm_hour,
			time_tm.tm_min, time_tm.tm_sec, time_tm.tm_year + 1900, time_tm.tm_mon + 1, time_tm.tm_mday);
		printf("Total experiment time: %02d:%02d\n", time_tm2.tm_min, time_tm2.tm_sec);
		printf("==================================================\n");
		//printf("object size PSE: %.3f mm\n", m_PSE_obj_size * 1000.0); //	printf("stiffness PSE: %.2f N/mm\n", m_PSE_K);
		printf("Size of Omni force : %.1f N 수정해야함\n", PHANTOM_TOOLS::get_kStiffness());
		printf("==================================================\n");
		pFile = fopen(m_rec_filename, "a");
		if (pFile != NULL && !m_testSubject) {
			fprintf(pFile, "==================================================\n");
			fprintf(pFile, "Experiment ended at %02d:%02d:%02d on %04d/%02d/%02d\n", time_tm.tm_hour,
				time_tm.tm_min, time_tm.tm_sec, time_tm.tm_year + 1900, time_tm.tm_mon + 1, time_tm.tm_mday);
			fprintf(pFile, "Total experiment time: %02d:%02d\n", time_tm2.tm_min, time_tm2.tm_sec);
			fprintf(pFile, "==================================================\n");
			//printf("object size PSE: %.3f mm\n", m_PSE_obj_size * 1000.0); //	printf("stiffness PSE: %.2f N/mm\n", m_PSE_K);
			fprintf(pFile, "Size of Omni force : %.1f N \n", PHANTOM_TOOLS::get_kStiffness());
			fprintf(pFile, "==================================================\n");
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
		else if (m_exp_phase == EXP_PHASE::EXP_PHASE_LRA) {
			sprintf_s(pDestTxt[0], "%s", m_instruction_msg[m_exp_phase][0], m_txtBuf);
		}
		else if (m_exp_phase == EXP_PHASE::GET_ANSWER) {
			sprintf_s(pDestTxt[0], "%s", m_instruction_msg[m_exp_phase][0], m_txtBuf);
		}
		else if (m_exp_phase == EXP_PHASE::TEST_PRE_EXP) {
			sprintf_s(pDestTxt[0], "%s %s", m_instruction_msg[m_exp_phase][0], m_txtBuf);
		}
		else if (m_exp_phase == EXP_PHASE::EXP_PHASE_PRE_EXP) {
			char pTxt[100];
			sprintf(pDestTxt[0], "%s %d", m_instruction_msg[m_exp_phase][0], m_curr_trial_no);
			if (lra_first) {
				sprintf(pTxt, "\nFollow the red rectangle with your right hand. Remember the force.  Hit 'Enter'");
				strcat(pDestTxt[0], pTxt);
			}
			else
			{
				sprintf(pTxt, "\nRemember the force in your left hand. Hit 'Enter'");
				strcat(pDestTxt[0], pTxt);
			}
		}
		else if (m_exp_phase >= EXP_PHASE::EXP_PHASE_TOUCH && m_exp_phase <= EXP_PHASE::EXP_ANSWER_CORRECTNESS) {
			sprintf_s(pDestTxt[0], "%s %d", m_instruction_msg[m_exp_phase][0], m_curr_trial_no);
			char pTxt[100];
			if (lra_first && m_exp_phase == EXP_PHASE::EXP_PHASE_TOUCH) {
				sprintf(pTxt, "Compare the force. Hit 'Enter'.");
				strcat(pDestTxt[0], pTxt);
			}
			else if(!lra_first && m_exp_phase == EXP_PHASE::EXP_PHASE_TOUCH)
			{
				sprintf(pTxt, "\nRemember the force in your left hand. Hit 'Enter'");
				strcat(pDestTxt[0], pTxt);
			}
		}
		else if (m_exp_phase == EXP_PHASE::TRAINING_PHASE2) {
			sprintf_s(pDestTxt[0], "%s Object Perception Experiment", m_instruction_msg[m_exp_phase][0]);
		}
		else
		{
			strcpy_s(pDestTxt[0], m_instruction_msg[m_exp_phase][0]);
			pDestTxt[0][len[0]] = NULL;
		}
		/// msg 2
		if (m_exp_phase == EXP_PHASE::EXP_ANSWER_CORRECTNESS) {
			int curr_answer = m_txtBuf[0] - '0';
			sprintf(pDestTxt[1], "%s", m_instruction_msg[m_exp_phase][1]);
		}
		else {
			strcpy_s(pDestTxt[1], m_instruction_msg[m_exp_phase][1]);
			pDestTxt[1][len[1]] = NULL;
		}
		/// msg 3
		if (m_exp_phase == EXP_PHASE::EXP_ANSWER) {
			char pTxt[50];
			sprintf(pDestTxt[2], "%s", m_instruction_msg[m_exp_phase][2]);
			if (direction_changed) {
				sprintf(pTxt, " direction changed! Hit 'Enter'");
				strcat(pDestTxt[2], pTxt);
			}
		}
		else {
			strcpy_s(pDestTxt[2], m_instruction_msg[m_exp_phase][2]);
			pDestTxt[2][len[2]] = NULL;
		}
		/// msg 4
		strcpy_s(pDestTxt[3], m_instruction_msg[m_exp_phase][3]);
		pDestTxt[3][len[3]] = NULL;

		if (m_exp_phase == EXP_PHASE::EXP_PHASE3)
		{
			char pTxt[50];
			sprintf(pDestTxt[2], "%s", m_instruction_msg[m_exp_phase][2]);
			if (direction_changed) {
				sprintf(pTxt, " direction changed! Hit 'Enter'");
				strcat(pDestTxt[2], pTxt);
			}
		}
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
	glColor3f(0, 0, 0);
	DISP_TOOLS::Draw_Text(pTxt[0], -0.15f, m_view_height + 0.13f, 0.f);	//	DISP_TOOLS::Draw_Text(pTxt[0], -7.2f, 3.8f, 0.f);
	DISP_TOOLS::Draw_Text(pTxt[1], -0.15f, m_view_height + 0.12f, 0.f);	// 	DISP_TOOLS::Draw_Text(pTxt[1], -7.2f, 3.2f, 0.f);
	DISP_TOOLS::Draw_Text(pTxt[2], -0.15f, m_view_height + 0.11f, 0.f);	// 	DISP_TOOLS::Draw_Text(pTxt[2], -7.2f, 2.6f, 0.f);
	DISP_TOOLS::Draw_Text(pTxt[3], -0.15F, m_view_height + 0.10f, 0.f);

	if (tmp) {
		glColor3f(1, 0, 0);
		DISP_TOOLS::DrawSquare(square_pos,250);
	}
	if (draw_phantom_position_square) {
		glColor3f(0, 1, 0);
		DISP_TOOLS::DrawSquare(fabs(PHANTOM_TOOLS::get_phantom_x_position())+300, 100);
	}
}

void cExp_Size_Perception::handleSpecialKeys(int key, int x, int y)
{
	switch (key) {
	case GLUT_KEY_F8:
		PHANTOM_TOOLS::change_direction();
		if (!direction_changed) direction_changed = true;
		break;
	case GLUT_KEY_F9:
		m_exp_phase = EXP_PHASE::EXP_PHASE_LRA;
		animation_cnt--;
		square_pos = 300;
		tmp = true;
	}
	

}


void cExp_Size_Perception::init()
{
	///// Threads initialization;
	_beginthreadex(NULL, 0, audioThread, (void*)this, 0, NULL);
	///// PHANToM initialization
	PHANTOM_TOOLS::initHD();

	DISP_TOOLS::setProjectionState(false);
	DISP_TOOLS::setCameraVariables(0, glm::vec3(0, m_view_height, 0.3));
	DISP_TOOLS::setCameraVariables(1, glm::vec3(0, m_view_height, 0));
	glClearColor(1, 1, 1, 1);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);

	srand(time(NULL));
	setExpVariables(0.06, 3, 0.4, 12, 0.1);
}



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