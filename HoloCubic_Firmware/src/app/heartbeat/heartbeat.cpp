#include "heartbeat.h"
#include "heartbeat_gui.h"
#include "sys/app_controller.h"
#include "common.h"
#include "network.h"

#define HEARTBEAT_APP_NAME "Heartbeat"

// 动态数据，APP的生命周期结束也需要释放它
struct HeartbeatAppRunData
{
    uint8_t send_cnt = 0;
    uint8_t recv_cnt = 0;
};

// // 常驻数据，可以不随APP的生命周期而释放或删除
// struct HeartbeatAppForeverData
// {
// };

// 保存APP运行时的参数信息，理论上关闭APP时推荐在 xxx_exit_callback 中释放掉
static HeartbeatAppRunData *run_data = NULL;

// 当然你也可以添加恒定在内存中的少量变量（退出时不用释放，实现第二次启动时可以读取）
// 考虑到所有的APP公用内存，尽量减少 forever_data 的数据占用
// static HeartbeatAppForeverData forever_data;

static int heartbeat_init(void)
{
    // 初始化运行时的参数
    heartbeat_gui_init();
    // 初始化运行时参数
    run_data = (HeartbeatAppRunData *)calloc(1, sizeof(HeartbeatAppRunData));
    run_data->send_cnt = 0;
    run_data->recv_cnt = 0;
}

static void heartbeat_process(AppController *sys,
                            const ImuAction *act_info)
{
    lv_scr_load_anim_t anim_type = LV_SCR_LOAD_ANIM_NONE;
    if (RETURN == act_info->active)
    {
        sys->app_exit(); // 退出APP
        return;
    }
    else if (GO_FORWORD == act_info->active) // 向前按发送一条消息
    {
        anim_type = LV_SCR_LOAD_ANIM_MOVE_TOP;
        run_data->send_cnt += 1;
        mqtt_client.publish(liz_mqtt_pubtopic, "hello!");
        Serial.printf("sent publish %s successful", liz_mqtt_pubtopic); 
        Serial.println();
        // 发送指示灯
        RgbParam rgb_setting = {LED_MODE_RGB,
                            0, 0, 0, 
                            112,0,0,
                            1, 1, 1,
                            0.15, 0.25,
                            0.001, 8};
        set_rgb(&rgb_setting);
    }
    if (run_data->recv_cnt > 0 && run_data->send_cnt > 0) 
    {
        heartbeat_set_sr_type(HEART);
    }
    else if (run_data->recv_cnt > 0)
    {
        heartbeat_set_sr_type(RECV);
    }
    else if (run_data->send_cnt == 0) // 进入app时自动发送mqtt消息
    {
        heartbeat_set_sr_type(SEND);
        run_data->send_cnt += 1;
        mqtt_client.publish(liz_mqtt_pubtopic, MQTT_SEND_MSG);
        Serial.printf("sent publish %s successful", liz_mqtt_pubtopic); 
    }
    // 发送请求。如果是wifi相关的消息，当请求完成后自动会调用 heartbeat_message_handle 函数
    // sys->send_to(HEARTBEAT_APP_NAME, CTRL_NAME,
    //              APP_MESSAGE_WIFI_CONN, (void *)run_data->val1, NULL);

    // 程序需要时可以适当加延时
    display_heartbeat("heartbeat", anim_type);
    heartbeat_set_send_recv_cnt_label(run_data->send_cnt, run_data->recv_cnt);
    display_heartbeat_img();
    delay(30);
}

static int heartbeat_exit_callback(void *param)
{
    // 释放资源
    heartbeat_gui_del();
    free(run_data);
    run_data = NULL;
}

static void heartbeat_message_handle(const char *from, const char *to,
                                   APP_MESSAGE_TYPE type, void *message,
                                   void *ext_info)
{
    // 目前主要是wifi开关类事件（用于功耗控制）
    switch (type)
    {
    case APP_MESSAGE_WIFI_CONN:
    {
        // todo
    }
    break;
    case APP_MESSAGE_WIFI_AP:
    {
        // todo
    }
    break;
    case APP_MESSAGE_WIFI_ALIVE:
    {
        // wifi心跳维持的响应 可以不做任何处理
    }
    break;
    case APP_MESSAGE_GET_PARAM:
    {
        char *param_key = (char *)message;
    }
    break;
    case APP_MESSAGE_SET_PARAM:
    {
        char *param_key = (char *)message;
        char *param_val = (char *)ext_info;
    }
    break;
    case APP_MESSAGE_MQTT_DATA:
    {
        if (run_data->send_cnt > 0)   //已经手动发送过了
        {
            heartbeat_set_sr_type(HEART); 
        }
        else 
        {
            heartbeat_set_sr_type(RECV);
        }
        /* 亮一下 */
        RgbParam rgb_setting = {LED_MODE_RGB,
                            0, 0, 0, 
                            3,36,86,
                            1, 1, 1,
                            0.15, 0.25,
                            0.001, 8};
        /*  cfg->mode;
        cfg->min_value_0;cfg->min_value_1;cfg->min_value_2;
        cfg->max_value_0;cfg->max_value_1;cfg->max_value_2;
        cfg->step_0;cfg->step_1;cfg->step_2;
        cfg->min_brightness;cfg->max_brightness;
        cfg->brightness_step;cfg->time; */
        set_rgb(&rgb_setting);
        run_data->recv_cnt++;
        Serial.println("received heartbeat");
    }
    break;
    default:
        break;
    }
}

APP_OBJ heartbeat_app = {HEARTBEAT_APP_NAME, &app_heartbeat, "Author HQ\nVersion 2.0.0\n",
                       heartbeat_init, heartbeat_process,
                       heartbeat_exit_callback, heartbeat_message_handle};
