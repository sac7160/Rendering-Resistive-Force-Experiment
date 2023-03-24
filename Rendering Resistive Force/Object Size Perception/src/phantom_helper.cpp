#include "phantom_helper.h"
# include <conio.h>
#include <cmath>

///
#include "SerialClass.h"
//Serial* SP = new Serial("\\\\.\\COM3");


#define MINUS 72
#define MINUS_SHIFT 80
#define PLUS 75
#define PLUS_SHIFT 77

static HHD ghHD = HD_INVALID_HANDLE;

static HDSchedulerHandle hUpdateDeviceCallback = HD_INVALID_HANDLE;

static HDboolean isActive = HD_FALSE;

/* Handle to haptic rendering context. */
//HHLRC ghHLRC;

//effect id
//HLuint gEffect;

/* Effect properties */
float gGain = 1.0f;
float gMagnitude = 1.0f;


bool once = true;

namespace PHANTOM_TOOLS
{
    void initHD()
    {
        HDErrorInfo error;

        ghHD = hdInitDevice(HD_DEFAULT_DEVICE);
        if (HD_DEVICE_ERROR(error = hdGetError()))
        {
            hduPrintError(stderr, &error, "Failed to initialize haptic device");
            fprintf(stderr, "Press any key to exit");
            getchar();
            exit(-1);
        }

        printf("Found device model: %s / serial number: %s.\n\n",
            hdGetString(HD_DEVICE_MODEL_TYPE), hdGetString(HD_DEVICE_SERIAL_NUMBER));

        kStiffness = 1.0; /* N/mm 이 변수로 N 조절*/
        
        hUpdateDeviceCallback = hdScheduleAsynchronous(
            DeviceStateCallback, 0, HD_MAX_SCHEDULER_PRIORITY);

        hdEnable(HD_FORCE_OUTPUT);
        hdStartScheduler();
    }
    /*
    void adjust_force()
    {
        if (_kbhit())
        {
            int key = toupper(_getch());

            switch (key)
            {
            case '_':
            case '-':
                if (kStiffness < -3)
                {
                    std::cout << "min force 입니다" << '\n';
                    break;
                }
                kStiffness -= 0.1;
                break;
            case '=':
            case '+':
                if (kStiffness > 3)
                {
                    std::cout << "max force 입니다" << '\n';
                    break;
                }
                kStiffness += 0.1;
                break;
            }
            once = true;
        }
    }*/
    void init_force(int trial)
    {
        if (trial == 3 || trial == 6 || trial == 9)
        {
            kStiffness = 2;
        }
        else 
        {
            kStiffness = 1;
        }
    }

    void adjust_force(unsigned char KeyPressed)
    {
        int key = toupper(KeyPressed);

        switch (key)
        {
        case '1':
            if (kStiffness < -6)
            {
                std::cout << "min force 입니다" << '\n';
                break;
            }
            kStiffness -= 0.1;
            break;
        case '2':
            if (kStiffness > 6)
            {
                std::cout << "max force 입니다" << '\n';
                break;
            }
            kStiffness += 0.1;
            break;
        }
        once = true;
    }

    void adjust_force2(double force)
    {
        if (kStiffness < -3.1) std::cout << "min force 입니다" << '\n';
        else if (kStiffness > 3.1) std::cout << "max force 입니다" << '\n';
        else
        {
            kStiffness += force;
            once = true;
        }
    }

    void exitHandler()
    {
        hdStopScheduler();
        hdUnschedule(hUpdateDeviceCallback);

        if (ghHD != HD_INVALID_HANDLE)
        {
            hdDisableDevice(ghHD);
            ghHD = HD_INVALID_HANDLE;
        }
    }

    void printOutput(double force)
    {
        std::ofstream writeFile("test.txt", std::ios::app);
        //writeFile.open("test.txt");

        std::string str = std::to_string(force);
        str += "\n";
        writeFile.write(str.c_str(), str.size());
        writeFile.close();
    }

    void change_direction()
    {
        if (bool_RighToLeft)  bool_RighToLeft = HD_FALSE;
        else bool_RighToLeft = HD_TRUE;
    }

    float get_kStiffness()
    {
        return kStiffness;
    }

    void set_tip_mass(float mass)
    {
        tip_mass = mass;
    }
    void renderForce()
    {
        bRenderForce = HD_TRUE;
    }
    double get_phantom_x_position()
    {
        return phantom_x_position;
    }
    VOID set_phantom_x_position(double position)
    {
        phantom_x_position = position;
    }
}


