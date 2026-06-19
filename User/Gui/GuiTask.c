//
// Created by Meng on 2026/6/12.
//

#include "GuiTask.h"

#include <string.h>
#include <stdio.h>

#include "cmsis_os2.h"
#include "Gui.h"
#include "xprintf.h"
#include "rtc.h"
#include "app_config.h"
#include "MyTask.h"
#include "Temperature.h"
#include "Humidity.h"
#include "cjhr31_table.h"
#include "control.h"
#include "buzz.h"

/*========= 布局常量 =========*/

#define RTC_TIME_X      13
#define RTC_TIME_Y      148
#define RTC_TIME_LEN    19

#define MAIN_SELECT_START       0
#define MAIN_SELECT_MODE        1
#define MAIN_SELECT_SET         2
#define MAIN_SELECT_NUM         3
#define MAIN_SELECT_COLOR       YELLOW

#define GUI_SCENE_MAIN          0
#define GUI_SCENE_MODE          1
#define GUI_SCENE_SETTING       2
#define GUI_SCENE_CURVE         3

#define MODE_SELECT_BACK        0
#define MODE_SELECT_EGG         1
#define MODE_SELECT_CHICKEN     2
#define MODE_SELECT_PRINTER     3
#define MODE_SELECT_BEAN        4
#define MODE_SELECT_BREAD       5
#define MODE_SELECT_NUM         6

#define SETTING_SELECT_TEMP_SUB     0
#define SETTING_SELECT_TEMP_ADD     1
#define SETTING_SELECT_HUM_SUB      2
#define SETTING_SELECT_HUM_ADD      3
#define SETTING_SELECT_TIMER        4
#define SETTING_SELECT_TEMP_CURVE   5
#define SETTING_SELECT_BACK         6
#define SETTING_SELECT_NUM          7
#define SETTING_SELECT_EDIT_COLOR   GREEN

#define SETTING_PAGE_MAIN           0
#define SETTING_PAGE_TIME           1

#define SETTING_TIME_SELECT_BACK    0
#define SETTING_TIME_SELECT_TIME    1
#define SETTING_TIME_SELECT_NUM     2

#define TIME_EDIT_YEAR_THOUSAND     0
#define TIME_EDIT_YEAR_HUNDRED      1
#define TIME_EDIT_YEAR_TEN          2
#define TIME_EDIT_YEAR_ONE          3
#define TIME_EDIT_MONTH_TEN         4
#define TIME_EDIT_MONTH_ONE         5
#define TIME_EDIT_DAY_TEN           6
#define TIME_EDIT_DAY_ONE           7
#define TIME_EDIT_HOUR_TEN          8
#define TIME_EDIT_HOUR_ONE          9
#define TIME_EDIT_MIN_TEN           10
#define TIME_EDIT_MIN_ONE           11
#define TIME_EDIT_SEC_TEN           12
#define TIME_EDIT_SEC_ONE           13
#define TIME_EDIT_NUM               14

/*========= 选择框数据表 =========*/

/* 通用选择框描述 */
typedef struct {
    u8 x, y, w, h;
} SelectBoxItem;

/* Main 场景选择框位置 */
static const SelectBoxItem MAIN_BOXES[] = {
    [MAIN_SELECT_START] = {  7, 114, 52, 28 },
    [MAIN_SELECT_MODE]  = { 69, 114, 52, 28 },
    [MAIN_SELECT_SET]   = {111, 146, 16, 14 },
};

/* Mode 场景选择框位置 */
static const SelectBoxItem MODE_BOXES[] = {
    [MODE_SELECT_BACK]    = {111,   0, 16, 14 },
    [MODE_SELECT_EGG]     = {  1,  16,126, 26 },
    [MODE_SELECT_CHICKEN] = {  1,  40,126, 26 },
    [MODE_SELECT_PRINTER] = {  1,  64,126, 26 },
    [MODE_SELECT_BEAN]    = {  1,  88,126, 26 },
    [MODE_SELECT_BREAD]   = {  1, 112,126, 26 },
};

/* Curve 场景选择框项 */
#define CURVE_SELECT_BACK       0
#define CURVE_SELECT_TEMP       1
#define CURVE_SELECT_HUM        2
#define CURVE_SELECT_NUM        3

/* Curve 场景选择框位置 */
static const SelectBoxItem CURVE_BOXES[] = {
    [CURVE_SELECT_BACK] = {  0,  2, 12, 12 },
    [CURVE_SELECT_TEMP] = { 18,  0, 26, 18 },
    [CURVE_SELECT_HUM]  = { 52,  0, 26, 18 },
};

/* Setting 场景选择框位置 */
static const SelectBoxItem SETTING_BOXES[] = {
    [SETTING_SELECT_TEMP_SUB]    = { 15,  52, 12, 12 },
    [SETTING_SELECT_TEMP_ADD]    = { 37,  52, 12, 12 },
    [SETTING_SELECT_HUM_SUB]     = { 79,  52, 12, 12 },
    [SETTING_SELECT_HUM_ADD]     = {101,  52, 12, 12 },
    [SETTING_SELECT_TIMER]       = {  1, 119,126, 16 },
    [SETTING_SELECT_TEMP_CURVE]  = {  1, 134,126, 25 },
    [SETTING_SELECT_BACK]        = {113,   0, 16, 14 },
};

/* Setting 第二页选择框位置 */
static const SelectBoxItem SETTING_TIME_BOXES[] = {
    [SETTING_TIME_SELECT_BACK] = {113,  0, 16, 14 },
    [SETTING_TIME_SELECT_TIME] = {  1, 48,126, 28 },
};

/* Mode 场景中每个模式的静态显示信息 */
typedef struct {
    const char *image;      /* 24x24 图标路径 */
    const char *text;       /* 模式名称（中文） */
    u8 image_y;             /* 图标 Y 坐标 */
    u8 text_x;              /* 文字 X 坐标 */
    u8 text_y;              /* 文字 Y 坐标 */
} ModeItemInfo;

static const ModeItemInfo MODE_ITEMS[] = {
    [MODE_SELECT_EGG]     = { "/images/egg_24x24.bin",     "孵蛋模式",      17, 37, 24 },
    [MODE_SELECT_CHICKEN] = { "/images/chicken_24x24.bin", "小鸡保温箱模式",  41, 35, 48 },
    [MODE_SELECT_PRINTER] = { "/images/printer_24x24.bin", "耗材烘干模式",    65, 35, 72 },
    [MODE_SELECT_BEAN]    = { "/images/bean_24x24.bin",    "纳豆模式",       89, 37, 96 },
    [MODE_SELECT_BREAD]   = { "/images/bread_24x24.bin",   "面粉发酵模式",  113, 35,120 },
};

/* Setting 场景温湿度变量描述 — TEMP_SUB/TEMP_ADD→索引0, HUM_SUB/HUM_ADD→索引1 */
typedef struct {
    int16_t *var;       /* 指向 setting_temp_x10 或 setting_hum_x10 */
    u8       disp_x;    /* 数值显示的 X 坐标 */
    u16      color;     /* 显示颜色 */
} SettingVarInfo;

/* 定时器编辑状态 */
#define TIMER_EDIT_DAY_HIGH     0   /* 天十位 */
#define TIMER_EDIT_DAY_LOW      1   /* 天个位 */
#define TIMER_EDIT_HOUR_HIGH    2   /* 时十位 */
#define TIMER_EDIT_HOUR_LOW     3   /* 时个位 */
#define TIMER_EDIT_MIN_HIGH     4   /* 分十位 */
#define TIMER_EDIT_MIN_LOW      5   /* 分个位 */
#define TIMER_EDIT_SEC_HIGH     6   /* 秒十位 */
#define TIMER_EDIT_SEC_LOW      7   /* 秒个位 */
#define TIMER_EDIT_NUM          8

/* 定时器状态 */
#define TIMER_STATE_IDLE        0   /* 未设置 */
#define TIMER_STATE_SET         1   /* 已设置等待启动（编辑中） */
#define TIMER_STATE_RUNNING     2   /* 正在倒计时 */
#define TIMER_STATE_TIMEOUT     3   /* 定时已到 */

/* 通用选择框导航宏 — 消除 SelectNext/SelectPrev 重复代码 */
#define DEFINE_SELECT_NEXT_PREV(Scene, Var, First, Count)                                     \
static void scene_##Scene##_SelectNext(void)                                                  \
{                                                                                             \
    scene_##Scene##_DrawSelectBox(Var, BLACK);                                                \
    Var++;                                                                                    \
    if (Var >= Count) Var = First;                                                            \
    scene_##Scene##_DrawSelectBox(Var, MAIN_SELECT_COLOR);                                    \
}                                                                                             \
static void scene_##Scene##_SelectPrev(void)                                                  \
{                                                                                             \
    scene_##Scene##_DrawSelectBox(Var, BLACK);                                                \
    if (Var == First) Var = Count - 1;                                                        \
    else Var--;                                                                               \
    scene_##Scene##_DrawSelectBox(Var, MAIN_SELECT_COLOR);                                    \
}

