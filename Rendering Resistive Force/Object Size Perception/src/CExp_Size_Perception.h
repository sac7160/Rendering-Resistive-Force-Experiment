#ifndef _CEXP_OBJECT_SIZE_PERCEPTION_H_
#define _CEXP_OBJECT_SIZE_PERCEPTION_H_
#include <Windows.h>
#include <process.h>
#include "CAdaptive_PSE.h"
#include "geometry_helper.h"
#include "SerialClass.h"



#define _VIB_TEST

enum EXP_PHASE {
	INIT = 0,
	EXP_PHASE_LRA,	//showing moving rectangle
	GET_ANSWER,
	DEVICE_INIT,	// Find the initial contact position.. 
	INFO_INPUT,
	TRAINING_PHASE1,	// Check if the contact point is over the border
	TRAINING_PHASE2,	// Training instruction
	TEST_PRE_EXP, 
	EXP_PHASE_PRE_EXP,
	EXP_PHASE_TOUCH,	//Generate Omni_Touch Force
	EXP_PHASE3, // A participants feels the 1st stimulus of current trial.
	EXP_PHASE4, 
	EXP_PHASE5,
	EXP_PHASE6,	//  A participants feels the 2nd stimulus of current trial. 
	EXP_ANSWER,	//  Type your answer and hit 'Enter' (0: first stimulus was larger, 1: second stimulus was larger)
	EXP_ANSWER_CORRECTNESS,	// Your answer for this trial is.... Hit 'Enter' to move to the next trial.
	DATA_ANALYSIS,
	WRITE_RESULT,
	EXP_DONE,
	QUIT_EXP
};

enum AUDIO_PHASE {
	AUDIO_INIT = 0,
	PLAY,
	PAUSE,
	STOP,
	COMPLETE
};

enum RECORD_TYPE {
	REC_INIT = 0,
	REC_TRIAL,
	REC_END
};

typedef unsigned int uint;


class cExp_Size_Perception : public cAdaptive_PSE
{
public:
	//-----------------------------------------------------------------------
	// CONSTRUCTOR & DESTRUCTOR:
	//-----------------------------------------------------------------------
	cExp_Size_Perception();
	~cExp_Size_Perception();
	//-----------------------------------------------------------------------
	// METHODS:
	//-----------------------------------------------------------------------
	virtual int handleKeyboard(unsigned char key, char* ret_string);	// pure virtual functions
	virtual int moveToNextPhase(char* ret_string = NULL);
	virtual void dataAnalysis();
	virtual void recordResult(int type);
	virtual int getCurrInstructionText(char pDestTxt[3][128]);
	void handleSpecialKeys(int key, int x, int y);
	void init();
	void sub_display();
	void setAudioPhase(int audio_phase);
	int gen_random_num();
	char get_left_time();
	void set_left_time(int);
	
private:
	void initTrainingPhase(uint feedback);
	//-----------------------------------------------------------------------
	// MEMBERS:
	//-----------------------------------------------------------------------
public:
	static HANDLE m_hAudioEvent;
	static HANDLE m_hPosCtrlEvent;
	int m_exp_phase;
	int m_num_devices;

	float m_tip_mass;
	float square_pos;
	bool tmp; //애니메이션 추가하기위해 임시 추가
	bool draw_phantom_position_square;

	//int animation_cnt;
	vector<int>usr_input;	//결과 저장
	vector<float>force_change;
	time_t animation_start, animation_end;
	bool direction_changed;
	bool lra_first;	//omni, lra 순서 random하게 하기 위한 변수 
	bool finish_trial;
	double trial_result;
    int left_time;	//직사각형 움직이기까지 남은 시간

#ifdef _SS_TEST
	bool m_enable_ss;
#endif
#ifdef _VIB_TEST
	bool m_enable_vib;
#endif
private:
	double m_view_height;
	char m_txtBuf[65];
	int m_textBuf_len;
	char m_rec_filename[128];
};

#endif// _CEXP_OBJECT_SIZE_PERCEPTION_H_