HDCallbackCode HDCALLBACK DeviceStateCallback(void* data)
{


    /* This is the position of the gravity well in cartesian
       (i.e. x,y,z) space. */
    static const hduVector3Dd wellPos = { -100,0,0 };

    HDErrorInfo error;
    hduVector3Dd position;
    hduVector3Dd force;
    hduVector3Dd positionTwell;


    HHD hHD = hdGetCurrentDevice();

    /* Begin haptics frame.  ( In general, all state-related haptics calls
       should be made within a frame. ) */
    hdBeginFrame(hHD);

    /* Get the current position of the device. */
    hdGetDoublev(HD_CURRENT_POSITION, position);

    memset(force, 0, sizeof(hduVector3Dd));

    hduVector3Dd sub = { 1,0,0 };
    hduVector3Dd previous_position;
    hduVecSubtract(previous_position, position, sub);    //현재 position x방향 -1 position

    hduVector3Dd tmp;
    hduVecSubtract(tmp, previous_position, position);


    //현재 position과 이전 position 1차이
    /* > F = k * x <
        F: Force in Newtons(N)
        k : Stiffness of the well(N / mm)
        x : 오른쪽 방향 1mm
    */

    //hduVecScale(force, tmp, kStiffness);

    /* Send the force to the device. */
    if (position[0] >= -10 && position[0] <=10) PHANTOM_TOOLS::bRenderForce = HD_TRUE;
    else PHANTOM_TOOLS::bRenderForce = HD_FALSE;

 
    HDdouble tmp_kStiffness;
    if (PHANTOM_TOOLS::bRenderForce)
    {
        if (PHANTOM_TOOLS::bool_RighToLeft)
        {
            tmp_kStiffness = - PHANTOM_TOOLS::kStiffness;
            //SP->WriteData("1", 255);
        }
        else
        {
            tmp_kStiffness = PHANTOM_TOOLS::kStiffness;
            //SP->WriteData("0", 255);
            
        }
    }
    else 
    {
        tmp_kStiffness = 0;
    }
    hduVecScale(force, tmp, tmp_kStiffness);

 
    /*float weight = PHANTOM_TOOLS::tip_mass;
    hduVector3Dd y = { 0,weight,0 };

    if (position[1] < 0)
    {
        static int directionFlag = 1;
        double penetrationDistance = fabs(position[1]);
        hduVector3Dd forceDirection(0, directionFlag, 0);

        // Hooke's law explicitly:
        double k = 1;
        hduVector3Dd x = penetrationDistance * forceDirection;
        hduVector3Dd f = k * x;
        hduVecAdd(y, y, f);
    } 
    
    if (position[0] < -80)
    {
        static int directionFlag = 1;
       
        hduVector3Dd forceDirection(directionFlag, 0, 0);
        double k = 1;
        hduVector3Dd x = 1 * forceDirection;
        hduVector3Dd f = k * x;
        hduVecAdd(y, y, f);
    }
    if (position[0] > 80)
    {
        static int directionFlag = -1;
        hduVector3Dd forceDirection(directionFlag, 0, 0);
        double k = 1;
        hduVector3Dd x = 1 * forceDirection;
        hduVector3Dd f = k * x;
        hduVecAdd(y, y, f);
    }
    if (position[2] > 0)
    {
        static int directionFlag = -1;
        hduVector3Dd forceDirection( 0, 0, directionFlag);
        double k = 1;
        hduVector3Dd x = 1 * forceDirection;
        hduVector3Dd f = k * x;
        hduVecAdd(y, y, f);
    }
  
    
    hduVecAdd(force, force, y);

    */
    
    //230324 read phantom position
    PHANTOM_TOOLS::set_phantom_x_position(position[0]);


    //230301 virtual plane 생성
    hduVector3Dd y = { 0,1,0 };
    if (position[1] < 100)
    {
        double penetrationDistance = fabs(position[1]-100);
        y = y * penetrationDistance;
        hduVecAdd(force, y, force);
    }

    if (position[0] < -150)
    {
        hduVector3Dd left_limit = { 0.5,0,0 };
        hduVecAdd(force, left_limit, force);
    }

    if (position[0] > 150)
    {
        hduVector3Dd right_limit = { -0.5,0,0 };
        hduVecAdd(force, right_limit, force);
    }
    
    if (position[2] > 80)
    {
        hduVector3Dd z_limit = { 0,0,-0.5 };
        hduVecAdd(force, z_limit, force);
    }
    

    float weight = PHANTOM_TOOLS::tip_mass;
    hduVector3Dd gravity = { 0,weight,0 };

    hduVecAdd(force, force, gravity);      /// 중력보상

    hdSetDoublev(HD_CURRENT_FORCE, force);
   

    if (once)
    {
        //std::cout << PHANTOM_TOOLS::kStiffness << "N 크기의 force 생성" << '\n';
        once = false;
        PHANTOM_TOOLS::printOutput(PHANTOM_TOOLS::kStiffness);
    }


    /* End haptics frame. */
    hdEndFrame(hHD);


    /* Check for errors and abort the callback if a scheduler error
       is detected. */
    if (HD_DEVICE_ERROR(error = hdGetError()))
    {
        hduPrintError(stderr, &error,
            "Error detected while rendering gravity well\n");

        if (hduIsSchedulerError(&error))
        {
            return HD_CALLBACK_DONE;
        }
    }

    /* Signify that the callback should continue running, i.e. that
       it will be called again the next scheduler tick. */
    return HD_CALLBACK_CONTINUE;
}