/*========= 本地变量 =========*/

/* 曲线场景前向声明 (被 scene_Setting_HandleInput 调用) */
static void scene_Curve_Static(void);
static void scene_Curve_Dynamic(void);
static void scene_Curve_HandleInput(uint32_t flags);
static void scene_SettingTime_Static(void);
static void scene_SettingTime_Dynamic(void);
static void scene_SettingTime_HandleInput(uint32_t flags);

/* 传感器读数 (x10) — 供界面显示和曲线采样 */
static int16_t g_cur_temp_x10 = 0;
static int16_t g_cur_hum_x10  = 0;

/* DS18B20 两步法测温状态 */
static uint8_t g_temp_pending = 0;
static uint8_t g_temp_valid = 0;

static u8 main_select_index = MAIN_SELECT_START;
static u8 mode_select_index = MODE_SELECT_BACK;
static u8 setting_page = SETTING_PAGE_MAIN;
static u8 setting_select_index = SETTING_SELECT_TEMP_SUB;
static u8 setting_time_select_index = SETTING_TIME_SELECT_BACK;
static u8 setting_editing = 0;
static int16_t setting_temp_x10 = 375;
static int16_t setting_hum_x10 = 900;
static u8 main_is_running = 0;
static u8 gui_current_scene = GUI_SCENE_MAIN;

/* 设置暂存区 — 退出设置时保存, 按启动时再写入 control 模块 */
static int16_t staging_temp_x10 = 375;  /* 37.5°C */
static int16_t staging_hum_x10  = 650;  /* 65.0%RH */
static uint32_t staging_timer_sec = 0;  /* 定时秒数 (0=未设置) */

/* 定时器相关变量 */
static u8 timer_days    = 0;    /* 天 */
static u8 timer_hours   = 0;    /* 时 (0-23) */
static u8 timer_mins    = 0;    /* 分 (0-59) */
static u8 timer_secs    = 10;   /* 秒 (0-59)，默认10秒方便调试 */
static u8 timer_state   = TIMER_STATE_IDLE;
static u8 timer_edit_digit = 0; /* 当前编辑的数字位 (0-7) */
static uint32_t timer_target_sec = 0;   /* 倒计时目标绝对秒数(相对启动时刻) */
static uint32_t timer_start_sec = 0;    /* 定时器启动时的已运行秒数 */

/* 定时器编辑状态标志 */
static u8 setting_timer_editing = 0;

/* 系统时间编辑状态 */
static u8 setting_time_editing = 0;
static u8 time_edit_digit = 0;
static uint16_t edit_year = 2026;
static u8 edit_month = 1;
static u8 edit_day = 1;
static u8 edit_hour = 0;
static u8 edit_min = 0;
static u8 edit_sec = 0;

/* 时间数字编辑数组 — 14 个独立的数字位 */
static u8 time_digits[TIME_EDIT_NUM];

/* 定时器超时全局标志 — 其他任务可轮询 */
volatile uint8_t g_timer_timeout = 0;

/*========= 曲线场景常量 & 数据缓冲区 =========*/

#define CURVE_PLOT_LEFT     18
#define CURVE_PLOT_TOP      20
#define CURVE_PLOT_W        108
#define CURVE_PLOT_H        120
#define CURVE_MODE_TEMP     0
#define CURVE_MODE_HUM      1
#define CURVE_MAX_POINTS    600

/* 曲线数据 (线性缓冲，满则前移丢弃旧数据) */
static int16_t curve_temp_buf[CURVE_MAX_POINTS];
static int16_t curve_hum_buf[CURVE_MAX_POINTS];
static uint16_t curve_buf_count = 0;
static uint8_t  curve_mode = CURVE_MODE_TEMP;

/* 上一帧每列的像素 Y 位置 — 用于擦除旧数据线 (0xFFFF = 无效/未初始化) */
static uint16_t curve_prev_py[CURVE_PLOT_W];

/* 曲线场景当前选择索引 */
static u8 curve_select_index = CURVE_SELECT_BACK;

/* Setting 场景温湿度变量数据表 */
static const SettingVarInfo SETTING_VARS[] = {
    { &setting_temp_x10,  30, RED  },
    { &setting_hum_x10,  105, BLUE },
};

/*========= 本地函数 =========*/

static void rtc_make_string(char *buf, const calendar_obj *t)
{
    u8 year = t->year % 100;

    buf[0]  = '0' + year / 10;
    buf[1]  = '0' + year % 10;
    buf[2]  = '-';
    buf[3]  = '0' + t->month / 10;
    buf[4]  = '0' + t->month % 10;
    buf[5]  = '-';
    buf[6]  = '0' + t->date / 10;
    buf[7]  = '0' + t->date % 10;
    buf[8]  = ' ';
    buf[9]  = '0' + t->week;
    buf[10] = ' ';
    buf[11] = '0' + t->hour / 10;
    buf[12] = '0' + t->hour % 10;
    buf[13] = ':';
    buf[14] = '0' + t->min / 10;
    buf[15] = '0' + t->min % 10;
    buf[16] = ':';
    buf[17] = '0' + t->sec / 10;
    buf[18] = '0' + t->sec % 10;
    buf[19] = '\0';
}

/* 通用：用数据表画选择框 */
static void DrawBoxByTable(u8 index, const SelectBoxItem *table, u8 table_size, u16 color)
{
    if (index >= table_size) return;
    u16 x = table[index].x;
    u16 y = table[index].y;
    u16 w = table[index].w;
    u16 h = table[index].h;
    Gui_DrawLine(x, y, x + w, y, color);
    Gui_DrawLine(x + w, y, x + w, y + h, color);
    Gui_DrawLine(x, y + h, x + w, y + h, color);
    Gui_DrawLine(x, y, x, y + h, color);
}

static void scene_Main_DrawSelectBox(u8 index, u16 color)
{
    DrawBoxByTable(index, MAIN_BOXES, MAIN_SELECT_NUM, color);
}

static void scene_Main_DrawStartButton(void)
{
    if (main_is_running)
    {
        Gui_showimage_48x24("/images/stop_button_48x24.bin",9 ,116);
    }
    else
    {
        Gui_showimage_48x24("/images/start_button_48x24.bin",9 ,116);
    }
}

DEFINE_SELECT_NEXT_PREV(Main, main_select_index, MAIN_SELECT_START, MAIN_SELECT_NUM)

static void scene_Main_HandleInput(uint32_t flags)
{
    if ((flags & GUI_FLAG_EC11_CW) != 0U)
    {
        scene_Main_SelectNext();
    }

    if ((flags & GUI_FLAG_EC11_CCW) != 0U)
    {
        scene_Main_SelectPrev();
    }

    if ((flags & GUI_FLAG_EC11_PRESS) != 0U)
    {
        switch (main_select_index)
        {
            case MAIN_SELECT_START:
                if (main_is_running)
                {
                    /* 停止 — 关闭继电器, 清除定时器 */
                    control_heater_set(0);
                    control_fan_set(0);
                    timer_state = TIMER_STATE_IDLE;
                    main_is_running = 0;
                }
                else
                {
                    /* 启动 — 暂存区设置生效 */
                    control_set_targets(staging_temp_x10, staging_hum_x10);
                    if (staging_timer_sec > 0)
                    {
                        timer_state = TIMER_STATE_RUNNING;
                        timer_start_sec = g_elapsed_sec;
                        timer_target_sec = g_elapsed_sec + staging_timer_sec;
                        g_timer_timeout = 0;
                    }
                    main_is_running = 1;
                }
                scene_Main_DrawStartButton();
                scene_Main_DrawSelectBox(main_select_index, MAIN_SELECT_COLOR);
                break;

            case MAIN_SELECT_MODE:
                gui_current_scene = GUI_SCENE_MODE;
                mode_select_index = MODE_SELECT_BACK;
                Lcd_Clear(BLACK);
                scene_Mode_Static();
                scene_Mode_Dynamic();
                break;

            case MAIN_SELECT_SET:
                gui_current_scene = GUI_SCENE_SETTING;
                setting_page = SETTING_PAGE_MAIN;
                setting_select_index = SETTING_SELECT_TEMP_SUB;
                setting_editing = 0;
                setting_timer_editing = 0;
                setting_time_editing = 0;
                Lcd_Clear(BLACK);
                scene_Setting_Static();
                scene_Setting_Dynamic();
                break;

            default:
                break;
        }
    }
}

static void scene_Mode_DrawSelectBox(u8 index, u16 color)
{
    DrawBoxByTable(index, MODE_BOXES, MODE_SELECT_NUM, color);
}

