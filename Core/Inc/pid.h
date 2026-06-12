/**
  ******************************************************************************
  * @file    pid.h
  * @author  UF4
  * @date    26-6-12 下午2:59
  * @brief
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 UF4.
  * All rights reserved.
  *
  * This software is provided "as is", without warranty of any kind.
  *
  ******************************************************************************
  */
#ifndef PID_H
#define PID_H

// 环路的参数buck输出-恒压-PID型补偿器
#define BUCKPIDb0 5795
#define BUCKPIDb1 -11411
#define BUCKPIDb2 5617
// 环路的参数BOOST输出-恒压-PID型补偿器
#define BOOSTPIDb0 8844
#define BOOSTPIDb1 -17413
#define BOOSTPIDb2 8572

#define ILOOP_KP 6 // 电流环PID补偿器P值
#define ILOOP_KI 3 // 电流环PID补偿器I值
#define ILOOP_KD 1 // 电流环PID补偿器D值

void PID_Init(void);
void BuckBoostVILoopCtlPID(void);

#endif //PID_H
