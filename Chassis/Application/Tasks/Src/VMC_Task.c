#include "VMC_Task.h"

#include "cmsis_os.h"
#include "Control_Task.h"
#include "INS_Task.h"
#include "ramp.h"
#include "pid.h"
#include "motor.h"
#include "arm_math.h"
#include "remote_control.h"
#include "bsp_uart.h"
#include "config.h"


VMC_Info_Typedef Left_VMC_Info={
   .L1 = 0.062f,
   .L4 = 0.062f,
	 .L2 = 0.1f,
   .L3 = 0.1f,
	 .L5 = 0.082f,
   .VMC_USAGE = Left_VMC,
	 .Phi1 =  &Damiao_Motor[Left_Anterior_Joint].Data.Position,
   .Phi4 = &Damiao_Motor[Left_Posterior_Joint].Data.Position,
};
VMC_Info_Typedef Right_VMC_Info={
   .L1 = 0.062f,
   .L4 = 0.062f,
	 .L2 = 0.1f,
   .L3 = 0.1f,
	 .L5 = 0.082f,
   .VMC_USAGE = Right_VMC,
	 .Phi1 = &Damiao_Motor[Right_Posterior_Joint].Data.Position,
   .Phi4 = &Damiao_Motor[Right_Anterior_Joint].Data.Position,
};
VMC_Rad_to_Angle_Info_Typedef VMC_Rad_to_Angle_Info;
static void VMC_Calculate(VMC_Info_Typedef *VMC_Info);

static void VMC_F_Tp_To_Joint_Calculate(VMC_Info_Typedef *VMC_Info,Control_Info_Typedef *Control_Info);
static void VMC_rad_to_Angle(VMC_Info_Typedef *VMC_Info,VMC_Rad_to_Angle_Info_Typedef *VMC_Rad_to_Angle_Info);
PID_Info_TypeDef PID_Leg_length_thrust;
static float PID_Leg_length_thrust_param[6]={50.f,0.2f,40.f,0,300,400};
#define printf(title, fmt, args...) usart_printf("{"#title"}"fmt"\n", ##args)
void VMC_Task(void const * argument)
{
  /* USER CODE BEGIN CAN_Task */
  TickType_t systick = 0;
  PID_Init(&PID_Leg_length_thrust,PID_POSITION,PID_Leg_length_thrust_param);
  for(;;)
  {
   
	systick = osKernelSysTick();
	VMC_Calculate(&Left_VMC_Info);
	VMC_Calculate(&Right_VMC_Info);
	VMC_F_Tp_To_Joint_Calculate(&Left_VMC_Info,&Control_Info);
	VMC_F_Tp_To_Joint_Calculate(&Right_VMC_Info,&Control_Info); 
	VMC_rad_to_Angle(&Left_VMC_Info,&VMC_Rad_to_Angle_Info);
	//	printf(theta,"%f",Control_Info.Measure.Theta_dot);
    osDelayUntil(&systick,2);

  }
}
  /* USER CODE END CAN_Task */