DEFINE_SELECT_NEXT_PREV(Mode, mode_select_index, MODE_SELECT_BACK, MODE_SELECT_NUM)

static void scene_Mode_HandleInput(uint32_t flags)
{
    if ((flags & GUI_FLAG_EC11_CW) != 0U)
    {
        scene_Mode_SelectNext();
    }

    if ((flags & GUI_FLAG_EC11_CCW) != 0U)
    {
        scene_Mode_SelectPrev();
    }

    if ((flags & GUI_FLAG_EC11_PRESS) != 0U)
    {
        if (mode_select_index == MODE_SELECT_BACK)
        {
            gui_current_scene = GUI_SCENE_MAIN;
            Lcd_Clear(BLACK);
            scene_Main_Static();
            scene_Main_Dynamic();
        }
    }
}

/*========= 设置场景 - 定时器功能 =========*/

static void scene_Setting_TimerShowValue(void)
{
    u8 buf[32] = "";

    if (timer_state == TIMER_STATE_TIMEOUT)
    {
        xsprintf((char*)buf, "  TIME UP!   ");
    }
    else if (timer_state == TIMER_STATE_RUNNING)
    {
        uint32_t remaining = (timer_target_sec > g_elapsed_sec) ?
                              (timer_target_sec - g_elapsed_sec) : 0;
        u16 d = remaining / 86400;
        u16 h = (remaining % 86400) / 3600;
        u16 m = (remaining % 3600) / 60;
        u16 s = remaining % 60;
        xsprintf((char*)buf, "%02d:%02d:%02d:%02d", d, h, m, s);
    }
    else
    {
        xsprintf((char*)buf, "%02d:%02d:%02d:%02d",
                 timer_days, timer_hours, timer_mins, timer_secs);
    }

    Gui_ShowString(55, 121, buf, WHITE, BLACK, 12, 0);
}

static void scene_Setting_TimerDrawCursor(u16 color)
{
    /* 对应 %02d:%02d:%02d:%02d 每个数字位的 x 起始位置
     * Gui_ShowString 每字符固定推进 sizey/2 = 6px，含冒号 */
    u8 edit_x[] = {55, 61, 73, 79, 91, 97, 109, 115};
    u8 pos = timer_edit_digit;
    if (pos >= TIMER_EDIT_NUM) return;
    u8 x = edit_x[pos];
    u8 y = 129; /* 在数字下方一行画下划线 */
    Gui_DrawLine(x, y, x + 5, y, color);
}

/* 定时器数字位信息表 */
typedef struct {
    u8 *var;
    u8  max_val;
} TimerDigitInfo;

static const TimerDigitInfo TIMER_DIGITS[TIMER_EDIT_NUM] = {
    [TIMER_EDIT_DAY_HIGH]  = { &timer_days,  9 },
    [TIMER_EDIT_DAY_LOW]   = { &timer_days,  9 },
    [TIMER_EDIT_HOUR_HIGH] = { &timer_hours, 2 },
    [TIMER_EDIT_HOUR_LOW]  = { &timer_hours, 9 },
    [TIMER_EDIT_MIN_HIGH]  = { &timer_mins,  5 },
    [TIMER_EDIT_MIN_LOW]   = { &timer_mins,  9 },
    [TIMER_EDIT_SEC_HIGH]  = { &timer_secs,  5 },
    [TIMER_EDIT_SEC_LOW]   = { &timer_secs,  9 },
};

static void scene_Setting_TimerAdjustDigit(s8 add)
{
    u8 *p;
    u8 max_val;

    if (timer_edit_digit >= TIMER_EDIT_NUM) return;
    p = TIMER_DIGITS[timer_edit_digit].var;
    max_val = TIMER_DIGITS[timer_edit_digit].max_val;

    u8 digit;
    if ((timer_edit_digit % 2) == 0) /* 高位 (十位) */
        digit = *p / 10;
    else /* 低位 (个位) */
        digit = *p % 10;

    s16 new_digit = (s16)digit + add;
    if (new_digit < 0) new_digit = max_val;
    if (new_digit > max_val) new_digit = 0;

    if ((timer_edit_digit % 2) == 0)
        *p = (u8)(*p % 10 + new_digit * 10);
    else
        *p = (u8)(*p / 10 * 10 + new_digit);

    /* 校验范围 */
    if (timer_hours > 23) timer_hours = 23;
    if (timer_mins > 59)  timer_mins = 59;
    if (timer_secs > 59)  timer_secs = 59;

    scene_Setting_TimerShowValue();
}

static void scene_Setting_TimerEditNext(void)
{
    scene_Setting_TimerDrawCursor(BLACK);
    timer_edit_digit++;
    if (timer_edit_digit >= TIMER_EDIT_NUM)
        timer_edit_digit = 0;
    scene_Setting_TimerDrawCursor(WHITE);
}

static void scene_Setting_TimerEditPrev(void)
{
    scene_Setting_TimerDrawCursor(BLACK);
    if (timer_edit_digit == 0)
        timer_edit_digit = TIMER_EDIT_NUM - 1;
    else
        timer_edit_digit--;
    scene_Setting_TimerDrawCursor(WHITE);
}

static void scene_Setting_ShowValue(u8 index)
{
    const SettingVarInfo *v = &SETTING_VARS[index >> 1];
    print_string_gui(v->disp_x, 30, v->color, BLACK, "%d.%d",
                     *v->var / 10, *v->var % 10);
}

static void scene_Setting_DrawSelectBox(u8 index, u16 color)
{
    DrawBoxByTable(index, SETTING_BOXES, SETTING_SELECT_NUM, color);
}

static void scene_SettingTime_DrawSelectBox(u8 index, u16 color)
{
    DrawBoxByTable(index, SETTING_TIME_BOXES, SETTING_TIME_SELECT_NUM, color);
}

static void scene_Setting_GotoPage(u8 page)
{
    setting_page = page;
    Lcd_Clear(BLACK);
    if (setting_page == SETTING_PAGE_TIME)
    {
        setting_time_select_index = SETTING_TIME_SELECT_TIME;
        scene_SettingTime_Static();
        scene_SettingTime_Dynamic();
    }
    else
    {
        setting_select_index = SETTING_SELECT_TEMP_SUB;
        scene_Setting_Static();
        scene_Setting_Dynamic();
    }
}

static void scene_Setting_SelectNext(void)
{
    scene_Setting_DrawSelectBox(setting_select_index, BLACK);
    if (setting_select_index == SETTING_SELECT_TEMP_CURVE)
    {
        scene_Setting_GotoPage(SETTING_PAGE_TIME);
        return;
    }
    setting_select_index++;
    if (setting_select_index >= SETTING_SELECT_NUM) setting_select_index = SETTING_SELECT_TEMP_SUB;
    scene_Setting_DrawSelectBox(setting_select_index, MAIN_SELECT_COLOR);
}

static void scene_Setting_SelectPrev(void)
{
    scene_Setting_DrawSelectBox(setting_select_index, BLACK);
    if (setting_select_index == SETTING_SELECT_TEMP_SUB) setting_select_index = SETTING_SELECT_NUM - 1;
    else setting_select_index--;
    scene_Setting_DrawSelectBox(setting_select_index, MAIN_SELECT_COLOR);
}

static void scene_SettingTime_SelectNext(void)
{
    scene_SettingTime_DrawSelectBox(setting_time_select_index, BLACK);
    setting_time_select_index++;
    if (setting_time_select_index >= SETTING_TIME_SELECT_NUM) setting_time_select_index = SETTING_TIME_SELECT_BACK;
    scene_SettingTime_DrawSelectBox(setting_time_select_index, MAIN_SELECT_COLOR);
}

static void scene_SettingTime_SelectPrev(void)
{
    scene_SettingTime_DrawSelectBox(setting_time_select_index, BLACK);
    if (setting_time_select_index == SETTING_TIME_SELECT_BACK)
    {
        setting_page = SETTING_PAGE_MAIN;
        setting_select_index = SETTING_SELECT_TEMP_CURVE;
        Lcd_Clear(BLACK);
        scene_Setting_Static();
        scene_Setting_Dynamic();
        return;
    }
    setting_time_select_index--;
    scene_SettingTime_DrawSelectBox(setting_time_select_index, MAIN_SELECT_COLOR);
}

static void scene_Curve_DrawSelectBox(u8 index, u16 color)
{
    DrawBoxByTable(index, CURVE_BOXES, CURVE_SELECT_NUM, color);
}

DEFINE_SELECT_NEXT_PREV(Curve, curve_select_index, CURVE_SELECT_BACK, CURVE_SELECT_NUM)

static void scene_Setting_AdjustValue(u8 add)
{
    int16_t step = add ? 1 : -1;
    const SettingVarInfo *v = &SETTING_VARS[setting_select_index >> 1];

    *v->var += step;
    if (*v->var < 0) *v->var = 0;

    scene_Setting_ShowValue(setting_select_index);
    scene_Setting_DrawSelectBox(setting_select_index, SETTING_SELECT_EDIT_COLOR);
}

