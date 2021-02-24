/**
 * @file lv_calendar.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_calendar.h"
#include "../../../lvgl.h"
#if LV_USE_CALENDAR

/*********************
 *      DEFINES
 *********************/
#define LV_CALENDAR_CTRL_TODAY      LV_BTNMATRIX_CTRL_CUSTOM_1
#define LV_CALENDAR_CTRL_HIGHLIGHT  LV_BTNMATRIX_CTRL_CUSTOM_2

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void my_constructor(lv_obj_t * obj, const lv_obj_t * copy);
static void draw_event_cb(lv_obj_t * obj, lv_event_t e);

static uint8_t get_day_of_week(uint32_t year, uint32_t month, uint32_t day);
static uint8_t get_month_length(int32_t year, int32_t month);
static uint8_t is_leap_year(uint32_t year);
static void highlight_update(lv_obj_t * calendar);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_calendar_class = {
    .constructor_cb = my_constructor,
    .instance_size = sizeof(lv_calendar_t),
    .base_class = &lv_btnmatrix_class
};


static const char * day_names_def[7] = LV_CALENDAR_DEFAULT_DAY_NAMES;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_calendar_create(lv_obj_t * parent)
{
    lv_obj_t * obj = lv_obj_create_from_class(&lv_calendar_class, parent, NULL);

    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_calendar_set_day_names(lv_obj_t * obj, const char * day_names[])
{
    lv_calendar_t * calendar = (lv_calendar_t *) obj;
    uint32_t i;
    for(i = 0; i < 7; i++) {
        calendar->map[i] = day_names[i];
    }
}

void lv_calendar_set_today_date(lv_obj_t * obj, lv_calendar_date_t * today)
{
    LV_ASSERT_NULL(today);
    lv_calendar_t * calendar = (lv_calendar_t *) obj;
    calendar->today.year         = today->year;
    calendar->today.month        = today->month;
    calendar->today.day          = today->day;

    highlight_update(obj);
}

void lv_calendar_set_highlighted_dates(lv_obj_t * obj, lv_calendar_date_t highlighted[], uint16_t date_num)
{
    LV_ASSERT_NULL(highlighted);

    lv_calendar_t * calendar = (lv_calendar_t *) obj;
    calendar->highlighted_dates     = highlighted;
    calendar->highlighted_dates_num = date_num;

    highlight_update(obj);
}

void lv_calendar_set_showed_date(lv_obj_t * obj, lv_calendar_date_t * showed)
{
    LV_ASSERT_NULL(showed);

    lv_calendar_t * calendar = (lv_calendar_t *) obj;
    calendar->showed_date.year   = showed->year;
    calendar->showed_date.month  = showed->month;
    calendar->showed_date.day    = showed->day;

    lv_calendar_date_t d;
    d.year = calendar->showed_date.year;
    d.month = calendar->showed_date.month;
    d.day = calendar->showed_date.day;

    uint8_t i;

    /*Remove the disabled state but revert it for day names*/
    lv_btnmatrix_clear_btn_ctrl_all(obj, LV_BTNMATRIX_CTRL_DISABLED);
    for(i = 0; i < 7; i++) {
        lv_btnmatrix_set_btn_ctrl(obj, i, LV_BTNMATRIX_CTRL_DISABLED);
    }

    uint8_t act_mo_len = get_month_length(d.year, d.month);
    uint8_t day_first = get_day_of_week(d.year, d.month, 1);
    uint8_t c;
    for(i = day_first, c = 1; i < act_mo_len + day_first; i++, c++) {
        lv_snprintf(calendar->nums[i], sizeof(calendar->nums[0]), "%d", c);
    }

    uint8_t prev_mo_len = get_month_length(d.year, d.month - 1);
    for(i = 0, c = prev_mo_len - day_first + 1; i < day_first; i++, c++) {
        lv_snprintf(calendar->nums[i], sizeof(calendar->nums[0]), "%d", c);
        lv_btnmatrix_set_btn_ctrl(obj, i + 7, LV_BTNMATRIX_CTRL_DISABLED);
    }

    for(i = day_first + act_mo_len, c = 1; i < 6 * 7; i++, c++) {
        lv_snprintf(calendar->nums[i], sizeof(calendar->nums[0]), "%d", c);
        lv_btnmatrix_set_btn_ctrl(obj, i + 7, LV_BTNMATRIX_CTRL_DISABLED);
    }

    highlight_update(obj);
    lv_obj_invalidate(obj);
}

/*=====================
 * Getter functions
 *====================*/

/**
 * Get the today's date
 * @param calendar pointer to a calendar object
 * @return return pointer to an `lv_calendar_date_t` variable containing the date of today.
 */
const lv_calendar_date_t * lv_calendar_get_today_date(const lv_obj_t * obj)
{
    const lv_calendar_t * calendar = (lv_calendar_t *) obj;
    return &calendar->today;
}

/**
 * Get the currently showed
 * @param calendar pointer to a calendar object
 * @return pointer to an `lv_calendar_date_t` variable containing the date is being shown.
 */
const lv_calendar_date_t * lv_calendar_get_showed_date(const lv_obj_t * obj)
{
    const lv_calendar_t * calendar = (lv_calendar_t *) obj;
    return &calendar->showed_date;
}

/**
 * Get the the highlighted dates
 * @param calendar pointer to a calendar object
 * @return pointer to an `lv_calendar_date_t` array containing the dates.
 */
lv_calendar_date_t * lv_calendar_get_highlighted_dates(const lv_obj_t * obj)
{
    lv_calendar_t * calendar = (lv_calendar_t *) obj;
    return calendar->highlighted_dates;
}

uint16_t lv_calendar_get_highlighted_dates_num(const lv_obj_t * obj)
{
    lv_calendar_t * calendar = (lv_calendar_t *) obj;
    return calendar->highlighted_dates_num;
}

bool lv_calendar_get_pressed_date(const lv_obj_t * obj, lv_calendar_date_t * date)
{
    lv_calendar_t * calendar = (lv_calendar_t *) obj;
    uint16_t d = lv_btnmatrix_get_active_btn(obj);
    if(d == LV_BTNMATRIX_BTN_NONE) {
        date->year = 0;
        date->month = 0;
        date->day = 0;
        return false;
    }

    const char * txt = lv_btnmatrix_get_btn_text(obj, lv_btnmatrix_get_active_btn(obj));

    if(txt[1] == 0) date->day = txt[0] - '0';
    else date->day = (txt[0] - '0') * 10 + (txt[1] - '0');

    date->year = calendar->showed_date.year;
    date->month = calendar->showed_date.month;

    return true;
}


/**********************
 *  STATIC FUNCTIONS
 **********************/

static void my_constructor(lv_obj_t * obj, const lv_obj_t * copy)
{
    LV_UNUSED(copy);
    lv_calendar_t * calendar = (lv_calendar_t *) obj;

    /*Initialize the allocated 'ext' */
    calendar->today.year  = 2020;
    calendar->today.month = 1;
    calendar->today.day   = 1;

    calendar->showed_date.year  = 2020;
    calendar->showed_date.month = 1;
    calendar->showed_date.day   = 1;

    calendar->highlighted_dates      = NULL;
    calendar->highlighted_dates_num  = 0;

    lv_obj_set_size(obj, LV_DPX(240), LV_DPX(240));

    lv_memset_00(calendar->nums, sizeof(calendar->nums));
    uint8_t i;
    uint8_t j = 0;
    for(i = 0; i < 8 * 7; i++) {
        /*Every 8th string is "\n"*/
        if(i != 0 && (i + 1) % 8 == 0) {
            calendar->map[i] = "\n";
        } else if(i < 8){
            calendar->map[i] = day_names_def[i];
        } else {
            calendar->nums[j][0] = 'x';
            calendar->map[i] = calendar->nums[j];
            j++;
        }
    }
    calendar->map[8 * 7 - 1] = "";

    lv_btnmatrix_set_map(obj, calendar->map);
    lv_btnmatrix_set_btn_ctrl_all(obj, LV_BTNMATRIX_CTRL_CLICK_TRIG | LV_BTNMATRIX_CTRL_NO_REPEAT);


    lv_calendar_set_showed_date(obj, &calendar->showed_date);
    lv_calendar_set_today_date(obj, &calendar->today);

    lv_obj_add_event_cb(obj, draw_event_cb, NULL);

}

static void draw_event_cb(lv_obj_t * obj, lv_event_t e)
{
    if(e == LV_EVENT_DRAW_PART_BEGIN) {
        lv_obj_draw_hook_dsc_t * hook_dsc = lv_event_get_param();
        if(hook_dsc->part == LV_PART_ITEMS) {
            /*Day name styles*/
            if(hook_dsc->id < 7) {
                hook_dsc->rect_dsc->bg_opa = LV_OPA_TRANSP;
                hook_dsc->rect_dsc->border_opa = LV_OPA_TRANSP;
            }
            else if(lv_btnmatrix_has_btn_ctrl(obj, hook_dsc->id, LV_BTNMATRIX_CTRL_DISABLED)) {
                hook_dsc->rect_dsc->bg_opa = LV_OPA_TRANSP;
                hook_dsc->rect_dsc->border_opa = LV_OPA_TRANSP;
                hook_dsc->label_dsc->color = lv_color_grey();
            }

            if(lv_btnmatrix_has_btn_ctrl(obj, hook_dsc->id, LV_CALENDAR_CTRL_HIGHLIGHT)) {
                hook_dsc->rect_dsc->bg_opa = LV_OPA_40;
                hook_dsc->rect_dsc->bg_color = lv_theme_get_color_primary();
                if(lv_btnmatrix_get_pressed_btn(obj) == hook_dsc->id) {
                    hook_dsc->rect_dsc->bg_opa = LV_OPA_70;
                }
            }

            if(lv_btnmatrix_has_btn_ctrl(obj, hook_dsc->id, LV_CALENDAR_CTRL_TODAY)) {
                hook_dsc->rect_dsc->border_opa = LV_OPA_COVER;
                hook_dsc->rect_dsc->border_color = lv_theme_get_color_primary();
                hook_dsc->rect_dsc->border_width += 1;
            }

        }
    }
}




/**
 * Get the number of days in a month
 * @param year a year
 * @param month a month. The range is basically [1..12] but [-11..0] or [13..24] is also
 *              supported to handle next/prev. year
 * @return [28..31]
 */
static uint8_t get_month_length(int32_t year, int32_t month)
{
    month--;
    if(month < 0) {
        year--;             /*Already in the previous year (won't be less then -12 to skip a whole year)*/
        month = 12 + month; /*`month` is negative, the result will be < 12*/
    }
    if(month >= 12) {
        year++;
        month -= 12;
    }

    /*month == 1 is february*/
    return (month == 1) ? (28 + is_leap_year(year)) : 31 - month % 7 % 2;
}

/**
 * Tells whether a year is leap year or not
 * @param year a year
 * @return 0: not leap year; 1: leap year
 */
static uint8_t is_leap_year(uint32_t year)
{
    return (year % 4) || ((year % 100 == 0) && (year % 400)) ? 0 : 1;
}

/**
 * Get the day of the week
 * @param year a year
 * @param month a  month [1..12]
 * @param day a day [1..32]
 * @return [0..6] which means [Sun..Sat] or [Mon..Sun] depending on LV_CALENDAR_WEEK_STARTS_MONDAY
 */
static uint8_t get_day_of_week(uint32_t year, uint32_t month, uint32_t day)
{
    uint32_t a = month < 3 ? 1 : 0;
    uint32_t b = year - a;

#if LV_CALENDAR_WEEK_STARTS_MONDAY
    uint32_t day_of_week = (day + (31 * (month - 2 + 12 * a) / 12) + b + (b / 4) - (b / 100) + (b / 400) - 1) % 7;
#else
    uint32_t day_of_week = (day + (31 * (month - 2 + 12 * a) / 12) + b + (b / 4) - (b / 100) + (b / 400)) % 7;
#endif

    return day_of_week;
}

static void highlight_update(lv_obj_t * obj)
{
    lv_calendar_t * calendar = (lv_calendar_t *) obj;
    uint16_t i;

    /*Clear all kind of selection*/
    lv_btnmatrix_clear_btn_ctrl_all(obj, LV_CALENDAR_CTRL_TODAY | LV_CALENDAR_CTRL_HIGHLIGHT);

    if(calendar->highlighted_dates) {
        for(i = 0; i < calendar->highlighted_dates_num; i++) {
            if(calendar->highlighted_dates[i].year == calendar->today.year && calendar->highlighted_dates[i].month == calendar->showed_date.month) {
                lv_btnmatrix_set_btn_ctrl(obj, calendar->highlighted_dates[i].day + 7, LV_CALENDAR_CTRL_HIGHLIGHT);
            }
        }
    }

    if(calendar->showed_date.year == calendar->today.year && calendar->showed_date.month == calendar->today.month) {
        uint8_t day_first = get_day_of_week(calendar->today.year, calendar->today.month, calendar->today.day - 1);
        lv_btnmatrix_set_btn_ctrl(obj, calendar->today.day + day_first + 7, LV_CALENDAR_CTRL_TODAY);
    }
}

#endif  /* LV_USE_CALENDAR*/