static void VMC_Calculate(VMC_Info_Typedef *VMC_Info){
  VMC_Info->X_B = VMC_Info->L1 * arm_cos_f32(*VMC_Info->Phi1);
  VMC_Info->Y_B = VMC_Info->L1 * arm_sin_f32(*VMC_Info->Phi1);
	VMC_Info->X_D = VMC_Info->L5 + VMC_Info->L4 * arm_cos_f32(*VMC_Info->Phi4);
  VMC_Info->Y_D = VMC_Info->L4 * arm_sin_f32(*VMC_Info->Phi4);
	float X_D_Difference_X_B =  VMC_Info->X_D -  VMC_Info->X_B;
	float Y_D_Difference_Y_B =  VMC_Info->Y_D -  VMC_Info->Y_B;
  float LBD_2 =   X_D_Difference_X_B*X_D_Difference_X_B +Y_D_Difference_Y_B*Y_D_Difference_Y_B;
	float A0 =  2 * VMC_Info->L2  * X_D_Difference_X_B;
  float B0 =  2 * VMC_Info->L2  * Y_D_Difference_Y_B;
	float sqrt_out1;
	arm_sqrt_f32(A0*A0 + B0*B0 - LBD_2*LBD_2,&sqrt_out1);
  VMC_Info->Phi2 =2 * atan2f(B0+sqrt_out1 , A0 + LBD_2);
  VMC_Info->X_C  = VMC_Info->X_B + VMC_Info->L2 * arm_cos_f32(VMC_Info->Phi2);
  VMC_Info->Y_C  = VMC_Info->Y_B + VMC_Info->L2 * arm_sin_f32(VMC_Info->Phi2);
  VMC_Info->Phi3 = atan2f( VMC_Info->Y_C- VMC_Info->Y_D,VMC_Info->X_C- VMC_Info->X_D);
	arm_sqrt_f32(powf(VMC_Info->X_C - VMC_Info->L5/2,2) + VMC_Info->Y_C*VMC_Info->Y_C,&VMC_Info->L0);
  VMC_Info->Phi0 = atan2f(VMC_Info->Y_C,VMC_Info->X_C - (VMC_Info->L5/2));
	//VMC_Info->Phi0_dot = (VMC_Info->Phi0 - VMC_Info->Last_Phi0)/0.002f;
//	  VMC_Info->Predict_Phi0 =  VMC_Info->Phi0  +   VMC_Info->Phi0_dot*0.00125f;
//	  VMC_Info->Last_Phi0 = VMC_Info->Phi0;
}
//static void VMC_Predict_Calculate(VMC_Info_Typedef *VMC_Info){
//	  VMC_Info->Predict_Phi1 =  *VMC_Info->Phi1 +   (*VMC_Info->Phi1_dot)*0.005f;
//	  VMC_Info->Predict_Phi4 =  *VMC_Info->Phi4 +   (*VMC_Info->Phi4_dot)*0.005f;
//    VMC_Info->X_B = VMC_Info->L1 * arm_cos_f32(VMC_Info->Predict_Phi1);
//    VMC_Info->Y_B = VMC_Info->L1 * arm_sin_f32(VMC_Info->Predict_Phi1);
//	  VMC_Info->X_D = VMC_Info->L5 + VMC_Info->L4 * arm_cos_f32(VMC_Info->Predict_Phi4);
//    VMC_Info->Y_D = VMC_Info->L4 * arm_sin_f32(VMC_Info->Predict_Phi4);
//	  float X_D_Difference_X_B =  VMC_Info->X_D -  VMC_Info->X_B;
//	  float Y_D_Difference_Y_B =  VMC_Info->Y_D -  VMC_Info->Y_B;
//	  float LBD_2 =   X_D_Difference_X_B*X_D_Difference_X_B +Y_D_Difference_Y_B*Y_D_Difference_Y_B;
//	  float A0 =  2 * VMC_Info->L2  * X_D_Difference_X_B;
//	  float B0 =  2 * VMC_Info->L2  * Y_D_Difference_Y_B;
//	  float sqrt_out1;
//	  arm_sqrt_f32(A0*A0 + B0*B0 - LBD_2*LBD_2,&sqrt_out1);
//    VMC_Info->Predict_Phi2 =2 * atan2f(B0+sqrt_out1 , A0 + LBD_2);
//    VMC_Info->X_C  = VMC_Info->X_B + VMC_Info->L2 * arm_cos_f32(VMC_Info->Predict_Phi2);
//    VMC_Info->Y_C  = VMC_Info->Y_B + VMC_Info->L2 * arm_sin_f32(VMC_Info->Predict_Phi2);
//    //VMC_Info->Phi3 = atan2f( VMC_Info->Y_C- VMC_Info->Y_D,VMC_Info->X_C- VMC_Info->X_D);
//	  //arm_sqrt_f32(powf(VMC_Info->X_C - VMC_Info->L5/2,2) + VMC_Info->Y_C*VMC_Info->Y_C,&VMC_Info->L0);
//    VMC_Info->Predict_Phi0 = atan2f(VMC_Info->Y_C,VMC_Info->X_C - (VMC_Info->L5/2));
//    VMC_Info->Phi0_dot =   (VMC_Info->Predict_Phi0-  VMC_Info->Phi0)/0.005f;
//    VMC_Info->Filter_Phi0_dot = LowPassFilter1p_Update(&VMC_Info->Phi0_dot_LowPassFilter, VMC_Info->Phi0_dot);



//}

static void VMC_F_Tp_To_Joint_Calculate(VMC_Info_Typedef *VMC_Info,Control_Info_Typedef *Control_Info){
 
	 if(VMC_Info->L0 > 0.02f){
   VMC_Info->F =   f_PID_Calculate(&PID_Leg_length_thrust,VMC_Info->Target_L0,VMC_Info->L0);
	 } else  VMC_Info->F = 0;
   VMC_Info ->Last_Target_L0 =  VMC_Info->Target_L0;
  float Phi3_2 =  ( VMC_Info->Phi3 - VMC_Info->Phi2);
  float Phi0_3 =  ( VMC_Info->Phi0 -  VMC_Info->Phi3);
  float Phi1_2 =  ((*VMC_Info->Phi1) -  VMC_Info->Phi2);
  VMC_Info->T1 =  ((VMC_Info->L1 * arm_sin_f32(Phi1_2))*(VMC_Info->F * VMC_Info->L0 * arm_sin_f32(Phi0_3)+ VMC_Info->Tp * arm_cos_f32(Phi0_3)))/(VMC_Info->L0*arm_sin_f32(Phi3_2));
  float Phi0_2 =  ( VMC_Info->Phi0 -  VMC_Info->Phi2);
  float Phi3_4 =  ( VMC_Info->Phi3 - (*VMC_Info->Phi4));
  VMC_Info->T2 =  ((VMC_Info->L4 * arm_sin_f32(Phi3_4))*(VMC_Info->F * VMC_Info->L0 * arm_sin_f32(Phi0_2)+ VMC_Info->Tp * arm_cos_f32(Phi0_2)))/(VMC_Info->L0*arm_sin_f32(Phi3_2));
}

static void VMC_rad_to_Angle(VMC_Info_Typedef *VMC_Info,VMC_Rad_to_Angle_Info_Typedef *VMC_Rad_to_Angle_Info){
    VMC_Rad_to_Angle_Info->Phi0 =  VMC_Info->Phi0*Rad_to_angle;
	VMC_Rad_to_Angle_Info->Phi1 =  *VMC_Info->Phi1*Rad_to_angle;
	VMC_Rad_to_Angle_Info->Phi2 =  VMC_Info->Phi2*Rad_to_angle;
	VMC_Rad_to_Angle_Info->Phi3 =  VMC_Info->Phi3*Rad_to_angle;
	VMC_Rad_to_Angle_Info->Phi4 =  *VMC_Info->Phi4*Rad_to_angle;
	
}