static u8 scene_Time_IsLeapYear(uint16_t year)
{
    return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

static u8 scene_Time_GetMonthDays(uint16_t year, u8 month)
{
    static const u8 days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (month < 1) month = 1;
    if (month > 12) month = 12;
    if (month == 2 && scene_Time_IsLeapYear(year)) return 29;
    return days[month - 1];
}

static void scene_SettingTime_LimitValue(void)
{
    u8 max_day;

    if (edit_year < 1970) edit_year = 1970;
    if (edit_year > 2105) edit_year = 2105;
    if (edit_month < 1) edit_month = 1;
    if (edit_month > 12) edit_month = 12;

    max_day = scene_Time_GetMonthDays(edit_year, edit_month);
    if (edit_day < 1) edit_day = 1;
    if (edit_day > max_day) edit_day = max_day;
    if (edit_hour > 23) edit_hour = 23;
    if (edit_min > 59) edit_min = 59;
    if (edit_sec > 59) edit_sec = 59;
}

static void scene_SettingTime_LoadCurrent(void)
{
    edit_year = calendar.year;
    edit_month = calendar.month;
    edit_day = calendar.date;
    edit_hour = calendar.hour;
    edit_min = calendar.min;
    edit_sec = calendar.sec;
    scene_SettingTime_LimitValue();
}

static void scene_SettingTime_ShowValue(void)
{
    u8 buf[24] = "";

    xsprintf((char*)buf, "%04d-%02d-%02d", edit_year, edit_month, edit_day);
    Gui_ShowString(14, 56, buf, WHITE, BLACK, 12, 0);
    memset(buf, 0, sizeof(buf));
    xsprintf((char*)buf, "%02d:%02d:%02d", edit_hour, edit_min, edit_sec);
    Gui_ShowString(38, 68, buf, WHITE, BLACK, 12, 0);
}

static void scene_SettingTime_DrawCursor(u16 color)
{
    static const u8 x[] = {14, 20, 26, 32, 44, 50, 62, 68, 38, 44, 56, 62, 74, 80};
    static const u8 y[] = {64, 64, 64, 64, 64, 64, 64, 64, 76, 76, 76, 76, 76, 76};

    if (time_edit_digit >= TIME_EDIT_NUM) return;
    Gui_DrawLine(x[time_edit_digit], y[time_edit_digit], x[time_edit_digit] + 5, y[time_edit_digit], color);
}

typedef struct {
    uint16_t *var16;
    u8       *var8;
    uint16_t  weight;
    u8        max_digit;
} TimeDigitInfo;

static const TimeDigitInfo TIME_DIGITS[TIME_EDIT_NUM] = {
    [TIME_EDIT_YEAR_THOUSAND] = { &edit_year, 0, 1000, 2 },
    [TIME_EDIT_YEAR_HUNDRED]  = { &edit_year, 0,  100, 9 },
    [TIME_EDIT_YEAR_TEN]      = { &edit_year, 0,   10, 9 },
    [TIME_EDIT_YEAR_ONE]      = { &edit_year, 0,    1, 9 },
    [TIME_EDIT_MONTH_TEN]     = { 0, &edit_month,  10, 1 },
    [TIME_EDIT_MONTH_ONE]     = { 0, &edit_month,   1, 9 },
    [TIME_EDIT_DAY_TEN]       = { 0, &edit_day,    10, 3 },
    [TIME_EDIT_DAY_ONE]       = { 0, &edit_day,     1, 9 },
    [TIME_EDIT_HOUR_TEN]      = { 0, &edit_hour,   10, 2 },
    [TIME_EDIT_HOUR_ONE]      = { 0, &edit_hour,    1, 9 },
    [TIME_EDIT_MIN_TEN]       = { 0, &edit_min,    10, 5 },
    [TIME_EDIT_MIN_ONE]       = { 0, &edit_min,     1, 9 },
    [TIME_EDIT_SEC_TEN]       = { 0, &edit_sec,    10, 5 },
    [TIME_EDIT_SEC_ONE]       = { 0, &edit_sec,     1, 9 },
};

static void scene_SettingTime_AdjustDigit(s8 add)
{
    const TimeDigitInfo *d;
    uint16_t value;
    uint16_t digit;
    s16 new_digit;

    if (time_edit_digit >= TIME_EDIT_NUM) return;
    d = &TIME_DIGITS[time_edit_digit];
    value = d->var16 ? *d->var16 : *d->var8;
    digit = (value / d->weight) % 10;

    new_digit = (s16)digit + add;
    if (new_digit < 0) new_digit = d->max_digit;
    if (new_digit > d->max_digit) new_digit = 0;

    value = value - digit * d->weight + (uint16_t)new_digit * d->weight;
    if (d->var16) *d->var16 = value;
    else *d->var8 = (u8)value;

    scene_SettingTime_LimitValue();
    scene_SettingTime_ShowValue();
    scene_SettingTime_DrawCursor(WHITE);
}

static void scene_SettingTime_EditNext(void)
{
    scene_SettingTime_DrawCursor(BLACK);
    time_edit_digit++;
    if (time_edit_digit >= TIME_EDIT_NUM) time_edit_digit = 0;
    scene_SettingTime_DrawCursor(WHITE);
}

static void scene_SettingTime_Apply(void)
{
    scene_SettingTime_LimitValue();
    rtc_set_time(edit_year, edit_month, edit_day, edit_hour, edit_min, edit_sec);
    rtc_get_time();
    scene_SettingTime_ShowValue();
}

static void scene_SettingTime_HandleInput(uint32_t flags)
{
    if (setting_time_editing)
    {
        if ((flags & GUI_FLAG_EC11_CW) != 0U)
        {
            scene_SettingTime_AdjustDigit(1);
        }

        if ((flags & GUI_FLAG_EC11_CCW) != 0U)
        {
            scene_SettingTime_AdjustDigit(-1);
        }

        if ((flags & GUI_FLAG_EC11_PRESS) != 0U)
        {
            if (time_edit_digit >= TIME_EDIT_NUM - 1)
            {
                scene_SettingTime_DrawCursor(BLACK);
                setting_time_editing = 0;
                scene_SettingTime_Apply();
                scene_SettingTime_DrawSelectBox(setting_time_select_index, MAIN_SELECT_COLOR);
            }
            else
            {
                scene_SettingTime_EditNext();
            }
        }
        return;
    }

    if ((flags & GUI_FLAG_EC11_CW) != 0U)
    {
        scene_SettingTime_SelectNext();
    }

    if ((flags & GUI_FLAG_EC11_CCW) != 0U)
    {
        scene_SettingTime_SelectPrev();
    }

    if ((flags & GUI_FLAG_EC11_PRESS) != 0U)
    {
        switch (setting_time_select_index)
        {
            case SETTING_TIME_SELECT_BACK:
                scene_Setting_GotoPage(SETTING_PAGE_MAIN);
                break;

            case SETTING_TIME_SELECT_TIME:
                scene_SettingTime_LoadCurrent();
                scene_SettingTime_ShowValue();
                setting_time_editing = 1;
                time_edit_digit = 0;
                scene_SettingTime_DrawSelectBox(setting_time_select_index, SETTING_SELECT_EDIT_COLOR);
                scene_SettingTime_DrawCursor(WHITE);
                break;

            default:
                break;
        }
    }
}

static void scene_Setting_HandleInput(uint32_t flags)
{
    if (setting_page == SETTING_PAGE_TIME)
    {
        scene_SettingTime_HandleInput(flags);
        return;
    }

    /* ===== 定时器数字编辑模式 ===== */
    if (setting_timer_editing)
    {
        if ((flags & GUI_FLAG_EC11_CW) != 0U)
        {
            scene_Setting_TimerAdjustDigit(1);
        }

        if ((flags & GUI_FLAG_EC11_CCW) != 0U)
        {
            scene_Setting_TimerAdjustDigit(-1);
        }

        if ((flags & GUI_FLAG_EC11_PRESS) != 0U)
        {
            if (timer_edit_digit >= TIMER_EDIT_NUM - 1)
            {
                /* 最后一位 → 确认, 保存到暂存区, 等主屏启动才生效 */
                scene_Setting_TimerDrawCursor(BLACK);
                setting_timer_editing = 0;

                staging_timer_sec = (uint32_t)timer_days * 86400UL +
                                    (uint32_t)timer_hours * 3600UL +
                                    (uint32_t)timer_mins * 60UL +
                                    (uint32_t)timer_secs;

                timer_state = (staging_timer_sec > 0) ? TIMER_STATE_SET : TIMER_STATE_IDLE;
                g_timer_timeout = 0;
                scene_Setting_TimerShowValue();
                scene_Setting_DrawSelectBox(setting_select_index, MAIN_SELECT_COLOR);
            }
            else
            {
                scene_Setting_TimerEditNext();
            }
        }
        return;
    }

    /* ===== 温湿度微调模式 ===== */
    if (setting_editing)
    {
        if ((flags & GUI_FLAG_EC11_CW) != 0U)
        {
            scene_Setting_AdjustValue(1);
        }

        if ((flags & GUI_FLAG_EC11_CCW) != 0U)
        {
            scene_Setting_AdjustValue(0);
        }

        if ((flags & GUI_FLAG_EC11_PRESS) != 0U)
        {
            setting_editing = 0;
            scene_Setting_DrawSelectBox(setting_select_index, MAIN_SELECT_COLOR);
        }

        return;
    }

    /* ===== 导航模式 ===== */
    if ((flags & GUI_FLAG_EC11_CW) != 0U)
    {
        scene_Setting_SelectNext();
    }

    if ((flags & GUI_FLAG_EC11_CCW) != 0U)
    {
        scene_Setting_SelectPrev();
    }

    if ((flags & GUI_FLAG_EC11_PRESS) != 0U)
    {
        switch (setting_select_index)
        {
            case SETTING_SELECT_TEMP_SUB:
            case SETTING_SELECT_TEMP_ADD:
            case SETTING_SELECT_HUM_SUB:
            case SETTING_SELECT_HUM_ADD:
                /* 数据表统一处理温湿度 ±10 调整 */
                {
                    const SettingVarInfo *v = &SETTING_VARS[setting_select_index >> 1];
                    if ((setting_select_index & 1) == 0) {  /* SUB: -10 */
                        if (*v->var > 0) *v->var -= 10;
                    } else {  /* ADD: +10 */
                        *v->var += 10;
                    }
                }
                scene_Setting_ShowValue(setting_select_index);
                setting_editing = 1;
                scene_Setting_DrawSelectBox(setting_select_index, SETTING_SELECT_EDIT_COLOR);
                break;

            case SETTING_SELECT_TIMER:
                /* 进入定时器数字编辑模式 */
                setting_timer_editing = 1;
                timer_edit_digit = 0;
                timer_state = TIMER_STATE_SET;

                /* 如果之前超时了，重置编辑值 */
                if (g_timer_timeout)
                {
                    timer_days = 0;
                    timer_hours = 0;
                    timer_mins = 0;
                    timer_secs = 0;
                    g_timer_timeout = 0;
                    scene_Setting_TimerShowValue();
                }

                scene_Setting_DrawSelectBox(setting_select_index, SETTING_SELECT_EDIT_COLOR);
                scene_Setting_TimerDrawCursor(WHITE);
                break;

            case SETTING_SELECT_TEMP_CURVE:
                /* 进入曲线界面 — 清空旧数据重新开始 */
                gui_current_scene = GUI_SCENE_CURVE;
                setting_editing = 0;
                setting_timer_editing = 0;
                curve_buf_count = 0;
                Lcd_Clear(BLACK);
                scene_Curve_Static();
                /* 等待按键弹跳稳定后清除残留标志 */
                osDelay(50);
                osThreadFlagsWait(GUI_FLAG_EC11_PRESS, osFlagsWaitAny, 0);
                break;

            case SETTING_SELECT_BACK:
                /* 返回主界面 — 保存到暂存区, 等启动时才生效 */
                staging_temp_x10 = setting_temp_x10;
                staging_hum_x10  = setting_hum_x10;
                staging_timer_sec = (uint32_t)timer_days * 86400UL +
                                    (uint32_t)timer_hours * 3600UL +
                                    (uint32_t)timer_mins * 60UL +
                                    (uint32_t)timer_secs;
                gui_current_scene = GUI_SCENE_MAIN;
                setting_page = SETTING_PAGE_MAIN;
                setting_editing = 0;
                setting_timer_editing = 0;
                setting_time_editing = 0;
                Lcd_Clear(BLACK);
                scene_Main_Static();
                scene_Main_Dynamic();
                break;

            default:
                break;
        }
    }
}

/*========= 场景函数 =========*/

void scene_Main_Static(void)
{
    u8 ui_data[20]="";

    // 左上角显示当前时间 HH:MM
    xsprintf((char*)ui_data, "%02d:%02d", calendar.hour, calendar.min);
    Gui_ShowString(0, 1, ui_data, WHITE, BLACK, 12, 0);

    Gui_showimage_12x12("/images/hot_12x12.bin", 60,0);
    Gui_showimage_12x12("/images/fan_12x12.bin",78 ,0);
    Gui_showimage_12x12("/images/hum_12x12.bin",93 ,0);

    Gui_showimage_24x24("/images/egg_24x24.bin",18 ,15);

    memset(ui_data,0,sizeof(ui_data));
    xsprintf(ui_data,"孵蛋模式");
    Gui_ShowChinese(48,21,ui_data,YELLOW,BLACK,12,0);

    Gui_showimage_24x24("/images/temp_24x24.bin",3 ,39);
    Gui_DrawLine(64,43,64,58,WHITE);

    Gui_showimage_24x24("/images/hum_24x24.bin",75 ,39);

    Gui_DrawLine(6,63,124,63,WHITE);

    Gui_showimage_24x24("/images/heat_24x24.bin",0 ,65);
    memset(ui_data,0,sizeof(ui_data));
    xsprintf(ui_data,"加热中");
    Gui_ShowChinese(27,72,ui_data,RED,BLACK,12,0);
    Gui_showimage_24x24("/images/fan_24x24.bin",0 ,89);
    memset(ui_data,0,sizeof(ui_data));
    xsprintf(ui_data,"吹风中");
    Gui_ShowChinese(27,101,ui_data,BLUE,BLACK,12,0);

    Gui_showimage_12x12("/images/clock_12x12.bin",0 ,148);
    memset(ui_data,0,sizeof(ui_data));
    xsprintf(ui_data,"运行时间");
    Gui_ShowChinese(13,148,ui_data,WHITE,BLACK,12,0);

    // 初始显示运行时长（rtcTask每秒更新）
    Gui_ShowString(49, 148, (u8*)"0d00:00:00", WHITE, BLACK, 12, 0);

    scene_Main_DrawStartButton();
    Gui_showimage_48x24("/images/mode_button_48x24.bin",71 ,116);
    Gui_showimage_12x12("/images/set_12x12.bin",113 ,148);
    scene_Main_DrawSelectBox(main_select_index, MAIN_SELECT_COLOR);
}

void scene_Main_Dynamic(void)
{
    u8 ui_data[16] = "";

    // 刷新左上角时间
    u8 time_buf[10]="";
    xsprintf((char*)time_buf, "%02d:%02d", calendar.hour, calendar.min);
    Gui_ShowString(0, 1, time_buf, WHITE, BLACK, 12, 0);

    // 当前温度/湿度读数
    print_string_gui(24,45,RED,BLACK,"%d.%d", g_cur_temp_x10 / 10, g_cur_temp_x10 % 10);
    print_string_gui(99,45,BLUE,BLACK,"%d.%d", g_cur_hum_x10 / 10, g_cur_hum_x10 % 10);

    // "加热中" 状态 — 根据继电器切换颜色
    memset(ui_data, 0, sizeof(ui_data));
    xsprintf((char*)ui_data, "加热中");
    Gui_ShowChinese(27, 72, ui_data, g_heater_on ? RED : GRAY1, BLACK, 12, 0);
    // 加热进度条 — 当前温度占目标温度的百分比
    {
        u8 pct = 0;
        if (g_target_temp_x10 > 0) {
            uint16_t v = (uint16_t)(g_cur_temp_x10 * 100 / g_target_temp_x10);
            if (v > 100) v = 100;
            pct = (u8)v;
        }
        Gui_DrawProgressBar(65, 74, 30, 8, pct, RED, WHITE);
        print_string_gui(100, 72, RED, BLACK, "%d%%", pct);
    }

    // "吹风中" 状态
    memset(ui_data, 0, sizeof(ui_data));
    xsprintf((char*)ui_data, "吹风中");
    Gui_ShowChinese(27, 101, ui_data, g_fan_on ? BLUE : GRAY1, BLACK, 12, 0);
    // 风扇进度条 — 当前湿度占目标湿度的百分比
    {
        u8 pct = 0;
        if (g_target_hum_x10 > 0) {
            uint16_t v = (uint16_t)(g_cur_hum_x10 * 100 / g_target_hum_x10);
            if (v > 100) v = 100;
            pct = (u8)v;
        }
        Gui_DrawProgressBar(65, 103, 30, 8, pct, BLUE, WHITE);
        print_string_gui(102, 100, BLUE, BLACK, "%d%%", pct);
    }

    // 刷新运行时长
    {
        uint32_t elapsed = g_elapsed_sec;
        uint32_t days  = elapsed / 86400;
        uint32_t rem   = elapsed % 86400;
        uint32_t hours = rem / 3600;
        rem %= 3600;
        uint32_t mins  = rem / 60;
        uint32_t secs  = rem % 60;

        u8 elapsed_str[16] = "";
        xsprintf((char*)elapsed_str, "%ud%02u:%02u:%02u", days, hours, mins, secs);
        Gui_ShowString(49, 148, elapsed_str, WHITE, BLACK, 12, 0);
    }
}

void scene_Mode_Static(void)
{
    u8 ui_data[32]="";

    /* 标题 + 返回箭头 */
    memset(ui_data,0,sizeof(ui_data));
    xsprintf(ui_data,"模式选择");
    Gui_ShowChinese(37,0,ui_data,WHITE,BLACK,12,0);
    Gui_showimage_12x12("/images/leftArrow_12x12.bin", 113,0);

    /* 数据表驱动：循环绘制每个模式项 */
    for (u8 i = MODE_SELECT_EGG; i <= MODE_SELECT_BREAD; i++)
    {
        Gui_showimage_24x24(MODE_ITEMS[i].image, 3, MODE_ITEMS[i].image_y);
        memset(ui_data,0,sizeof(ui_data));
        xsprintf(ui_data, MODE_ITEMS[i].text);
        Gui_ShowChinese(MODE_ITEMS[i].text_x, MODE_ITEMS[i].text_y, ui_data, WHITE, BLACK, 12, 0);
        Gui_showimage_12x12("/images/rightArrow_12x12.bin", 113, MODE_ITEMS[i].text_y);
    }

    scene_Mode_DrawSelectBox(mode_select_index, MAIN_SELECT_COLOR);
}

void scene_Mode_Dynamic(void)
{
    u8 ui_data[10]="";

    // 左上角显示当前时间 HH:MM
    xsprintf((char*)ui_data, "%02d:%02d", calendar.hour, calendar.min);
    Gui_ShowString(0, 1, ui_data, WHITE, BLACK, 12, 0);

    // 标题右侧显示当前页/总页数
    memset(ui_data,0,sizeof(ui_data));
    xsprintf((char*)ui_data,"1/2");
    Gui_ShowString(88, 1, ui_data, WHITE, BLACK, 12, 0);

    // 右上角返回图标
    Gui_showimage_12x12("/images/leftArrow_12x12.bin", 113,0);
}
void scene_Setting_Static(void)
{
    u8 ui_data[32]="";

    memset(ui_data,0,sizeof(ui_data));
    xsprintf(ui_data,"孵蛋设置");
    Gui_ShowChinese(40,0,ui_data,WHITE,BLACK,12,0);
    Gui_showimage_12x12("/images/leftArrow_12x12.bin", 113,0);

    Gui_showimage_24x24("/images/temp_24x24.bin", 5,22);
    scene_Setting_ShowValue(SETTING_SELECT_TEMP_SUB);
    Gui_showimage_12x12("/images/reduce_12x12.bin", 15,52);
    Gui_showimage_12x12("/images/add_12x12.bin", 37,52);

    Gui_DrawLine(64,20,64,66,WHITE);

    Gui_showimage_24x24("/images/hum_24x24.bin", 78,22);
    scene_Setting_ShowValue(SETTING_SELECT_HUM_SUB);
    Gui_showimage_12x12("/images/reduce_12x12.bin", 79,52);
    Gui_showimage_12x12("/images/add_12x12.bin", 101,52);

    Gui_DrawLine(0,68,127,68,WHITE);

    Gui_showimage_24x24("/images/heat_24x24.bin", 5,75);
    memset(ui_data,0,sizeof(ui_data));
    xsprintf(ui_data,"加热");
    Gui_ShowChinese(33,82,ui_data,WHITE,BLACK,12,0);
    print_string_gui(18,105,RED,BLACK,"60%%");

    Gui_DrawLine(64,74,64,116,WHITE);

    Gui_showimage_24x24("/images/fan_24x24.bin", 78,75);
    memset(ui_data,0,sizeof(ui_data));
    xsprintf(ui_data,"风速");
    Gui_ShowChinese(102,82,ui_data,WHITE,BLACK,12,0);
    print_string_gui(88,105,BLUE,BLACK,"90%%");

    Gui_showimage_12x12("/images/clock_12x12.bin", 5,121);
    memset(ui_data,0,sizeof(ui_data));
    xsprintf(ui_data,"定时");
    Gui_ShowChinese(24,121,ui_data,WHITE,BLACK,12,0);
    scene_Setting_TimerShowValue();

    Gui_showimage_24x24("/images/temp_curve_24x24.bin", 5,136);
    memset(ui_data,0,sizeof(ui_data));
    xsprintf(ui_data,"温度曲线");
    Gui_ShowChinese(48,142,ui_data,GREEN,BLACK,12,0);
    Gui_showimage_12x12("/images/rightArrow_12x12.bin", 113,142);

    Gui_ShowString(88, 1, (u8*)"1/2", WHITE, BLACK, 12, 0);
    scene_Setting_DrawSelectBox(setting_select_index, MAIN_SELECT_COLOR);
}

void scene_Setting_Dynamic(void)
{
    if (setting_page == SETTING_PAGE_TIME)
    {
        scene_SettingTime_Dynamic();
        return;
    }

    /* 定时器运行时每秒刷新剩余时间显示 */
    if (timer_state == TIMER_STATE_RUNNING || timer_state == TIMER_STATE_TIMEOUT)
    {
        scene_Setting_TimerShowValue();
    }
}

static void scene_SettingTime_Static(void)
{
    u8 ui_data[32] = "";

    memset(ui_data, 0, sizeof(ui_data));
    xsprintf(ui_data, "系统设置");
    Gui_ShowChinese(40, 0, ui_data, WHITE, BLACK, 12, 0);
    Gui_showimage_12x12("/images/leftArrow_12x12.bin", 113, 0);

    memset(ui_data, 0, sizeof(ui_data));
    xsprintf(ui_data, "系统时间");
    Gui_ShowChinese(38, 32, ui_data, WHITE, BLACK, 12, 0);

    scene_SettingTime_LoadCurrent();
    scene_SettingTime_ShowValue();

    memset(ui_data, 0, sizeof(ui_data));
    xsprintf(ui_data, "按下编辑");
    Gui_ShowChinese(38, 94, ui_data, GRAY1, BLACK, 12, 0);

    Gui_ShowString(88, 1, (u8*)"2/2", WHITE, BLACK, 12, 0);
    scene_SettingTime_DrawSelectBox(setting_time_select_index, MAIN_SELECT_COLOR);
}

static void scene_SettingTime_Dynamic(void)
{
    if (setting_time_editing == 0U)
    {
        scene_SettingTime_LoadCurrent();
        scene_SettingTime_ShowValue();
    }
}

/*========= 曲线场景 =========*/

static void scene_Curve_AddSample(int16_t temp, int16_t hum)
{
    if (curve_buf_count < CURVE_MAX_POINTS)
    {
        curve_temp_buf[curve_buf_count] = temp;
        curve_hum_buf[curve_buf_count] = hum;
        curve_buf_count++;
    }
    else
    {
        /* 缓冲区满，前移丢弃最旧数据 */
        for (uint16_t i = 1; i < CURVE_MAX_POINTS; i++)
        {
            curve_temp_buf[i - 1] = curve_temp_buf[i];
            curve_hum_buf[i - 1] = curve_hum_buf[i];
        }
        curve_temp_buf[CURVE_MAX_POINTS - 1] = temp;
        curve_hum_buf[CURVE_MAX_POINTS - 1] = hum;
    }
}

/* 画水平虚线（网格/设定值标志用） */
static void scene_Curve_DrawDashedHLine(u16 y, u16 color)
{
    u16 x0 = CURVE_PLOT_LEFT;
    u16 x1 = CURVE_PLOT_LEFT + CURVE_PLOT_W;
    for (u16 x = x0; x <= x1; x += 8)
    {
        u16 xe = x + 4;
        if (xe > x1) xe = x1;
        Gui_DrawLine(x, y, xe, y, color);
    }
}

/* 画垂直虚线网格 */
static void scene_Curve_DrawDashedVLine(u16 x, u16 color)
{
    u16 y0 = CURVE_PLOT_TOP;
    u16 y1 = CURVE_PLOT_TOP + CURVE_PLOT_H;
    for (u16 y = y0; y <= y1; y += 8)
    {
        u16 ye = y + 4;
        if (ye > y1) ye = y1;
        Gui_DrawLine(x, y, x, ye, color);
    }
}

/* 根据总数据量自动选择窗口量程（秒） */
static uint16_t scene_Curve_GetWindowSize(void)
{
    uint16_t n = curve_buf_count;
    if (n <= 60)       return 60;
    else if (n <= 120) return 120;
    else if (n <= 240) return 240;
    else if (n <= 480) return 480;
    else               return 600;
}

static void scene_Curve_DrawTopBar(void)
{
    u8 buf[8] = "";

    /* 返回箭头 */
    Gui_showimage_12x12("/images/leftArrow_12x12.bin", 0, 2);

    /* 曲线场景选择框 — 画在返回箭头和"温度"/"湿度"周围 */
    scene_Curve_DrawSelectBox(curve_select_index, MAIN_SELECT_COLOR);

    /* "温度" 选项 */
    memset(buf, 0, sizeof(buf));
    xsprintf((char*)buf, "温度");
    if (curve_mode == CURVE_MODE_TEMP)
        Gui_ShowChinese(20, 2, buf, RED, BLACK, 12, 0);
    else
        Gui_ShowChinese(20, 2, buf, WHITE, BLACK, 12, 0);

    /* "湿度" 选项 */
    memset(buf, 0, sizeof(buf));
    xsprintf((char*)buf, "湿度");
    if (curve_mode == CURVE_MODE_HUM)
        Gui_ShowChinese(54, 2, buf, BLUE, BLACK, 12, 0);
    else
        Gui_ShowChinese(54, 2, buf, WHITE, BLACK, 12, 0);

    /* 选中下划线指示 */
    u8 ux = (curve_mode == CURVE_MODE_TEMP) ? 20 : 54;
    u16 uc = (curve_mode == CURVE_MODE_TEMP) ? RED : BLUE;
    Gui_DrawLine(ux, 17, ux + 23, 17, uc);
}

static void scene_Curve_DrawAxes(void)
{
    u8 i;
    u8 buf[8] = "";

    /* 绘图区域边框 (四条边实线) */
    Gui_DrawLine(CURVE_PLOT_LEFT, CURVE_PLOT_TOP,
                 CURVE_PLOT_LEFT + CURVE_PLOT_W, CURVE_PLOT_TOP, WHITE);
    Gui_DrawLine(CURVE_PLOT_LEFT, CURVE_PLOT_TOP + CURVE_PLOT_H,
                 CURVE_PLOT_LEFT + CURVE_PLOT_W, CURVE_PLOT_TOP + CURVE_PLOT_H, WHITE);
    Gui_DrawLine(CURVE_PLOT_LEFT, CURVE_PLOT_TOP,
                 CURVE_PLOT_LEFT, CURVE_PLOT_TOP + CURVE_PLOT_H, WHITE);
    Gui_DrawLine(CURVE_PLOT_LEFT + CURVE_PLOT_W, CURVE_PLOT_TOP,
                 CURVE_PLOT_LEFT + CURVE_PLOT_W, CURVE_PLOT_TOP + CURVE_PLOT_H, WHITE);

    /* Y轴刻度 + 标签 (5档) — 先清标签区再画 */
    Lcd_Fill(0, CURVE_PLOT_TOP, CURVE_PLOT_LEFT - 1,
             CURVE_PLOT_TOP + CURVE_PLOT_H, BLACK);
    static const u8 pct[]  = {0, 25, 50, 75, 100};
    /* 温度: -10~100°C, 用 x10 表示为 -100~1000 */
    static const s16 t_val[] = {100, 72, 45, 17, -10};
    /* 湿度: 0~100%RH */
    static const s16 h_val[] = {100, 75, 50, 25, 0};
    const s16 *val = (curve_mode == CURVE_MODE_TEMP) ? t_val : h_val;

    for (i = 0; i < 5; i++)
    {
        u16 y = CURVE_PLOT_TOP + CURVE_PLOT_H * pct[i] / 100;
        /* 刻度线 (左外侧短实线) */
        Gui_DrawLine(CURVE_PLOT_LEFT - 3, y, CURVE_PLOT_LEFT, y, WHITE);
        /* 标签 (在绘图区左侧，不会被 Lcd_Fill 覆盖) */
        memset(buf, 0, sizeof(buf));
        xsprintf((char*)buf, "%d", val[i]);
        Gui_ShowString(0, y - 5, buf, WHITE, BLACK, 12, 0);
    }

    /* 湿度单位 % 标志（顶部） */
    if (curve_mode == CURVE_MODE_HUM)
        Gui_ShowChar(CURVE_PLOT_LEFT - 15, CURVE_PLOT_TOP + 1, '%', WHITE, BLACK, 12, 0);
}

static void scene_Curve_DrawGridLabels(void)
{
    u16 win = scene_Curve_GetWindowSize();

    /* ---- 横虚线网格 ---- */
    {
        static const u8 pct[] = {0, 25, 50, 75, 100};
        for (u8 g = 0; g < 5; g++)
        {
            u16 gy = CURVE_PLOT_TOP + CURVE_PLOT_H * pct[g] / 100;
            scene_Curve_DrawDashedHLine(gy, GRAY1);
        }
    }

    /* ---- 竖虚线网格 + 底部时间标签 ---- */
    {
        static const u8 pct[] = {0, 25, 50, 75, 100};
        u8 buf[8] = "";
        for (u8 g = 0; g < 5; g++)
        {
            u16 gx = CURVE_PLOT_LEFT + CURVE_PLOT_W * pct[g] / 100;
            if (g > 0 && g < 4)
                scene_Curve_DrawDashedVLine(gx, GRAY1);

            /* 底部时间标签 */
            u16 t_sec = (uint16_t)((uint32_t)win * pct[g] / 100);
            memset(buf, 0, sizeof(buf));
            if (t_sec < 60)
                xsprintf((char*)buf, "%d", t_sec);
            else
                xsprintf((char*)buf, "%dm", t_sec / 60);
            Gui_ShowString(gx - 6, CURVE_PLOT_TOP + CURVE_PLOT_H + 2, buf, WHITE, BLACK, 12, 0);
        }
    }

    /* ---- 运行时长（当前窗口采样总时长） ---- */
    {
        u8 buf[12] = "";
        if (win < 60)
            xsprintf((char*)buf, "%ds", win);
        else
            xsprintf((char*)buf, "%dm", win / 60);
        Gui_ShowString(CURVE_PLOT_LEFT + CURVE_PLOT_W - 24,
                       CURVE_PLOT_TOP + CURVE_PLOT_H + 2, buf, GRAY1, BLACK, 12, 0);
    }

    /* ---- 绘制设定值横虚线 + 标签 ---- */
    {
        int16_t y_min = (curve_mode == CURVE_MODE_TEMP) ? -100 : 0;
        int16_t y_max = (curve_mode == CURVE_MODE_TEMP) ? 1000 : 1000;
        int16_t y_range = y_max - y_min;
        int16_t setpoint = (curve_mode == CURVE_MODE_TEMP)
                           ? setting_temp_x10 : setting_hum_x10;
        u16 sp_color = (curve_mode == CURVE_MODE_TEMP) ? GREEN : WHITE;

        if (setpoint >= y_min && setpoint <= y_max && y_range > 0)
        {
            u16 sy = CURVE_PLOT_TOP + CURVE_PLOT_H
                     - (setpoint - y_min) * CURVE_PLOT_H / y_range;
            if (sy >= CURVE_PLOT_TOP && sy <= CURVE_PLOT_TOP + CURVE_PLOT_H)
            {
                scene_Curve_DrawDashedHLine(sy, sp_color);

                /* 在 Y 轴左侧标签区显示设定值 */
                u8 sp_buf[8] = "";
                if (curve_mode == CURVE_MODE_TEMP)
                    xsprintf((char*)sp_buf, "%d.%d", setpoint / 10, setpoint % 10);
                else
                    xsprintf((char*)sp_buf, "%d", setpoint / 10);
                /* 放在虚线稍上方，如果太靠近顶部则放到下方 */
                u8 sp_y = (sy < CURVE_PLOT_TOP + 8) ? sy + 2 : sy - 11;
                Gui_ShowString(0, sp_y, sp_buf, sp_color, BLACK, 12, 0);
            }
        }
    }
}

static void scene_Curve_DrawData(void)
{
    u16 total = curve_buf_count;
    if (total == 0) return;

    int16_t *data = (curve_mode == CURVE_MODE_TEMP) ? curve_temp_buf : curve_hum_buf;
    u16 color = (curve_mode == CURVE_MODE_TEMP) ? RED : BLUE;
    u16 win = scene_Curve_GetWindowSize();

    /* Y轴范围 */
    int16_t y_min, y_max;
    if (curve_mode == CURVE_MODE_TEMP)
    {
        y_min = -100;    /* -10.0°C */
        y_max = 1000;    /* 100.0°C */
    }
    else
    {
        y_min = 0;       /* 0%RH */
        y_max = 1000;    /* 100%RH */
    }
    int16_t y_range = y_max - y_min;
    if (y_range <= 0) return;

    /* 窗口中第一个采样点在数组中的索引 */
    s16 first_idx = (s16)total - (s16)win;
    if (first_idx < 0) first_idx = 0;
    u16 actual = total - (u16)first_idx;
    if (actual < 2) return;

    /* ===== 第1步：擦除旧曲线（仅旧曲线像素，不碰网格） ===== */
    {
        u16 prev_px = 0xFFFF;
        u16 prev_py = 0;
        for (u16 px = 0; px < CURVE_PLOT_W; px++)
        {
            u16 py = curve_prev_py[px];
            if (py == 0xFFFF)
            {
                prev_px = 0xFFFF;   /* 空洞，重置段 */
                continue;
            }
            if (prev_px != 0xFFFF)
            {
                Gui_DrawLine(CURVE_PLOT_LEFT + prev_px, prev_py,
                             CURVE_PLOT_LEFT + px,      py, BLACK);
            }
            prev_px = px;
            prev_py = py;
        }
    }

    /* ===== 第2步：画新曲线，同时记录到 curve_prev_py ===== */
    {
        u16 prev_px = 0xFFFF;
        u16 prev_py = 0;

        for (u16 px = 0; px < CURVE_PLOT_W; px++)
        {
            /* 按 win（时间窗口）映射像素 → 时间偏移（秒） */
            u16 t_offset = (u16)((uint32_t)px * win / CURVE_PLOT_W);

            /* 超出已有数据范围的像素不画，归为空洞 */
            if (t_offset >= actual)
            {
                prev_px = 0xFFFF;
                curve_prev_py[px] = 0xFFFF;
                continue;
            }

            u16 idx = (u16)first_idx + t_offset;
            int16_t v = data[idx];

            u16 py = CURVE_PLOT_TOP + CURVE_PLOT_H
                     - (v - y_min) * CURVE_PLOT_H / y_range;
            if (py < CURVE_PLOT_TOP) py = CURVE_PLOT_TOP;
            if (py > CURVE_PLOT_TOP + CURVE_PLOT_H) py = CURVE_PLOT_TOP + CURVE_PLOT_H;

            curve_prev_py[px] = py;    /* 保存用于下一帧擦除 */

            if (prev_px != 0xFFFF)
            {
                Gui_DrawLine(CURVE_PLOT_LEFT + prev_px, prev_py,
                             CURVE_PLOT_LEFT + px,      py, color);
            }
            prev_px = px;
            prev_py = py;
        }
    }
}

static void scene_Curve_Static(void)
{
    /* 初始化上一帧像素表 (0xFFFF = 无效，让第一帧不擦除) */
    for (u16 i = 0; i < CURVE_PLOT_W; i++)
        curve_prev_py[i] = 0xFFFF;

    scene_Curve_DrawTopBar();
    scene_Curve_DrawAxes();
    scene_Curve_DrawGridLabels();
    scene_Curve_DrawData();
}

static void scene_Curve_Dynamic(void)
{
    scene_Curve_DrawData();
}

static void scene_Curve_SwitchMode(u8 new_mode)
{
    curve_mode = new_mode;
    /* 清除整个绘图区 — 旧模式的设定值线和旧曲线一起擦掉 */
    Lcd_Fill(CURVE_PLOT_LEFT, CURVE_PLOT_TOP,
             CURVE_PLOT_LEFT + CURVE_PLOT_W, CURVE_PLOT_TOP + CURVE_PLOT_H, BLACK);

    /* 重置上一帧像素表，让下一帧不擦除空气 */
    for (u16 i = 0; i < CURVE_PLOT_W; i++)
        curve_prev_py[i] = 0xFFFF;

    scene_Curve_DrawTopBar();
    scene_Curve_DrawAxes();
    scene_Curve_DrawGridLabels();
    scene_Curve_DrawData();
}

static void scene_Curve_HandleInput(uint32_t flags)
{
    if ((flags & GUI_FLAG_EC11_CW) != 0U)
    {
        scene_Curve_SelectNext();
    }

    if ((flags & GUI_FLAG_EC11_CCW) != 0U)
    {
        scene_Curve_SelectPrev();
    }

    if ((flags & GUI_FLAG_EC11_PRESS) != 0U)
    {
        switch (curve_select_index)
        {
            case CURVE_SELECT_BACK:
                /* 返回设置界面 */
                gui_current_scene = GUI_SCENE_SETTING;
                setting_editing = 0;
                setting_timer_editing = 0;
                Lcd_Clear(BLACK);
                scene_Setting_Static();
                scene_Setting_Dynamic();
                break;

            case CURVE_SELECT_TEMP:
                scene_Curve_SwitchMode(CURVE_MODE_TEMP);
                break;

            case CURVE_SELECT_HUM:
                scene_Curve_SwitchMode(CURVE_MODE_HUM);
                break;

            default:
                break;
        }
    }
}

void guiTask(void* argument)
{
    scene_Main_Static();

    while (1)
    {
        uint32_t flags = osThreadFlagsWait(GUI_FLAG_ALL, osFlagsWaitAny, osWaitForever);

        if ((flags & (GUI_FLAG_EC11_CW | GUI_FLAG_EC11_CCW | GUI_FLAG_EC11_PRESS)) != 0U)
        {
            switch (gui_current_scene)
            {
                case GUI_SCENE_MAIN:
                    scene_Main_HandleInput(flags);
                    break;

                case GUI_SCENE_MODE:
                    scene_Mode_HandleInput(flags);
                    break;

                case GUI_SCENE_SETTING:
                    scene_Setting_HandleInput(flags);
                    break;

                case GUI_SCENE_CURVE:
                    scene_Curve_HandleInput(flags);
                    break;

                default:
                    break;
            }
        }

        if ((flags & GUI_FLAG_RTC_UPDATE) != 0U)
        {
            /* ===== 定时器倒计时（每秒 tick） ===== */
            if (timer_state == TIMER_STATE_RUNNING)
            {
                uint32_t now = g_elapsed_sec;
                if (now >= timer_target_sec)
                {
                    timer_state = TIMER_STATE_TIMEOUT;
                    g_timer_timeout = 1;
                    main_is_running = 0;                /* 回到待机 */
                    control_heater_set(0);              /* 关闭加热 */
                    control_fan_set(0);                 /* 关闭风扇 */
                    buzz_play(300);                     /* 蜂鸣器报警 300ms */

                    /* 如果在设置界面，刷新显示 "TIME UP!" */
                    if (gui_current_scene == GUI_SCENE_SETTING && setting_page == SETTING_PAGE_MAIN)
                    {
                        scene_Setting_TimerShowValue();
                    }
                    /* 如果在主界面，刷新按钮显示 */
                    if (gui_current_scene == GUI_SCENE_MAIN)
                    {
                        scene_Main_DrawStartButton();
                        scene_Main_DrawSelectBox(main_select_index, MAIN_SELECT_COLOR);
                    }
                }
            }

            /* ===== 曲线数据采样（每秒记录） ===== */
            {
                /* DS18B20 两步法测温：
                 * 第1秒: 读取上次转换结果 → 启动下一次转换
                 * 第2秒: 转换完成(~750ms) → 读取 → 启动下一次
                 * 每秒更新一次，不阻塞 guiTask */
                if (g_temp_pending)
                {
                    short t = ds18b20_read_temperature();
                    if (t >= -550 && t <= 1250)
                    {
                        g_cur_temp_x10 = t;
                    }
                    /* 通讯失败 (0x7FFF) 或超范围 — 保留上次有效值 */
                }
                else
                {
                    /* 首次 pending=0，先启动转换，等下一轮再读数 */
                    ds18b20_start();
                    g_temp_pending = 1;
                    goto skip_sampling;  /* 第1秒不加点 */
                }
                ds18b20_start();        /* 启动下一次转换 */

                /* 每秒加一个采样点，保证曲线时间轴连续 */
                {
                    /* 通过 CJHR-31 查表计算湿度 */
                    int16_t temp_c = g_cur_temp_x10 / 10;   /* x10 → °C */
                    int16_t rh_x10 = cjhr31_calc_rh(g_hum_res_ohm_x100, temp_c);
                    if (rh_x10 >= 0) {
                        g_cur_hum_x10 = rh_x10;
                    }
                }

                scene_Curve_AddSample(g_cur_temp_x10, g_cur_hum_x10);
            skip_sampling: ;
            }

            /* 控制 tick — 仅在启动状态下输出继电器 */
            if (main_is_running) {
                control_tick(g_cur_temp_x10, g_cur_hum_x10);
            }
            }

            switch (gui_current_scene)
            {
                case GUI_SCENE_MAIN:
                    scene_Main_Dynamic();
                    break;

                case GUI_SCENE_MODE:
                    scene_Mode_Dynamic();
                    break;

                case GUI_SCENE_SETTING:
                    scene_Setting_Dynamic();
                    break;

                case GUI_SCENE_CURVE:
                    scene_Curve_Dynamic();
                    break;

                default:
                    break;
            }
        }
    }