/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-03-20     Rick       the first version
 */
#include "rtthread.h"
#include "board.h"
#include "radio_protocol.h"
#include "radio_protocol_mainunit.h"

#define DBG_TAG "radio_protocol_mainunit"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

static uint8_t radio_door_delay_flag = 0;
static uint8_t radio_mainunit_rssi = 0;

uint8_t radio_mainunit_rssi_get(void)
{
    return radio_mainunit_rssi;
}

void radio_door_delay_flag_set(uint8_t value)
{
    radio_door_delay_flag = value;
}

uint8_t radio_door_delay_flag_get(void)
{
    return radio_door_delay_flag;
}

static void radio_frame_mainunit_parse_heart(rx_format *rx_frame)
{
    tx_format tx_frame = {0};
    uint8_t sub_command = rx_frame->rx_data[2];

    switch(sub_command)
    {
    case 0://normal heart
        Stop_Heart_Timer();
        break;
    case 1://rssi test
        led_rssi_show();
        break;
    default:
        break;
    }


    LOG_I("radio_frame_endunit_parse_heart 0x%08X\r\n",rx_frame->source_addr);
}

static void radio_frame_mainunit_parse_learn(rx_format *rx_frame)
{
    uint8_t send_value = 0;
    tx_format tx_frame = {0};
    uint8_t sub_command = rx_frame->rx_data[2];

    switch(sub_command)
    {
    case 0://learn ack
        send_value = 1;
        tx_frame.msg_ack = RT_TRUE;
        tx_frame.msg_type = MSG_UNCONFIRMED_UPLINK;
        tx_frame.dest_addr = rx_frame->source_addr;
        tx_frame.source_addr = get_local_address();
        tx_frame.command = LEARN_DEVICE_CMD;
        tx_frame.tx_data = &send_value;
        tx_frame.tx_len = 1;
        radio_mainunit_command_send(&tx_frame);
        LOG_I("radio_frame_mainunit_parse_learn request %d\r\n",rx_frame->source_addr);
        break;
    case 1://learn done
        Storage_Main_Write(rx_frame->source_addr);
        Led_LearnSuceess();
        LOG_I("radio_frame_mainunit_parse_learn done %d\r\n",rx_frame->source_addr);
        break;
    default:
        break;
    }
}

static void radio_frame_mainunit_parse_valve(rx_format *rx_frame)
{
    uint8_t value = rx_frame->rx_data[2];
    switch(value)
    {
    case 0://open
        break;
    case 1://close
        break;
    case 2://delay open
        radio_door_delay_flag = 0;
        break;
    case 3://delay close
        radio_door_delay_flag = 1;
        break;
    default:
        break;
    }

    if(rx_frame->rssi >= -100 && rx_frame->snr >= 0)
    {
        Beep_Recv();
    }
    Heart_Refresh();
}

void radio_mainunit_rssi_set(int value)
{
    if(value >= -65)
    {
        radio_mainunit_rssi = 0;
    }
    else if(value < -65 && value >= -90)
    {
        radio_mainunit_rssi = 1;
    }
    else if(value < -90 && value >= -100)
    {
        radio_mainunit_rssi = 2;
    }
    else if(value < -100)
    {
        radio_mainunit_rssi = 3;
    }

    LOG_I("radio_mainunit_rssi_set rssi %d,rssi_level %d\r\n",value,radio_mainunit_rssi);
}

void radio_frame_mainunit_parse(rx_format *rx_frame)
{
    if(rx_frame->rx_data[0] == LEARN_DEVICE_CMD)//learn device ignore address check
    {
        radio_frame_mainunit_parse_learn(rx_frame);
    }

    if((rx_frame->dest_addr != get_local_address()) || (rx_frame->source_addr != Storage_Main_Read()))
    {
        return;
    }

    radio_mainunit_rssi_set(rx_frame->rssi);

    uint8_t command = rx_frame->rx_data[0];
    switch(command)
    {
    case HEART_UPLOAD_CMD:
        radio_frame_mainunit_parse_heart(rx_frame);
        break;
    case CONTROL_VALVE_CMD:
        radio_frame_mainunit_parse_valve(rx_frame);
        break;
    default:
        break;
    }
}

void radio_mainunit_command_send(tx_format *tx_frame)
{
    unsigned short send_len = 0;

    send_len = set_lora_tx_byte(send_len,0xEF);
    send_len = set_lora_tx_byte(send_len,(NETID_TEST_ENV << 4) | NETWORK_VERSION);
    send_len = set_lora_tx_byte(send_len,(tx_frame->msg_ack << 7) | (DEVICE_TYPE_DOORUNIT << 3) | tx_frame->msg_type);
    send_len = set_lora_tx_word(send_len,tx_frame->dest_addr);
    send_len = set_lora_tx_word(send_len,tx_frame->source_addr);
    send_len = set_lora_tx_byte(send_len,tx_frame->command);
    send_len = set_lora_tx_byte(send_len,tx_frame->tx_len);
    send_len = set_lora_tx_buffer(send_len,tx_frame->tx_data,tx_frame->tx_len);
    send_len = set_lora_tx_crc(send_len);
    lora_tx_enqueue(get_lora_tx_buf(),send_len,tx_frame->parameter);
}
