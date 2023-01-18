#ifndef _CEXP_OBJECT_SIZE_PERCEPTION_H_
#define _CEXP_OBJECT_SIZE_PERCEPTION_H_
#include <Windows.h>
#include <process.h>
//#include <glm\glm.hpp>
#include "CAdaptive_PSE.h"
#include "geometry_helper.h"
#include "SerialClass.h"


class c826_DIO_ADC;
//#define _SS_TEST
#define _VIB_TEST

enum EXP_PHASE {
	INIT = 0,
	EXP_NULL,	//showing moving rectangle
	GET_ANSWER,
	DEVICE_INIT,	// Find the initial contact position.. 
	INFO_INPUT,
	TRAINING_PHASE1,	// Check if the contact point is over the border
	TRAINING_PHASE2,	// Training instruction
	TEST_PRE_EXP,
	EXP_PHASE1,	// Experiment initilization. 
	EXP_PHASE2,	// Checks if the contact point is close to the contact surface (not the border).
	EXP_PHASE3, // A participants feels the 1st stimulus of current trial.
	EXP_PHASE4, // Checks if the contact point is over the border
	EXP_PHASE5, // Checks if the contact point is close to the contact surface (not the border).
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

enum CUTAN_PHASE {
	C_INIT = 0,
	PRE_CONTACT,
	DEV1_CONTACT,
	DEV2_CONTACT,
	INITIALIZED
};

typedef unsigned int uint;
///// /////////////////////////////


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
	virtual int moveToNextPhase(char *ret_string = NULL);
	virtual void dataAnalysis();
	virtual void recordResult(int type);
	virtual int getCurrInstructionText(char pDestTxt[3][128]);
	void handleSpecialKeys(int key, int x, int y);
	void PhantomCallbackSubFunc();
	void init();
	void sub_display();
	int updateAdc();
	int calcContact(glm::vec3 contact_force[2]);
	int calcContact2(glm::vec3 contact_force[2], int contact_state[2]);
	void sendArd_ss(bool val1, bool val2);
	void sendArd_vib(int val1, int val2);
	void setAudioPhase(int audio_phase);
	int PeriodicTimerStart(uint period);
	int PeriodicTimerWait(uint* timestamp);
	int PeriodicTimerStop();
	int setOutPwm(int idx, double value);
	void enablePositionCtrl(bool enable);
	void onMultiMotion(int, int, int);
private:
	void enableCutaneous(bool enable);
	void initTrainingPhase(uint feedback);
	void moveToPos_c(uint idx, double target_pos);
	//-----------------------------------------------------------------------
	// MEMBERS:
	//-----------------------------------------------------------------------
public:
	static HANDLE m_hAudioEvent;
	static HANDLE m_hPosCtrlEvent;
	int m_exp_phase;
	int m_num_devices;
	double m_wall_pos[2];
	double  m_PWM_UpdateFreq;
	uint m_ch_timer;
	bool m_enableTimer;
	double m_c_contact_pos[2];
	double m_c_displace[2];	/// Cutaneous interface displacement
	double m_c_prev_pos[2];	/// Linear potentiometer previous position
	double m_c_ref_pos[2];	/// Cutaneous interface reference position
	uint m_c_phase;
	bool m_enable_cutaneous_render;
	glm::vec3 m_prev_proxy[2];
	glm::vec3 m_prev_contact_pt[2];
	bool m_prev_contact[2];
	double m_cutaneous_stiffness;
	double m_c_Ierror[2];
	double m_c_Perror_old[2];
	double m_c_Fprev[2];
	double m_c_Pgain[2];
	double m_c_Igain[2];
	double m_c_Dgain[2];
	glm::vec3 m_sphere_center_l[2];
	glm::vec3 m_dp_sphere_center_l[2];
	glm::vec3 m_sphere_center[2];
	glm::vec3 m_dp_sphere_center[2];
	double m_prev_collision_depth[2];
	double m_sum_collision_depth[2];
	uint m_cnt_collision_depth[2];	// to calculate mean sampled collision depth
	double m_curr_max_coll_depth[2];
	double m_first_max_coll_depth[2];
	bool m_enable_force;
	float m_tip_mass[2];
	float m_sphr_radius;
	float m_dp_sphr_radius;
	float square_pos;
	bool tmp; //애니메이션 추가하기위해 임시 추가
	int animation_cnt;
	Serial* m_pSerial_vib;
	vector<int>usr_input;	//결과 저장
	vector<float>force_change;
	time_t animation_start, animation_end;

#ifdef _SS_TEST
	bool m_enable_ss;
#endif
#ifdef _VIB_TEST
	bool m_enable_vib;
#endif
private:
	c826_DIO_ADC *m_p826;
	double m_view_height;
	char m_txtBuf[65];
	int m_textBuf_len;
	char m_rec_filename[128];
	bool m_disp_mode;
//	double m_thickness;
	float m_curr_K;
	uint m_training_feedback;	// 0: cutaneous+force 1: force only  2: cutaneous only
	int m_curr_answer;
	
	Serial *m_pSerial_ss;
	double m_FSR_V[2];
	double m_border_pos[2];
	bool m_show_border;
	bool m_show_fingertip;
	bool m_show_surface;
	double m_curr_obj_size;
	float m_exp_obj_size[2];
	uint m_training_size;
	int m_stimuli_ord;	// stimulus order 0: force/(force+cutaneous)   1: (force+cutaneous)/force
	double m_mean_coll_depth[2];
	double m_PSE_obj_size;
	bool m_show_dbg_info;
};

#endif// _CEXP_OBJECT_SIZE_PERCEPTION_H_
