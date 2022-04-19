#ifndef _WM_BT_APP_H
#define _WM_BT_APP_H

/*****************************************************************************
**
**  Name:           wm_bt_app.h
**
**  Description:    This file contains the sample functions for bluetooth application
**
*****************************************************************************/
#include "wm_bt.h"

int demo_bt_enable();

int demo_ble_server_on();

extern volatile tls_bt_state_t bt_adapter_state;
#endif
