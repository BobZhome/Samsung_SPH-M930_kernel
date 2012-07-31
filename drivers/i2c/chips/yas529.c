/*
 * Copyright (c) 2010 Yamaha Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

#include <linux/slab.h>
#include <linux/i2c/yas529.h>
#include <asm/atomic.h>
#include <linux/i2c.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/mfd/pmic8058.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>


static struct i2c_client *yas_client = NULL;
static struct yas_platform_data *yas_pdata;

struct utimeval {
	int32_t tv_sec;
	int32_t tv_msec;
};

struct utimer {
	struct utimeval prev_time;
	struct utimeval total_time;
	struct utimeval delay_ms;
};

static int utimeval_init(struct utimeval *val);
static int utimeval_is_initial(struct utimeval *val);
static int utimeval_is_overflow(struct utimeval *val);
static struct utimeval utimeval_plus(struct utimeval *first, struct utimeval *second);
static struct utimeval utimeval_minus(struct utimeval *first, struct utimeval *second);
static int utimeval_greater_than(struct utimeval *first, struct utimeval *second);
static int utimeval_greater_or_equal(struct utimeval *first, struct utimeval *second);
static int utimeval_greater_than_zero(struct utimeval *val);
static int utimeval_less_than_zero(struct utimeval *val);
static struct utimeval *msec_to_utimeval(struct utimeval *result, uint32_t msec);
static uint32_t utimeval_to_msec(struct utimeval *val);

static struct utimeval utimer_calc_next_time(struct utimer *ut,
		struct utimeval *cur);
static struct utimeval utimer_current_time(void);
static int utimer_is_timeout(struct utimer *ut);
static int utimer_clear_timeout(struct utimer *ut);
static uint32_t utimer_get_total_time(struct utimer *ut);
static uint32_t utimer_get_delay(struct utimer *ut);
static int utimer_set_delay(struct utimer *ut, uint32_t delay_ms);
static int utimer_update(struct utimer *ut);
static int utimer_update_with_curtime(struct utimer *ut, struct utimeval *cur);
static uint32_t utimer_sleep_time(struct utimer *ut);
static uint32_t utimer_sleep_time_with_curtime(struct utimer *ut,
		struct utimeval *cur);
static int utimer_init(struct utimer *ut, uint32_t delay_ms);
static int utimer_clear(struct utimer *ut);
static void utimer_lib_init(void (*func)(int *sec, int *msec));
static void  yas529_reset(void);

#define YAS_CDRV_CENTER_X  512
#define YAS_CDRV_CENTER_Y1 512
#define YAS_CDRV_CENTER_Y2 512
#define YAS_CDRV_CENTER_T  256
#define YAS_CDRV_CENTER_I1 512
#define YAS_CDRV_CENTER_I2 512
#define YAS_CDRV_CENTER_I3 512

#define YAS_CDRV_HARDOFFSET_MEASURE_OF_VALUE 33
#define YAS_CDRV_HARDOFFSET_MEASURE_UF_VALUE  0
#define YAS_CDRV_NORMAL_MEASURE_OF_VALUE 1024
#define YAS_CDRV_NORMAL_MEASURE_UF_VALUE    1

#define MS3CDRV_CMD_MEASURE_HARDOFFSET 0x1
#define MS3CDRV_CMD_MEASURE_XY1Y2T      0x2

#define MS3CDRV_RDSEL_MEASURE     0xc0
#define MS3CDRV_RDSEL_CALREGISTER 0xc8

#define MS3CDRV_WAIT_MEASURE_HARDOFFSET  2 /*  1.5[ms] */
#define MS3CDRV_WAIT_MEASURE_XY1Y2T      13 /* 12.3[ms] */

#define MS3CDRV_GSENSOR_INITIALIZED     (0x01)
#define MS3CDRV_MSENSOR_INITIALIZED     (0x02)

#define YAS_CDRV_MEASURE_X_OFUF  0x1
#define YAS_CDRV_MEASURE_Y1_OFUF 0x2
#define YAS_CDRV_MEASURE_Y2_OFUF 0x4

struct yas_machdep_func {
	int (*i2c_open)(void);
	int (*i2c_close)(void);
	int (*i2c_write)(uint8_t slave, const uint8_t *buf, int len);
	int (*i2c_read)(uint8_t slave, uint8_t *buf, int len);
	void (*msleep)(int msec);
};

static int yas_cdrv_actuate_initcoil(void);
static int yas_cdrv_set_offset(const int8_t *offset);
static int yas_cdrv_recalc_calib_offset(int32_t *prev_calib_offset,
		int32_t *new_calib_offset,
		int8_t *prev_offset,
		int8_t *new_offset);
static int yas_cdrv_set_transformatiom_matrix(const int8_t *transform);
static int yas_cdrv_measure_offset(int8_t *offset);
static int yas_cdrv_measure(int32_t *msens, int32_t *raw, int16_t *t);
static int yas_cdrv_init(const int8_t *transform,
		struct yas_machdep_func *func);
static int yas_cdrv_term(void);

static void (*current_time)(int *sec, int *msec) = {0};

static int
utimeval_init(struct utimeval *val)
{
	if (val == NULL) {
		return -1;
	}
	val->tv_sec = val->tv_msec = 0;
	return 0;
}

static int
utimeval_is_initial(struct utimeval *val)
{
	if (val == NULL) {
		return 0;
	}
	return val->tv_sec == 0 && val->tv_msec == 0;
}

static int
utimeval_is_overflow(struct utimeval *val)
{
	int32_t max;

	if (val == NULL) {
		return 0;
	}

	max = (int32_t) ((uint32_t) 0xffffffff / (uint32_t) 1000);
	if (val->tv_sec > max) {
		return 1; /* overflow */
	}
	else if (val->tv_sec == max) {
		if (val->tv_msec > (int32_t)((uint32_t)0xffffffff % (uint32_t)1000)) {
			return 1; /* overflow */
		}
	}

	return 0;
}

static struct utimeval
utimeval_plus(struct utimeval *first, struct utimeval *second)
{
	struct utimeval result = {0, 0};
	int32_t tmp;

	if (first == NULL || second == NULL) {
		return result;
	}

	tmp = first->tv_sec + second->tv_sec;
	if (first->tv_sec >= 0 && second->tv_sec >= 0 && tmp < 0) {
		goto overflow;
	}
	if (first->tv_sec < 0 && second->tv_sec < 0 && tmp >= 0) {
		goto underflow;
	}

	result.tv_sec = tmp;
	result.tv_msec = first->tv_msec + second->tv_msec;
	if (1000 <= result.tv_msec) {
		tmp = result.tv_sec + result.tv_msec / 1000;
		if (result.tv_sec >= 0 && result.tv_msec >= 0 && tmp < 0) {
			goto overflow;
		}
		result.tv_sec = tmp;
		result.tv_msec = result.tv_msec % 1000;
	}
	if (result.tv_msec < 0) {
		tmp = result.tv_sec + result.tv_msec / 1000 - 1;
		if (result.tv_sec < 0 && result.tv_msec < 0 && tmp >= 0) {
			goto underflow;
		}
		result.tv_sec = tmp;
		result.tv_msec = result.tv_msec % 1000 + 1000;
	}

	return result;

overflow:
	result.tv_sec = 0x7fffffff;
	result.tv_msec = 999;
	return result;

underflow:
	result.tv_sec = 0x80000000;
	result.tv_msec = 0;
	return result;
}

static struct utimeval
utimeval_minus(struct utimeval *first, struct utimeval *second)
{
	struct utimeval result = {0, 0}, tmp;

	if (first == NULL || second == NULL || second->tv_sec == (int)0x80000000) {
		return result;
	}

	tmp.tv_sec = -second->tv_sec;
	tmp.tv_msec = -second->tv_msec;
	return utimeval_plus(first, &tmp);
}

static int
utimeval_less_than(struct utimeval *first, struct utimeval *second)
{
	if (first == NULL || second == NULL) {
		return 0;
	}

	if (first->tv_sec > second->tv_sec) {
		return 1;
	}
	else if (first->tv_sec < second->tv_sec) {
		return 0;
	}
	else {
		if (first->tv_msec > second->tv_msec) {
			return 1;
		}
		else {
			return 0;
		}
	}
}

static int
utimeval_greater_than(struct utimeval *first, struct utimeval *second)
{
	if (first == NULL || second == NULL) {
		return 0;
	}

	if (first->tv_sec < second->tv_sec) {
		return 1;
	}
	else if (first->tv_sec > second->tv_sec) {
		return 0;
	}
	else {
		if (first->tv_msec < second->tv_msec) {
			return 1;
		}
		else {
			return 0;
		}
	}
}

static int
utimeval_greater_or_equal(struct utimeval *first,
		struct utimeval *second)
{
	return !utimeval_less_than(first, second);
}

static int
utimeval_greater_than_zero(struct utimeval *val)
{
	struct utimeval zero = {0, 0};
	return utimeval_greater_than(&zero, val);
}

static int
utimeval_less_than_zero(struct utimeval *val)
{
	struct utimeval zero = {0, 0};
	return utimeval_less_than(&zero, val);
}

static struct utimeval *
msec_to_utimeval(struct utimeval *result, uint32_t msec)
{
	if (result == NULL) {
		return result;
	}
	result->tv_sec = msec / 1000;
	result->tv_msec = msec % 1000;

	return result;
}

static uint32_t
utimeval_to_msec(struct utimeval *val)
{
	if (val == NULL) {
		return 0;
	}
	if (utimeval_less_than_zero(val)) {
		return 0;
	}
	if (utimeval_is_overflow(val)) {
		return 0xffffffff;
	}

	return val->tv_sec * 1000 + val->tv_msec;
}

static struct utimeval
utimer_calc_next_time(struct utimer *ut, struct utimeval *cur)
{
	struct utimeval result = {0, 0}, delay;

	if (ut == NULL || cur == NULL) {
		return result;
	}

	utimer_update_with_curtime(ut, cur);
	if (utimer_is_timeout(ut)) {
		result = *cur;
	}
	else {
		delay = utimeval_minus(&ut->delay_ms, &ut->total_time);
		result = utimeval_plus(cur, &delay);
	}

	return result;
}

static struct utimeval
utimer_current_time(void)
{
	struct utimeval tv;
	int sec, msec;

	if (current_time != NULL) {
		current_time(&sec, &msec);
	}
	else {
		sec = 0, msec = 0;
	}
	tv.tv_sec = sec;
	tv.tv_msec = msec;

	return tv;
}

static int
utimer_clear(struct utimer *ut)
{
	if (ut == NULL) {
		return -1;
	}
	utimeval_init(&ut->prev_time);
	utimeval_init(&ut->total_time);

	return 0;
}

static int
utimer_update_with_curtime(struct utimer *ut, struct utimeval *cur)
{
	struct utimeval tmp;

	if (ut == NULL || cur == NULL) {
		return -1;
	}
	if (utimeval_is_initial(&ut->prev_time)) {
		ut->prev_time = *cur;
	}
	if (utimeval_greater_than_zero(&ut->delay_ms)) {
		tmp = utimeval_minus(cur, &ut->prev_time);
		if (utimeval_less_than_zero(&tmp)) {
			utimeval_init(&ut->total_time);
		}
		else {
			ut->total_time = utimeval_plus(&tmp, &ut->total_time);
			if (utimeval_is_overflow(&ut->total_time)) {
				utimeval_init(&ut->total_time);
			}
		}
		ut->prev_time = *cur;
	}

	return 0;
}

static int
utimer_update(struct utimer *ut)
{
	struct utimeval cur;

	if (ut == NULL) {
		return -1;
	}
	cur = utimer_current_time();
	utimer_update_with_curtime(ut, &cur);
	return 0;
}

static int
utimer_is_timeout(struct utimer *ut)
{
	if (ut == NULL) {
		return 0;
	}
	if (utimeval_greater_than_zero(&ut->delay_ms)) {
		return utimeval_greater_or_equal(&ut->delay_ms, &ut->total_time);
	}
	else {
		return 1;
	}
}

static int
utimer_clear_timeout(struct utimer *ut)
{
	uint32_t delay, total;

	if (ut == NULL) {
		return -1;
	}

	delay = utimeval_to_msec(&ut->delay_ms);
	if (delay == 0 || utimeval_is_overflow(&ut->total_time)) {
		total = 0;
	}
	else {
		if (utimeval_is_overflow(&ut->total_time)) {
			total = 0;
		}
		else {
			total = utimeval_to_msec(&ut->total_time);
			total = total % delay;
		}
	}
	msec_to_utimeval(&ut->total_time, total);

	return 0;
}

static uint32_t
utimer_sleep_time_with_curtime(struct utimer *ut, struct utimeval *cur)
{
	struct utimeval tv;

	if (ut == NULL || cur == NULL) {
		return 0;
	}
	tv = utimer_calc_next_time(ut, cur);
	tv = utimeval_minus(&tv, cur);
	if (utimeval_less_than_zero(&tv)) {
		return 0;
	}

	return utimeval_to_msec(&tv);
}

static uint32_t
utimer_sleep_time(struct utimer *ut)
{
	struct utimeval cur;

	if (ut == NULL) {
		return 0;
	}

	cur = utimer_current_time();
	return utimer_sleep_time_with_curtime(ut, &cur);
}

static int
utimer_init(struct utimer *ut, uint32_t delay_ms)
{
	if (ut == NULL) {
		return -1;
	}
	utimer_clear(ut);
	msec_to_utimeval(&ut->delay_ms, delay_ms);

	return 0;
}

static uint32_t
utimer_get_total_time(struct utimer *ut)
{
	return utimeval_to_msec(&ut->total_time);
}

static uint32_t
utimer_get_delay(struct utimer *ut)
{
	if (ut == NULL) {
		return -1;
	}
	return utimeval_to_msec(&ut->delay_ms);
}

static int
utimer_set_delay(struct utimer *ut, uint32_t delay_ms)
{
	return utimer_init(ut, delay_ms);
}

static void
utimer_lib_init(void (*func)(int *sec, int *msec))
{
	current_time = func;
}

struct yas_cdriver {
	uint8_t raw_calreg[9];
	int8_t roughoffset_is_set;
	int16_t matrix[9];
	int16_t correction_m[9];
	int8_t temp_coeff[3];
	int8_t initialized;
	int16_t temperature;
	struct yas_machdep_func func;
};

static struct yas_cdriver cdriver = {
	{0, 0, 0, 0, 0, 0, 0, 0, 0},
	0,
	{0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0},
	0,
	-1,
	{NULL, NULL, NULL, NULL, NULL}
};


static int
i2c_open(void)
{
	if (cdriver.func.i2c_open == NULL) {
		return -1;
	}
	return cdriver.func.i2c_open();
}

static int
i2c_close(void)
{
	if (cdriver.func.i2c_close == NULL) {
		return -1;
	}
	return cdriver.func.i2c_close();
}

static int
i2c_write(const uint8_t *buf, int len)
{
	if (cdriver.func.i2c_write == NULL) {
		return -1;
	}
	return cdriver.func.i2c_write(YAS_MAG_I2C_SLAVEADDR, buf, len);
}

static int
i2c_read(uint8_t *buf, int len)
{
	if (cdriver.func.i2c_read == NULL) {
		return -1;
	}
	return cdriver.func.i2c_read(YAS_MAG_I2C_SLAVEADDR, buf, len);
}

static void
sleep(int millisec)
{
	if (cdriver.func.msleep == NULL) {
		return;
	}
	cdriver.func.msleep(millisec);
}

/*------------------------------------------------------------------------------
  Compact Driver Measure Functions
  -----------------------------------------------------------------------------*/

static int
yas_cdrv_check_transformation_matrix(const int8_t *p)
{
	int i, j;
	int n;

	for (i = 0; i < 3; ++i) {
		n = 0;
		for (j = 0; j < 3; ++j) {
			if (p[i*3 + j] == 1 || p[i*3 + j] == -1) {
				n++;
			}
		}
		if (n != 1) {
			return -1;
		}
	}

	for (i = 0; i < 3; ++i) {
		n = 0;
		for (j = 0; j < 3; ++j) {
			if (p[j*3 + i] == 1 || p[j*3 + i] == -1) {
				n++;
			}
		}
		if (n != 1) {
			return -1;
		}
	}

	return YAS_NO_ERROR;
}

static int
yas_cdrv_make_correction_matrix(const int8_t *p, const int16_t *matrix,
		int16_t *ans)
{
	int i, j, k;
	int16_t temp16;

	for (i = 0; i < 3; ++i) {
		for (j = 0; j < 3; ++j) {
			temp16 = 0;
			for (k = 0; k < 3; ++k) {
				temp16 += p[i*3 + k] * matrix[k*3 + j];
			}
			ans[i*3 + j] = temp16;
		}
	}

	return YAS_NO_ERROR;
}

static int
yas_cdrv_transform(const int16_t *matrix, const int32_t *raw, int32_t *data)
{
	int i, j;
	int32_t temp;

	for (i = 0; i < 3; ++i) {
		temp = 0;
		for (j = 0; j < 3; ++j) {
			temp += (int32_t)matrix[i*3 + j] * raw[j];
		}
		data[i] = temp;
	}

	return YAS_NO_ERROR;
}

static int
yas_cdrv_msens_correction(const int32_t *raw, uint16_t temperature,
		int32_t *data)
{
	static const int16_t center16[3] = {
		16 * YAS_CDRV_CENTER_X,
		16 * YAS_CDRV_CENTER_Y1,
		16 * YAS_CDRV_CENTER_Y2
	};
	int32_t temp_rawdata[3];
	int32_t raw_xyz[3];
	int32_t temps32;
	int i;

	for (i = 0; i < 3; i++) {
		temp_rawdata[i] = raw[i];
	}

	for (i = 0; i < 3; ++i) {
		temp_rawdata[i] <<= 4;
		temp_rawdata[i] -= center16[i];

		/*
Memo:
The number '3 / 100' is approximated to '0x7ae1 / 2^20'.
		 */

		temps32 = ((int32_t)temperature - YAS_CDRV_CENTER_T) *  cdriver.temp_coeff[i] * 0x7ae1;
		if (temps32 >= 0) {
			temp_rawdata[i] -= (int16_t)(temps32 >> 16);
		}
		else {
			temp_rawdata[i] += (int16_t)((-temps32) >> 16);
		}
	}

	raw_xyz[0] = - temp_rawdata[0];
	raw_xyz[1] = temp_rawdata[2] - temp_rawdata[1];
	raw_xyz[2] = temp_rawdata[2] + temp_rawdata[1];

	yas_cdrv_transform(cdriver.correction_m, raw_xyz, data);

	for (i = 0; i < 3; ++i) {
		data[i] /= 1600;
	}

	return YAS_NO_ERROR;
}

static int
yas_cdrv_actuate_initcoil(void)
{
	int i;
	static const uint8_t InitCoilTable[16] = {
		0x90, 0x81, 0x91, 0x82, 0x92, 0x83, 0x93, 0x84,
		0x94, 0x85, 0x95, 0x86, 0x96, 0x87, 0x97, 0x80
	};

	if (!cdriver.initialized) {
		return YAS_ERROR_NOT_INITIALIZED;
	}

	for (i = 0; i < 16; ++i) {
		if (i2c_write(&InitCoilTable[i], 1) < 0) {
			return YAS_ERROR_I2C;
		}
	}

	return YAS_NO_ERROR;
}

static int
yas_cdrv_measure_offset(int8_t *offset)
{
	int i;
	uint8_t dat;
	uint8_t buf[6];
	int rv = YAS_NO_ERROR;

	if (offset == NULL) {
		return YAS_ERROR_ARG;
	}

	if (!cdriver.initialized) {
		return YAS_ERROR_NOT_INITIALIZED;
	}

	dat = MS3CDRV_RDSEL_MEASURE;
	if (i2c_write(&dat, 1) < 0) {
		return YAS_ERROR_I2C;
	}

	dat = MS3CDRV_CMD_MEASURE_HARDOFFSET;
	if (i2c_write(&dat, 1) < 0) {
		return YAS_ERROR_I2C;
	}

	sleep(MS3CDRV_WAIT_MEASURE_HARDOFFSET);

	if (i2c_read(buf, 6) < 0) {
		return YAS_ERROR_I2C;
	}

	if (buf[0] & 0x80) {
		return YAS_ERROR_BUSY;
	}

	for (i = 0; i < 3; ++i) {
		offset[2 - i] = ((buf[i << 1] & 0x7) << 8) | buf[(i << 1) | 1];
	}

	if (offset[0] <= YAS_CDRV_HARDOFFSET_MEASURE_UF_VALUE
			|| offset[0] >= YAS_CDRV_HARDOFFSET_MEASURE_OF_VALUE) {
		rv |= YAS_CDRV_MEASURE_X_OFUF;
	}
	if (offset[1] <= YAS_CDRV_HARDOFFSET_MEASURE_UF_VALUE
			|| offset[1] >= YAS_CDRV_HARDOFFSET_MEASURE_OF_VALUE) {
		rv |= YAS_CDRV_MEASURE_Y1_OFUF;
	}
	if (offset[2] <= YAS_CDRV_HARDOFFSET_MEASURE_UF_VALUE
			|| offset[2] >= YAS_CDRV_HARDOFFSET_MEASURE_OF_VALUE) {
		rv |= YAS_CDRV_MEASURE_Y2_OFUF;
	}

	return rv;
}

static int
yas_cdrv_set_offset(const int8_t *offset)
{
	int i;
	uint8_t dat;
	static const uint8_t addr[3] = { 0x20, 0x40, 0x60};
	uint8_t tmp[3];
	int rv = YAS_NO_ERROR;

	if (offset == NULL) {
		return YAS_ERROR_ARG;
	}
	if (offset[0] > 32 || offset[1] > 32 || offset[2] > 32) {
		return YAS_ERROR_ARG;
	}

	if (!cdriver.initialized) {
		return YAS_ERROR_NOT_INITIALIZED;
	}

	for (i = 0; i < 3; i++) {
		tmp[i] = offset[i];
	}

	for (i = 0; i < 3; ++i) {
		if (tmp[i] <= 5) {
			tmp[i] = 0;
		}
		else {
			tmp[i] -= 5;
		}
	}
	for (i = 0; i < 3; ++i) {
		dat = addr[i] | tmp[i];
		if (i2c_write(&dat, 1) < 0) {
			return YAS_ERROR_I2C;
		}
	}

	cdriver.roughoffset_is_set = 1;

	return rv;
}

static int
yas_cdrv_measure_preparation(void)
{
	uint8_t dat;

	dat = MS3CDRV_RDSEL_MEASURE;
	if (i2c_write(&dat, 1) < 0) {
		return YAS_ERROR_I2C;
	}

	return YAS_NO_ERROR;
}

static int
yas_cdrv_measure_sub(int32_t *msens, int32_t *raw, int16_t *t)
{
	uint8_t dat;
	uint8_t buf[8];
	uint8_t rv = YAS_NO_ERROR;
	int32_t temp_msens[3];
	int i;

	if (cdriver.roughoffset_is_set == 0) {
		return YAS_ERROR_HARDOFFSET_NOT_WRITTEN;
	}

	dat = MS3CDRV_CMD_MEASURE_XY1Y2T;
	if (i2c_write(&dat, 1) < 0) {
		return YAS_ERROR_I2C;
	}

	for (i = 0; i < MS3CDRV_WAIT_MEASURE_XY1Y2T; i++) {
		sleep(1);

		if (i2c_read(buf, 8) < 0) {
			return YAS_ERROR_I2C;
		}
		if (!(buf[0] & 0x80)) {
			break;
		}
	}

	if (buf[0] & 0x80) {
		return YAS_ERROR_BUSY;
	}

	for (i = 0; i < 3; ++i) {
		temp_msens[2 - i] = ((buf[i << 1] & 0x7) << 8) + buf[(i << 1) | 1];
	}
	cdriver.temperature = ((buf[6] & 0x7) << 8) + buf[7];

	if (temp_msens[0] <= YAS_CDRV_NORMAL_MEASURE_UF_VALUE
			|| temp_msens[0] >= YAS_CDRV_NORMAL_MEASURE_OF_VALUE) {
		rv |= YAS_CDRV_MEASURE_X_OFUF;
	}
	if (temp_msens[1] <= YAS_CDRV_NORMAL_MEASURE_UF_VALUE
			|| temp_msens[1] >= YAS_CDRV_NORMAL_MEASURE_OF_VALUE) {
		rv |= YAS_CDRV_MEASURE_Y1_OFUF;
	}
	if (temp_msens[2] <= YAS_CDRV_NORMAL_MEASURE_UF_VALUE
			|| temp_msens[2] >= YAS_CDRV_NORMAL_MEASURE_OF_VALUE) {
		rv |= YAS_CDRV_MEASURE_Y2_OFUF;
	}

	yas_cdrv_msens_correction(temp_msens, cdriver.temperature, msens);

	if (raw != NULL) {
		for (i = 0; i < 3; i++) {
			raw[i] = temp_msens[i];
		}
	}
	if (t != NULL) {
		*t = cdriver.temperature;
	}

	return (int)rv;
}

static int
yas_cdrv_measure(int32_t *msens, int32_t *raw, int16_t *t)
{
	int rv, i;

	if (!cdriver.initialized) {
		return YAS_ERROR_NOT_INITIALIZED;
	}

	if (msens == NULL) {
		return YAS_ERROR_ARG;
	}

	rv = yas_cdrv_measure_preparation();
	if (rv < 0) {
		return rv;
	}

	rv = yas_cdrv_measure_sub(msens, raw, t);
	for (i = 0; i < 3; i++) {
		msens[i] *= 400; /* typically msens * 400 is nT in unit */
	}

	return rv;
}

static int
yas_cdrv_recalc_calib_offset(int32_t *prev_calib_offset,
		int32_t *new_calib_offset,
		int8_t *prev_offset,
		int8_t *new_offset)
{
	int32_t tmp[3], resolution[9], base[3];
	int32_t raw[3];
	int32_t diff, i;

	if (!cdriver.initialized) {
		return YAS_ERROR_NOT_INITIALIZED;
	}

	if (prev_calib_offset == NULL || new_calib_offset == NULL
			|| prev_offset == NULL || new_offset == NULL) {
		return YAS_ERROR_ARG;
	}

	raw[0] = raw[1] = raw[2] = 0;
	yas_cdrv_msens_correction(raw, cdriver.temperature, base);
	for (i = 0; i < 3; i++) {
		raw[0] = raw[1] = raw[2] = 0;
		raw[i] = 226;
		yas_cdrv_msens_correction(raw, cdriver.temperature, &resolution[i*3]);
		resolution[i*3 + 0] -= base[0];
		resolution[i*3 + 1] -= base[1];
		resolution[i*3 + 2] -= base[2];
	}

	for (i = 0; i < 3; i++) {
		tmp[i] = prev_calib_offset[i] / 400; /* nT -> count */
	}
	for (i = 0; i < 3; i++) {
		diff = (int32_t)new_offset[i] - (int32_t)prev_offset[i];
		while (diff > 0) {
			tmp[0] -= resolution[i*3 + 0];
			tmp[1] -= resolution[i*3 + 1];
			tmp[2] -= resolution[i*3 + 2];
			diff--;
		}
		while (diff < 0) {
			tmp[0] += resolution[i*3 + 0];
			tmp[1] += resolution[i*3 + 1];
			tmp[2] += resolution[i*3 + 2];
			diff++;
		}
	}
	for (i = 0; i < 3; i++) {
		new_calib_offset[i] = tmp[i] * 400; /* count -> nT */
	}

	return YAS_NO_ERROR;
}

static int
yas_cdrv_set_transformatiom_matrix(const int8_t *transform)
{
	if (transform == NULL) {
		return YAS_ERROR_ARG;
	}

	if (!cdriver.initialized) {
		return YAS_ERROR_NOT_INITIALIZED;
	}

	if (yas_cdrv_check_transformation_matrix(transform) < 0) {
		return YAS_ERROR_ARG;
	}

	yas_cdrv_make_correction_matrix(transform, cdriver.matrix, cdriver.correction_m);

	return YAS_NO_ERROR;
}

static int
yas_cdrv_init(const int8_t *transform, struct yas_machdep_func *func)
{
	int i;
	uint8_t dat;
	uint8_t *buf = cdriver.raw_calreg;
	uint8_t tempu8;

	if (transform == NULL || func == NULL) {
		return YAS_ERROR_ARG;
	}
	cdriver.func = *func;

	if (yas_cdrv_check_transformation_matrix(transform) < 0) {
		return YAS_ERROR_ARG;
	}

	cdriver.roughoffset_is_set = 0;

	if (!cdriver.initialized) {
		if (i2c_open() < 0) {
			return YAS_ERROR_I2C;
		}
	}

	/* preparation to read CAL register */
	dat = MS3CDRV_CMD_MEASURE_HARDOFFSET;
	if (i2c_write(&dat, 1) < 0) {
		i2c_close();
		return YAS_ERROR_I2C;
	}
	sleep(MS3CDRV_WAIT_MEASURE_HARDOFFSET);

	dat = MS3CDRV_RDSEL_CALREGISTER;
	if (i2c_write(&dat, 1) < 0) {
		i2c_close();
		return YAS_ERROR_I2C;
	}

	/* dummy read */
	if (i2c_read(buf, 9) < 0) {
		i2c_close();
		return YAS_ERROR_I2C;
	}

	if (i2c_read(buf, 9) < 0) {
		i2c_close();
		return YAS_ERROR_I2C;
	}

	cdriver.matrix[0] = 100;

	tempu8 = (buf[0] & 0xfc) >> 2;
	cdriver.matrix[1] = tempu8 - 0x20;

	tempu8 = ((buf[0] & 0x3) << 2) | ((buf[1] & 0xc0) >> 6);
	cdriver.matrix[2] = tempu8 - 8;

	tempu8 = buf[1] & 0x3f;
	cdriver.matrix[3] = tempu8 - 0x20;

	tempu8 = (buf[2] & 0xfc) >> 2;
	cdriver.matrix[4] = tempu8 + 38;

	tempu8 = ((buf[2] & 0x3) << 4) | (buf[3] & 0xf0) >> 4;
	cdriver.matrix[5] = tempu8 - 0x20;

	tempu8 = ((buf[3] & 0xf) << 2) | ((buf[4] & 0xc0) >> 6);
	cdriver.matrix[6] = tempu8 - 0x20;

	tempu8 = buf[4] & 0x3f;
	cdriver.matrix[7] = tempu8 - 0x20;

	tempu8 = (buf[5] & 0xfe) >> 1;
	cdriver.matrix[8] = tempu8 + 66;

	yas_cdrv_make_correction_matrix(transform, cdriver.matrix, cdriver.correction_m);

	for (i = 0; i < 3; ++i) {
		cdriver.temp_coeff[i] = (int8_t)((int16_t)buf[i + 6] - 0x80);
	}

	cdriver.initialized = 1;

	return YAS_NO_ERROR;
}

static int
yas_cdrv_term(void)
{
	if (!cdriver.initialized) {
		return YAS_ERROR_NOT_INITIALIZED;
	}
	cdriver.initialized = 0;

	if (i2c_close() < 0) {
		return YAS_ERROR_I2C;
	}

	return YAS_NO_ERROR;
}

#define MEASURE_RESULT_OVER_UNDER_FLOW  (0x07)

#define YAS_DEFAULT_CALIB_INTERVAL     (50)    /* 50 msecs */
#define YAS_DEFAULT_DATA_INTERVAL      (200)   /* 200 msecs */
#define YAS_INITCOIL_INTERVAL          (3000)  /* 3 seconds */
#define YAS_INITCOIL_GIVEUP_INTERVAL   (180000) /* 180 seconds */
#define YAS_DETECT_OVERFLOW_INTERVAL   (0)     /* 0 second */

#define YAS_MAG_ERROR_DELAY            (200)
#define YAS_MAG_STATE_NORMAL           (0)
#define YAS_MAG_STATE_INIT_COIL        (1)
#define YAS_MAG_STATE_MEASURE_OFFSET   (2)

static const int8_t YAS_TRANSFORMATION[][9] = {
	{ 0, 1, 0,-1, 0, 0, 0, 0, 1 },
	{-1, 0, 0, 0,-1, 0, 0, 0, 1 },
	{ 0,-1, 0, 1, 0, 0, 0, 0, 1 },
	{ 1, 0, 0, 0, 1, 0, 0, 0, 1 },
	{ 0,-1, 0,-1, 0, 0, 0, 0,-1 },
	{ 1, 0, 0, 0,-1, 0, 0, 0,-1 },
	{ 0, 1, 0, 1, 0, 0, 0, 0,-1 },
	{-1, 0, 0, 0, 1, 0, 0, 0,-1 },
};

static const int supported_data_interval[] = {20, 50, 60, 100, 200, 1000};
static const int supported_calib_interval[] = {60, 50, 60, 50, 50, 50};
static const int32_t INVALID_CALIB_OFFSET[] = {0x7fffffff, 0x7fffffff, 0x7fffffff};
static const int8_t INVALID_OFFSET[] = {0x7f, 0x7f, 0x7f};


struct yas_adaptive_filter {
	int num;
	int index;
	int filter_len;
	int filter_noise;
	int32_t sequence[YAS_MAG_MAX_FILTER_LEN];
};

struct yas_thresh_filter {
	int32_t threshold;
	int32_t last;
};

struct yas_driver {
	int initialized;
	struct yas_mag_driver_callback callback;
	struct utimer data_timer;
	struct utimer initcoil_timer;
	struct utimer initcoil_giveup_timer;
	struct utimer detect_overflow_timer;
	int32_t prev_mag[3];
	int32_t prev_xy1y2[3];
	int32_t prev_mag_w_offset[3];
	int16_t prev_temperature;
	int measure_state;
	int active;
	int overflow;
	int initcoil_gaveup;
	int position;
	int delay_timer_use_data;
	int delay_timer_interval;
	int delay_timer_counter;
	int filter_enable;
	int filter_len;
	int filter_thresh;
	int filter_noise[3];
	struct yas_adaptive_filter adap_filter[3];
	struct yas_thresh_filter thresh_filter[3];
	struct yas_mag_offset offset;
};

static struct yas_driver this_driver;

static int
lock(void)
{
	if (this_driver.callback.lock != NULL) {
		if (this_driver.callback.lock() < 0) {
			return YAS_ERROR_RESTARTSYS;
		}
	}
	return 0;
}

static int
unlock(void)
{
	if (this_driver.callback.unlock != NULL) {
		if (this_driver.callback.unlock() < 0) {
			return YAS_ERROR_RESTARTSYS;
		}
	}
	return 0;
}

static int32_t
square(int32_t data)
{
	return data * data;
}

static void
adaptive_filter_init(struct yas_adaptive_filter *adap_filter, int len, int noise)
{
	int i;

	adap_filter->num = 0;
	adap_filter->index = 0;
	adap_filter->filter_noise = noise;
	adap_filter->filter_len = len;

	for (i = 0; i < adap_filter->filter_len; ++i) {
		adap_filter->sequence[i] = 0;
	}
}

static int32_t
adaptive_filter_filter(struct yas_adaptive_filter *adap_filter, int32_t in)
{
	int32_t avg, sum;
	int i;

	if (adap_filter->filter_len == 0) {
		return in;
	}
	if (adap_filter->num < adap_filter->filter_len) {
		adap_filter->sequence[adap_filter->index++] = in / 100;
		adap_filter->num++;
		return in;
	}
	if (adap_filter->filter_len <= adap_filter->index) {
		adap_filter->index = 0;
	}
	adap_filter->sequence[adap_filter->index++] = in / 100;

	avg = 0;
	for (i = 0; i < adap_filter->filter_len; i++) {
		avg += adap_filter->sequence[i];
	}
	avg /= adap_filter->filter_len;

	sum = 0;
	for (i = 0; i < adap_filter->filter_len; i++) {
		sum += square(avg - adap_filter->sequence[i]);
	}
	sum /= adap_filter->filter_len;

	if (sum <= adap_filter->filter_noise) {
		return avg * 100;
	}

	return ((in/100 - avg) * (sum - adap_filter->filter_noise) / sum + avg) * 100;
}

static void
thresh_filter_init(struct yas_thresh_filter *thresh_filter, int threshold)
{
	thresh_filter->threshold = threshold;
	thresh_filter->last = 0;
}

static int32_t
thresh_filter_filter(struct yas_thresh_filter *thresh_filter, int32_t in)
{
	if (in < thresh_filter->last - thresh_filter->threshold
			|| thresh_filter->last + thresh_filter->threshold < in) {
		thresh_filter->last = in;
		return in;
	}
	else {
		return thresh_filter->last;
	}
}

static void
filter_init(struct yas_driver *d)
{
	int i;

	for (i = 0; i < 3; i++) {
		adaptive_filter_init(&d->adap_filter[i], d->filter_len, d->filter_noise[i]);
		thresh_filter_init(&d->thresh_filter[i], d->filter_thresh);
	}
}

static void
filter_filter(struct yas_driver *d, int32_t *orig, int32_t *filtered)
{
	int i;

	for (i = 0; i < 3; i++) {
		filtered[i] = adaptive_filter_filter(&d->adap_filter[i], orig[i]);
		filtered[i] = thresh_filter_filter(&d->thresh_filter[i], filtered[i]);
	}
}

static int
is_valid_offset(const int8_t *p)
{
	return (p != NULL
			&& (p[0] < 33) && (p[1] < 33) && (p[2] < 33)
			&& (0 <= p[0] && 0 <= p[1] && 0 <= p[2]));
}

static int
is_valid_calib_offset(const int32_t *p)
{
	int i;
	for (i = 0; i < 3; i++) {
		if (p[i] != INVALID_CALIB_OFFSET[i]) {
			return 1;
		}
	}
	return 0;
}

static int
is_offset_differ(const int8_t *p0, const int8_t *p1)
{
	return (p0[0] != p1[0] || p0[1] != p1[1] || p0[2] != p1[2]);
}

static int
is_calib_offset_differ(const int32_t *p0, const int32_t *p1)
{
	return (p0[0] != p1[0] || p0[1] != p1[1] || p0[2] != p1[2]);
}

static int
get_overflow(struct yas_driver *d)
{
	return d->overflow;
}

static void
set_overflow(struct yas_driver *d, const int overflow)
{
	if (d->overflow != overflow) {
		d->overflow = overflow;
	}
}

static int
get_initcoil_gaveup(struct yas_driver *d)
{
	return d->initcoil_gaveup;
}

static void
set_initcoil_gaveup(struct yas_driver *d, const int initcoil_gaveup)
{
	d->initcoil_gaveup = initcoil_gaveup;
}

static int32_t *
get_calib_offset(struct yas_driver *d)
{
	return d->offset.calib_offset.v;
}

static void
set_calib_offset(struct yas_driver *d, const int32_t *offset)
{
	int i;

	if (is_calib_offset_differ(d->offset.calib_offset.v, offset)) {
		for (i = 0; i < 3; i++) {
			d->offset.calib_offset.v[i] = offset[i];
		}
	}
}

static int8_t *
get_offset(struct yas_driver *d)
{
	return d->offset.hard_offset;
}

static void
set_offset(struct yas_driver *d, const int8_t *offset)
{
	int i;

	if (is_offset_differ(d->offset.hard_offset, offset)) {
		for (i = 0; i < 3; i++) {
			d->offset.hard_offset[i] = offset[i];
		}
	}
}

static int
get_active(struct yas_driver *d)
{
	return d->active;
}

static void
set_active(struct yas_driver *d, const int active)
{
	d->active = active;
}

static int
get_position(struct yas_driver *d)
{
	return d->position;
}

static void
set_position(struct yas_driver *d, const int position)
{
	d->position = position;
}

static int
get_measure_state(struct yas_driver *d)
{
	return d->measure_state;
}

static void
set_measure_state(struct yas_driver *d, const int state)
{
	YLOGI((KERN_INFO"state(%d)\n", state));
	d->measure_state = state;
}

static struct utimer *
get_data_timer(struct yas_driver *d)
{
	return &d->data_timer;
}

static struct utimer *
get_initcoil_timer(struct yas_driver *d)
{
	return &d->initcoil_timer;
}

static struct utimer *
get_initcoil_giveup_timer(struct yas_driver *d)
{
	return &d->initcoil_giveup_timer;
}

static struct utimer *
get_detect_overflow_timer(struct yas_driver *d)
{
	return &d->detect_overflow_timer;
}

static int
get_delay_timer_use_data(struct yas_driver *d)
{
	return d->delay_timer_use_data;
}

static void
set_delay_timer_use_data(struct yas_driver *d, int flag)
{
	d->delay_timer_use_data = !!flag;
}

static int
get_delay_timer_interval(struct yas_driver *d)
{
	return d->delay_timer_interval;
}

static void
set_delay_timer_interval(struct yas_driver *d, int interval)
{
	d->delay_timer_interval = interval;
}

static int
get_delay_timer_counter(struct yas_driver *d)
{
	return d->delay_timer_counter;
}

static void
set_delay_timer_counter(struct yas_driver *d, int counter)
{
	d->delay_timer_counter = counter;
}

static int
get_filter_enable(struct yas_driver *d)
{
	return d->filter_enable;
}

static void
set_filter_enable(struct yas_driver *d, int enable)
{
	if (enable) {
		filter_init(d);
	}
	d->filter_enable = !!enable;
}

static int
get_filter_len(struct yas_driver *d)
{
	return d->filter_len;
}

static void
set_filter_len(struct yas_driver *d, int len)
{
	if (len < 0) {
		return;
	}
	if (len > YAS_MAG_MAX_FILTER_LEN) {
		return;
	}
	d->filter_len = len;
	filter_init(d);
}

static int
get_filter_noise(struct yas_driver *d, int *noise)
{
	int i;

	for (i = 0; i < 3; i++) {
		noise[i] = d->filter_noise[i];
	}
	return 0;
}

static void
set_filter_noise(struct yas_driver *d, int *noise)
{
	int i;

	if (noise == NULL) {
		return;
	}
	for (i = 0; i < 3; i++) {
		if (noise[i] < 0) {
			noise[i] = 0;
		}
		d->filter_noise[i] = noise[i];
	}
	filter_init(d);
}

static int
get_filter_thresh(struct yas_driver *d)
{
	return d->filter_thresh;
}

static void
set_filter_thresh(struct yas_driver *d, int threshold)
{
	if (threshold < 0) {
		return;
	}
	d->filter_thresh = threshold;
	filter_init(d);
}

static int32_t*
get_previous_mag(struct yas_driver *d)
{
	return d->prev_mag;
}

static void
set_previous_mag(struct yas_driver *d, int32_t *data)
{
	int i;
	for (i = 0; i < 3; i++) {
		d->prev_mag[i] = data[i];
	}
}

static int32_t*
get_previous_xy1y2(struct yas_driver *d)
{
	return d->prev_xy1y2;
}

static void
set_previous_xy1y2(struct yas_driver *d, int32_t *data)
{
	int i;
	for (i = 0; i < 3; i++) {
		d->prev_xy1y2[i] = data[i];
	}
}

static int32_t*
get_previous_mag_w_offset(struct yas_driver *d)
{
	return d->prev_mag_w_offset;
}

static void
set_previous_mag_w_offset(struct yas_driver *d, int32_t *data)
{
	int i;
	for (i = 0; i < 3; i++) {
		d->prev_mag_w_offset[i] = data[i];
	}
}

static int16_t
get_previous_temperature(struct yas_driver *d)
{
	return d->prev_temperature;
}

static void
set_previous_temperature(struct yas_driver *d, int16_t temperature)
{
	d->prev_temperature = temperature;
}

static int
init_coil(struct yas_driver *d)
{
	int rt;

	YLOGD((KERN_DEBUG"init_coil IN\n"));

	utimer_update(get_initcoil_timer(d));
	if (!get_initcoil_gaveup(d)) {
		utimer_update(get_initcoil_giveup_timer(d));
		if (utimer_is_timeout(get_initcoil_giveup_timer(d))) {
			utimer_clear_timeout(get_initcoil_giveup_timer(d));
			set_initcoil_gaveup(d, TRUE);
		}
	}
	if (utimer_is_timeout(get_initcoil_timer(d)) && !get_initcoil_gaveup(d)) {
		utimer_clear_timeout(get_initcoil_timer(d));
		YLOGI((KERN_INFO"init_coil!\n"));
		if ((rt = yas_cdrv_actuate_initcoil()) < 0) {
			YLOGE((KERN_ERR"yas_cdrv_actuate_initcoil failed[%d]\n", rt));
			return rt;
		}
		if (get_overflow(d) || !is_valid_offset(get_offset(d))) {
			set_measure_state(d, YAS_MAG_STATE_MEASURE_OFFSET);
		}
		else {
			set_measure_state(d, YAS_MAG_STATE_NORMAL);
		}
	}

	YLOGD((KERN_DEBUG"init_coil OUT\n"));

	return 0;
}

static int
measure_offset(struct yas_driver *d)
{
	int8_t offset[3];
	int32_t moffset[3];
	int rt, result = 0, i;

	YLOGI((KERN_INFO"measure_offset IN\n"));

	if ((rt = yas_cdrv_measure_offset(offset)) < 0) {
		YLOGE((KERN_ERR"yas_cdrv_measure_offset failed[%d]\n", rt));
		return rt;
	}

	YLOGI((KERN_INFO"rough offset[%d][%d][%d]\n", offset[0], offset[1], offset[2]));

	if (rt & MEASURE_RESULT_OVER_UNDER_FLOW) {
		YLOGI((KERN_INFO"yas_cdrv_measure_offset overflow x[%d] y[%d] z[%d]\n",
					!!(rt&0x01), !!(rt&0x02), !!(rt&0x04)));

		set_overflow(d, TRUE);
		set_measure_state(d, YAS_MAG_STATE_INIT_COIL);
		return YAS_REPORT_OVERFLOW_OCCURED;
	}

	for (i = 0; i < 3; i++) {
		moffset[i] = get_calib_offset(d)[i];
	}
	if (is_offset_differ(get_offset(d), offset)) {
		if (is_valid_offset(get_offset(d))
				&& is_valid_calib_offset(get_calib_offset(d))) {
			yas_cdrv_recalc_calib_offset(get_calib_offset(d),
					moffset,
					get_offset(d),
					offset);
			result |= YAS_REPORT_CALIB_OFFSET_CHANGED;
		}
	}

	if ((rt = yas_cdrv_set_offset(offset)) < 0) {
		YLOGE((KERN_ERR"yas_cdrv_set_offset failed[%d]\n", rt));
		return rt;
	}

	result |= YAS_REPORT_HARD_OFFSET_CHANGED;
	set_offset(d, offset);
	if (is_valid_calib_offset(moffset)) {
		set_calib_offset(d, moffset);
	}
	set_measure_state(d, YAS_MAG_STATE_NORMAL);

	YLOGI((KERN_INFO"measure_offset OUT\n"));

	return result;
}

static int
measure_msensor_normal(struct yas_driver *d, int32_t *magnetic,
		int32_t *mag_w_offset, int32_t *xy1y2, int16_t *temperature)
{
	int rt = 0, result, i;
	int32_t filtered[3];

	YLOGD((KERN_DEBUG"measure_msensor_normal IN\n"));

	result = 0;
	if ((rt = yas_cdrv_measure(mag_w_offset, xy1y2, temperature)) < 0) {
		YLOGE((KERN_ERR"yas_cdrv_measure failed[%d]\n", rt));
		return rt;
	}
	if (rt & MEASURE_RESULT_OVER_UNDER_FLOW) {
		YLOGI((KERN_INFO"yas_cdrv_measure overflow x[%d] y[%d] z[%d]\n",
					!!(rt&0x01), !!(rt&0x02), !!(rt&0x04)));
		utimer_update(get_detect_overflow_timer(d));
		set_overflow(d, TRUE);
		if (utimer_is_timeout(get_detect_overflow_timer(d))) {
			utimer_clear_timeout(get_detect_overflow_timer(d));
			result |= YAS_REPORT_OVERFLOW_OCCURED;
		}
		if (get_measure_state(d) == YAS_MAG_STATE_NORMAL) {
			set_measure_state(d, YAS_MAG_STATE_INIT_COIL);
		}
	}
	else {
		utimer_clear(get_detect_overflow_timer(d));
		set_overflow(d, FALSE);
		if (get_measure_state(d) == YAS_MAG_STATE_NORMAL) {
			utimer_clear(get_initcoil_timer(d));
			utimer_clear(get_initcoil_giveup_timer(d));
		}
	}
	if (get_filter_enable(d)) {
		filter_filter(d, mag_w_offset, filtered);
	}

	if (is_valid_calib_offset(get_calib_offset(d))) {
		for (i = 0; i < 3; i++) {
			magnetic[i] = get_filter_enable(d)
				? filtered[i] - get_calib_offset(d)[i]
				: mag_w_offset[i] - get_calib_offset(d)[i];
		}
	}
	else {
		for (i = 0; i < 3; i++) {
			magnetic[i] = get_filter_enable(d) ? filtered[i] : mag_w_offset[i];
		}
	}

	YLOGD((KERN_DEBUG"measure_msensor_normal OUT\n"));

	return result;
}

static int
measure_msensor(struct yas_driver *d, int32_t *magnetic, int32_t *mag_w_offset,
		int32_t *xy1y2, int16_t *temperature)
{
	int result, i;

	YLOGD((KERN_DEBUG"measure_msensor IN\n"));

	for (i = 0; i < 3; i++) {
		magnetic[i] = get_previous_mag(d)[i];
		mag_w_offset[i] = get_previous_mag_w_offset(d)[i];
		xy1y2[i] = get_previous_xy1y2(d)[i];
		*temperature = get_previous_temperature(d);
	}

	result = 0;
	switch (get_measure_state(d)) {
		case YAS_MAG_STATE_INIT_COIL:
			result = init_coil(d);
			break;
		case YAS_MAG_STATE_MEASURE_OFFSET:
			result = measure_offset(d);
			break;
		case YAS_MAG_STATE_NORMAL:
			result = 0;
			break;
		default:
			result = -1;
			break;
	}

	if (result < 0) {
		return result;
	}

	if (!(result & YAS_REPORT_OVERFLOW_OCCURED)) {
		result |= measure_msensor_normal(d, magnetic, mag_w_offset, xy1y2, temperature);
	}
	set_previous_mag(d, magnetic);
	set_previous_xy1y2(d, xy1y2);
	set_previous_mag_w_offset(d, mag_w_offset);
	set_previous_temperature(d, *temperature);

	YLOGD((KERN_DEBUG"measure_msensor OUT\n"));

	return result;
}

static int
measure(struct yas_driver *d, int32_t *magnetic, int32_t *mag_w_offset,
		int32_t *xy1y2, int16_t *temperature, uint32_t *time_delay)
{
	int result;
	int counter;
	uint32_t total = 0;

	YLOGD((KERN_DEBUG"measure IN\n"));

	utimer_update(get_data_timer(d));

	if ((result = measure_msensor(d, magnetic, mag_w_offset, xy1y2, temperature)) < 0) {
		return result;
	}

	counter = get_delay_timer_counter(d);
	total = utimer_get_total_time(get_data_timer(d));
	if (utimer_get_delay(get_data_timer(d)) > 0) {
		counter -= total / utimer_get_delay(get_data_timer(d));
	}
	else {
		counter = 0;
	}

	if (utimer_is_timeout(get_data_timer(d))) {
		utimer_clear_timeout(get_data_timer(d));

		if (get_delay_timer_use_data(d)) {
			result |= YAS_REPORT_DATA;
			if (counter <= 0) {
				result |= YAS_REPORT_CALIB;
			}
		}
		else {
			result |= YAS_REPORT_CALIB;
			if (counter <= 0) {
				result |= YAS_REPORT_DATA;
			}
		}
	}

	if (counter <= 0) {
		set_delay_timer_counter(d, get_delay_timer_interval(d));
	}
	else {
		set_delay_timer_counter(d, counter);
	}

	*time_delay = utimer_sleep_time(get_data_timer(d));

	YLOGD((KERN_DEBUG"measure OUT [%d]\n", result));

	return result;
}

static int
resume(struct yas_driver *d)
{
	int32_t zero[] = {0, 0, 0};
	struct yas_machdep_func func;
	int rt;

	YLOGI((KERN_INFO"resume IN\n"));

	func.i2c_open = d->callback.i2c_open;
	func.i2c_close = d->callback.i2c_close;
	func.i2c_write = d->callback.i2c_write;
	func.i2c_read = d->callback.i2c_read;
	func.msleep = d->callback.msleep;

	if ((rt = yas_cdrv_init(YAS_TRANSFORMATION[get_position(d)], &func)) < 0) {
		YLOGE((KERN_ERR"yas_cdrv_init failed[%d]\n", rt));
		return rt;
	}

	utimer_clear(get_data_timer(d));
	utimer_clear(get_initcoil_giveup_timer(d));
	utimer_clear(get_initcoil_timer(d));
	utimer_clear(get_detect_overflow_timer(d));

	set_previous_mag(d, zero);
	set_previous_xy1y2(d, zero);
	set_previous_mag_w_offset(d, zero);
	set_previous_temperature(d, 0);
	set_overflow(d, FALSE);
	set_initcoil_gaveup(d, FALSE);

	filter_init(d);

	if (is_valid_offset(d->offset.hard_offset)) {
		yas_cdrv_set_offset(d->offset.hard_offset);
	}
	else {
		if ((rt = yas_cdrv_actuate_initcoil()) < 0) {
			YLOGE((KERN_ERR"yas_cdrv_actuate_initcoil failed[%d]\n", rt));
			set_measure_state(d, YAS_MAG_STATE_INIT_COIL);
		}
		else {
			set_measure_state(d, YAS_MAG_STATE_MEASURE_OFFSET);
		}
	}

	YLOGI((KERN_INFO"resume OUT\n"));
	return 0;
}

static int
suspend(struct yas_driver *d)
{
	YLOGI((KERN_INFO"suspend IN\n"));

	(void) d;
	yas_cdrv_term();

	YLOGI((KERN_INFO"suspend OUT\n"));
	return 0;
}

static int
check_interval(int ms)
{
	int index;

	if (ms <= supported_data_interval[0]) {
		ms = supported_data_interval[0];
	}
	for (index = 0; index < NELEMS(supported_data_interval); index++) {
		if (ms == supported_data_interval[index]) {
			return index;
		}
	}
	return -1;
}

static int
yas_get_delay_nolock(struct yas_driver *d, int *ms)
{
	if (!d->initialized) {
		return YAS_ERROR_NOT_INITIALIZED;
	}
	if (get_delay_timer_use_data(d)) {
		*ms = utimer_get_delay(get_data_timer(d));
	}
	else {
		*ms = utimer_get_delay(get_data_timer(d)) * get_delay_timer_interval(d);
	}
	return YAS_NO_ERROR;
}

static int
yas_set_delay_nolock(struct yas_driver *d, int ms)
{
	int index;
	uint32_t delay_data, delay_calib;

	if (!d->initialized) {
		return YAS_ERROR_NOT_INITIALIZED;
	}
	if ((index = check_interval(ms)) < 0) {
		return YAS_ERROR_ARG;
	}
	delay_data = supported_data_interval[index];
	delay_calib = supported_calib_interval[index];
	set_delay_timer_use_data(d, delay_data < delay_calib);
	if (delay_data < delay_calib) {
		set_delay_timer_interval(d, delay_calib / delay_data);
		set_delay_timer_counter(d, delay_calib / delay_data);
		utimer_set_delay(get_data_timer(d), supported_data_interval[index]);
	}
	else {
		set_delay_timer_interval(d, delay_data / delay_calib);
		set_delay_timer_counter(d, delay_data / delay_calib);
		utimer_set_delay(get_data_timer(d), supported_calib_interval[index]);
	}

	return YAS_NO_ERROR;
}

static int
yas_get_offset_nolock(struct yas_driver *d, struct yas_mag_offset *offset)
{
	if (!d->initialized) {
		return YAS_ERROR_NOT_INITIALIZED;
	}
	*offset = d->offset;
	return YAS_NO_ERROR;
}

static int
yas_set_offset_nolock(struct yas_driver *d, struct yas_mag_offset *offset)
{
	int32_t zero[] = {0, 0, 0};
	int rt;

	if (!d->initialized) {
		return YAS_ERROR_NOT_INITIALIZED;
	}
	if (!get_active(d)) {
		d->offset = *offset;
		return YAS_NO_ERROR;
	}

	if (!is_valid_offset(offset->hard_offset)
			|| is_offset_differ(offset->hard_offset, d->offset.hard_offset)) {
		filter_init(d);

		utimer_clear(get_data_timer(d));
		utimer_clear(get_initcoil_giveup_timer(d));
		utimer_clear(get_initcoil_timer(d));
		utimer_clear(get_detect_overflow_timer(d));

		set_previous_mag(d, zero);
		set_previous_xy1y2(d, zero);
		set_previous_mag_w_offset(d, zero);
		set_previous_temperature(d, 0);
		set_overflow(d, FALSE);
		set_initcoil_gaveup(d, FALSE);
	}
	d->offset = *offset;

	if (is_valid_offset(d->offset.hard_offset)) {
		yas_cdrv_set_offset(d->offset.hard_offset);
	}
	else {
		if ((rt = yas_cdrv_actuate_initcoil()) < 0) {
			YLOGE((KERN_ERR"yas_cdrv_actuate_initcoil failed[%d]\n", rt));
			set_measure_state(d, YAS_MAG_STATE_INIT_COIL);
		}
		else {
			set_measure_state(d, YAS_MAG_STATE_MEASURE_OFFSET);
		}
	}

	return YAS_NO_ERROR;
}

static int
yas_get_enable_nolock(struct yas_driver *d)
{
	if (!d->initialized) {
		return YAS_ERROR_NOT_INITIALIZED;
	}
	return get_active(d);
}

static int
yas_set_enable_nolock(struct yas_driver *d, int active)
{
	int rt;

	if (!d->initialized) {
		return YAS_ERROR_NOT_INITIALIZED;
	}
	if (active) {
		if (get_active(d)) {
			return YAS_NO_ERROR;
		}
		if ((rt = resume(d)) < 0) {
			return rt;
		}
		set_active(d, TRUE);
	}
	else {
		if (!get_active(d)) {
			return YAS_NO_ERROR;
		}
		if ((rt = suspend(d)) < 0) {
			return rt;
		}
		set_active(d, FALSE);
	}
	return YAS_NO_ERROR;
}

static int
yas_get_filter_nolock(struct yas_driver *d, struct yas_mag_filter *filter)
{
	if (!d->initialized) {
		return YAS_ERROR_NOT_INITIALIZED;
	}
	filter->len = get_filter_len(d);
	get_filter_noise(d, filter->noise);
	filter->threshold = get_filter_thresh(d);
	return YAS_NO_ERROR;
}

static int
yas_set_filter_nolock(struct yas_driver *d, struct yas_mag_filter *filter)
{
	if (!d->initialized) {
		return YAS_ERROR_NOT_INITIALIZED;
	}
	set_filter_len(d, filter->len);
	set_filter_noise(d, filter->noise);
	set_filter_thresh(d, filter->threshold);
	return YAS_NO_ERROR;
}

static int
yas_get_filter_enable_nolock(struct yas_driver *d)
{
	if (!d->initialized) {
		return YAS_ERROR_NOT_INITIALIZED;
	}
	return get_filter_enable(d);
}

static int
yas_set_filter_enable_nolock(struct yas_driver *d, int enable)
{
	if (!d->initialized) {
		return YAS_ERROR_NOT_INITIALIZED;
	}
	set_filter_enable(d, enable);
	return YAS_NO_ERROR;
}

static int
yas_get_position_nolock(struct yas_driver *d, int *position)
{
	if (!d->initialized) {
		return YAS_ERROR_NOT_INITIALIZED;
	}
	*position = get_position(d);
	return YAS_NO_ERROR;
}

static int
yas_set_position_nolock(struct yas_driver *d, int position)
{
	if (!d->initialized) {
		return YAS_ERROR_NOT_INITIALIZED;
	}
	if (get_active(d)) {
		yas_cdrv_set_transformatiom_matrix(YAS_TRANSFORMATION[position]);
	}
	set_position(d, position);
	filter_init(d);
	return YAS_NO_ERROR;
}

static int
yas_read_reg_nolock(struct yas_driver *d, uint8_t *buf, int len)
{
	if (!d->initialized) {
		return YAS_ERROR_NOT_INITIALIZED;
	}
	if (!get_active(d)) {
		if (d->callback.i2c_open() < 0) {
			return YAS_ERROR_I2C;
		}
	}
	if (d->callback.i2c_read(YAS_MAG_I2C_SLAVEADDR, buf, len) < 0) {
		return YAS_ERROR_I2C;
	}
	if (!get_active(d)) {
		if (d->callback.i2c_close() < 0) {
			return YAS_ERROR_I2C;
		}
	}

	return YAS_NO_ERROR;
}

static int
yas_write_reg_nolock(struct yas_driver *d, const uint8_t *buf, int len)
{
	if (!d->initialized) {
		return YAS_ERROR_NOT_INITIALIZED;
	}
	if (!get_active(d)) {
		if (d->callback.i2c_open() < 0) {
			return YAS_ERROR_I2C;
		}
	}
	if (d->callback.i2c_write(YAS_MAG_I2C_SLAVEADDR, buf, len) < 0) {
		return YAS_ERROR_I2C;
	}
	if (!get_active(d)) {
		if (d->callback.i2c_close() < 0) {
			return YAS_ERROR_I2C;
		}
	}

	return YAS_NO_ERROR;
}

static int
yas_measure_nolock(struct yas_driver *d, struct yas_mag_data *data, int *time_delay_ms)
{
	uint32_t time_delay = YAS_MAG_ERROR_DELAY;
	int rt, i;

	if (!d->initialized) {
		return YAS_ERROR_NOT_INITIALIZED;
	}
	*time_delay_ms = YAS_MAG_ERROR_DELAY;

	if (!get_active(d)) {
		for (i = 0; i < 3; i++) {
			data->xyz.v[i] = get_previous_mag(d)[i];
			data->raw.v[i] = get_previous_mag_w_offset(d)[i];
			data->xy1y2.v[i] = get_previous_xy1y2(d)[i];
		}
		data->temperature = get_previous_temperature(d);
		return YAS_NO_ERROR;
	}

	rt = measure(d, data->xyz.v, data->raw.v, data->xy1y2.v,
			&data->temperature, &time_delay);
	if (rt >= 0) {
		*time_delay_ms = time_delay;
		if (*time_delay_ms > 0) {
			*time_delay_ms += 1; /* for the system that the time is in usec
						unit */
		}
	}

	return rt;
}

static int
yas_init_nolock(struct yas_driver *d)
{
	int noise[] = {
		YAS_MAG_DEFAULT_FILTER_NOISE_X,
		YAS_MAG_DEFAULT_FILTER_NOISE_Y,
		YAS_MAG_DEFAULT_FILTER_NOISE_Z
	};

	YLOGI((KERN_INFO"yas_init_nolock IN\n"));

	if (d->initialized) {
		return YAS_ERROR_NOT_INITIALIZED;
	}

	utimer_lib_init(this_driver.callback.current_time);
	utimer_init(get_data_timer(d), 50);
	utimer_init(get_initcoil_timer(d), YAS_INITCOIL_INTERVAL);
	utimer_init(get_initcoil_giveup_timer(d), YAS_INITCOIL_GIVEUP_INTERVAL);
	utimer_init(get_detect_overflow_timer(d), YAS_DETECT_OVERFLOW_INTERVAL);

	set_delay_timer_use_data(d, 0);
	set_delay_timer_interval(d,
			YAS_DEFAULT_DATA_INTERVAL / YAS_DEFAULT_CALIB_INTERVAL);
	set_delay_timer_counter(d,
			YAS_DEFAULT_DATA_INTERVAL / YAS_DEFAULT_CALIB_INTERVAL);

	set_filter_enable(d, FALSE);
	set_filter_len(d, YAS_MAG_DEFAULT_FILTER_LEN);
	set_filter_thresh(d, YAS_MAG_DEFAULT_FILTER_THRESH);
	set_filter_noise(d, noise);
	filter_init(d);
	set_calib_offset(d, INVALID_CALIB_OFFSET);
	set_offset(d, INVALID_OFFSET);
	set_active(d, FALSE);
	set_position(d, 0);

	d->initialized = 1;

	YLOGI((KERN_INFO"yas_init_nolock OUT\n"));

	return YAS_NO_ERROR;
}

static int
yas_term_nolock(struct yas_driver *d)
{
	YLOGI((KERN_INFO"yas_term_nolock\n"));

	if (!d->initialized) {
		return YAS_ERROR_NOT_INITIALIZED;
	}

	if (get_active(d)) {
		suspend(d);
	}
	d->initialized = 0;

	YLOGI((KERN_INFO"yas_term_nolock out\n"));
	return YAS_NO_ERROR;
}

static int
yas_get_delay(void)
{
	int ms = 0, rt;

	YLOGI((KERN_INFO"yas_get_delay\n"));

	if (lock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	rt = yas_get_delay_nolock(&this_driver, &ms);

	if (unlock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	YLOGI((KERN_INFO"yas_get_delay[%d] OUT\n", ms));

	return (rt < 0 ? rt : ms);
}

static int
yas_set_delay(int delay)
{
	int rt;

	YLOGI((KERN_INFO"yas_set_delay\n"));

	if (lock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	rt = yas_set_delay_nolock(&this_driver, delay);

	if (unlock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	YLOGI((KERN_INFO"yas_set_delay OUT\n"));

	return rt;
}

static int
yas_get_offset(struct yas_mag_offset *offset)
{
	int rt;

	YLOGI((KERN_INFO"yas_get_offset\n"));

	if (offset == NULL) {
		return YAS_ERROR_ARG;
	}

	if (lock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	rt = yas_get_offset_nolock(&this_driver, offset);

	if (unlock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	YLOGI((KERN_INFO"yas_get_offset[%d] OUT\n", rt));

	return rt;
}

static int
yas_set_offset(struct yas_mag_offset *offset)
{
	int rt;

	YLOGI((KERN_INFO"yas_set_offset IN\n"));

	if (offset == NULL) {
		return YAS_ERROR_ARG;
	}

	if (lock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	rt = yas_set_offset_nolock(&this_driver, offset);

	if (unlock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	YLOGI((KERN_INFO"yas_set_offset OUT\n"));

	return rt;
}

static int
yas_get_enable(void)
{
	int rt;

	YLOGI((KERN_INFO"yas_get_enable\n"));

	if (lock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	rt = yas_get_enable_nolock(&this_driver);

	if (unlock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	YLOGI((KERN_INFO"yas_get_enable OUT[%d]\n", rt));

	return rt;
}

static int
yas_set_enable(int enable)
{
	int rt;

	YLOGI((KERN_INFO"yas_set_enable IN\n"));

	if (lock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	rt = yas_set_enable_nolock(&this_driver, enable);

	if (unlock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	YLOGI((KERN_INFO"yas_set_enable OUT\n"));

	return rt;
}

static int
yas_get_filter(struct yas_mag_filter *filter)
{
	int rt;

	YLOGI((KERN_INFO"yas_get_filter\n"));

	if (filter == NULL) {
		return YAS_ERROR_ARG;
	}

	if (lock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	rt = yas_get_filter_nolock(&this_driver, filter);

	if (unlock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	YLOGI((KERN_INFO"yas_get_filter[%d] OUT\n", rt));

	return rt;
}

static int
yas_set_filter(struct yas_mag_filter *filter)
{
	int rt, i;

	YLOGI((KERN_INFO"yas_set_filter IN\n"));

	if (filter == NULL
			|| filter->len < 0
			|| YAS_MAG_MAX_FILTER_LEN < filter->len
			|| filter->threshold < 0) {
		return YAS_ERROR_ARG;
	}
	for (i = 0; i < 3; i++) {
		if (filter->noise[i] < 0) {
			return YAS_ERROR_ARG;
		}
	}

	if (lock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	rt = yas_set_filter_nolock(&this_driver, filter);

	if (unlock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	YLOGI((KERN_INFO"yas_set_filter OUT\n"));

	return rt;
}

static int
yas_get_filter_enable(void)
{
	int rt;

	YLOGI((KERN_INFO"yas_get_filter_enable\n"));

	if (lock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	rt = yas_get_filter_enable_nolock(&this_driver);

	if (unlock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	YLOGI((KERN_INFO"yas_get_filter_enable OUT[%d]\n", rt));

	return rt;
}

static int
yas_set_filter_enable(int enable)
{
	int rt;

	YLOGI((KERN_INFO"yas_set_filter_enable IN\n"));

	if (lock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	rt = yas_set_filter_enable_nolock(&this_driver, enable);

	if (unlock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	YLOGI((KERN_INFO"yas_set_filter_enable OUT\n"));

	return rt;
}

static int
yas_get_position(void)
{
	int position = 0;
	int rt;

	YLOGI((KERN_INFO"yas_get_position\n"));

	if (lock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	rt = yas_get_position_nolock(&this_driver, &position);

	if (unlock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	YLOGI((KERN_INFO"yas_get_position[%d] OUT\n", position));

	return (rt < 0 ? rt : position);
}

static int
yas_set_position(int position)
{
	int rt;

	YLOGI((KERN_INFO"yas_set_position\n"));

	if (position < 0 || 7 < position) {
		return YAS_ERROR_ARG;
	}

	if (lock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	rt = yas_set_position_nolock(&this_driver, position);

	if (unlock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	YLOGI((KERN_INFO"yas_set_position[%d] OUT\n", position));

	return rt;
}

static int
yas_read_reg(uint8_t *buf, int len)
{
	int rt;

	YLOGI((KERN_INFO"yas_read_reg\n"));

	if (buf == NULL || len <= 0) {
		return YAS_ERROR_ARG;
	}

	if (lock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	rt = yas_read_reg_nolock(&this_driver, buf, len);

	if (unlock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	YLOGI((KERN_INFO"yas_read_reg[%d] OUT\n", rt));

	return rt;
}

static int
yas_write_reg(const uint8_t *buf, int len)
{
	int rt;

	YLOGI((KERN_INFO"yas_write_reg\n"));

	if (buf == NULL || len <= 0) {
		return YAS_ERROR_ARG;
	}

	if (lock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	rt = yas_write_reg_nolock(&this_driver, buf, len);

	if (unlock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	YLOGI((KERN_INFO"yas_write_reg[%d] OUT\n", rt));

	return rt;
}

static int
yas_measure(struct yas_mag_data *data, int *time_delay_ms)
{
	int rt;

	YLOGD((KERN_DEBUG"yas_measure IN\n"));

	if (data == NULL || time_delay_ms == NULL) {
		return YAS_ERROR_ARG;
	}

	if (lock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	rt = yas_measure_nolock(&this_driver, data, time_delay_ms);

	if (unlock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	YLOGD((KERN_DEBUG"yas_measure OUT[%d]\n", rt));

	return rt;
}

static int
yas_init(void)
{
	int rt;

	YLOGI((KERN_INFO"yas_init\n"));

	if (lock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	rt = yas_init_nolock(&this_driver);

	if (unlock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	return rt;
}

static int
yas_term(void)
{
	int rt;
	YLOGI((KERN_INFO"yas_term\n"));

	if (lock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	rt = yas_term_nolock(&this_driver);

	if (unlock() < 0) {
		return YAS_ERROR_RESTARTSYS;
	}

	return rt;
}

int
yas_mag_driver_init(struct yas_mag_driver *f)
{
	if (f == NULL) {
		return YAS_ERROR_ARG;
	}
	if (f->callback.i2c_open == NULL
			|| f->callback.i2c_close == NULL
			|| f->callback.i2c_read == NULL
			|| f->callback.i2c_write == NULL
			|| f->callback.msleep == NULL
			|| f->callback.current_time == NULL) {
		return YAS_ERROR_ARG;
	}

	f->init = yas_init;
	f->term = yas_term;
	f->get_delay = yas_get_delay;
	f->set_delay = yas_set_delay;
	f->get_offset = yas_get_offset;
	f->set_offset = yas_set_offset;
	f->get_enable = yas_get_enable;
	f->set_enable = yas_set_enable;
	f->get_filter = yas_get_filter;
	f->set_filter = yas_set_filter;
	f->get_filter_enable = yas_get_filter_enable;
	f->set_filter_enable = yas_set_filter_enable;
	f->get_position = yas_get_position;
	f->set_position = yas_set_position;
	f->read_reg = yas_read_reg;
	f->write_reg = yas_write_reg;
	f->measure = yas_measure;

	if ((f->callback.lock == NULL && f->callback.unlock != NULL)
			|| (f->callback.lock != NULL && f->callback.unlock == NULL)) {
		this_driver.callback.lock = NULL;
		this_driver.callback.unlock = NULL;
	}
	else {
		this_driver.callback.lock = f->callback.lock;
		this_driver.callback.unlock = f->callback.unlock;
	}
	this_driver.callback.i2c_open = f->callback.i2c_open;
	this_driver.callback.i2c_close = f->callback.i2c_close;
	this_driver.callback.i2c_write = f->callback.i2c_write;
	this_driver.callback.i2c_read = f->callback.i2c_read;
	this_driver.callback.msleep = f->callback.msleep;
	this_driver.callback.current_time = f->callback.current_time;
	yas_term();

	return YAS_NO_ERROR;
}

/*Linux Driver*/

#define GEOMAGNETIC_I2C_DEVICE_NAME         "geomagnetic"
#define GEOMAGNETIC_INPUT_NAME              "geomagnetic"
#define GEOMAGNETIC_INPUT_RAW_NAME          "geomagnetic_raw"

#define ABS_STATUS                          (ABS_BRAKE)
#define ABS_WAKE                            (ABS_MISC)

#define ABS_RAW_DISTORTION                  (ABS_THROTTLE)
#define ABS_RAW_THRESHOLD                   (ABS_RUDDER)
#define ABS_RAW_SHAPE                       (ABS_WHEEL)
#define ABS_RAW_REPORT                      (ABS_GAS)

struct geomagnetic_data {
	struct input_dev *input_data;
	struct input_dev *input_raw;
	struct delayed_work work;
	struct semaphore driver_lock;
	struct semaphore multi_lock;
	atomic_t last_data[3];
	atomic_t last_status;
	atomic_t enable;
	int filter_enable;
	int filter_len;
	int32_t filter_noise[3];
	int32_t filter_threshold;
	int delay;
	int32_t threshold;
	int32_t distortion[3];
	int32_t shape;
	struct yas_mag_offset driver_offset;
#if DEBUG
	int suspend;
#endif
};

static int
geomagnetic_i2c_open(void)
{
	return 0;
}

static int
geomagnetic_i2c_close(void)
{
	return 0;
}

static int
geomagnetic_i2c_write(uint8_t slave, const uint8_t *buf, int len)
{
	if (i2c_master_send(yas_client, buf, len) < 0) {
		return -1;
	}
#if DEBUG
	YLOGD((KERN_DEBUG"[W] [%02x]\n", buf[0]));
#endif

	return 0;
}

static int
geomagnetic_i2c_read(uint8_t slave, uint8_t *buf, int len)
{
	if (i2c_master_recv(yas_client, buf, len) < 0) {
		return -1;
	}

#if DEBUG
	if (len == 1) {
		YLOGD((KERN_DEBUG"[R] [%02x]\n", buf[0]));
	}
	else if (len == 6) {
		YLOGD((KERN_DEBUG"[R] "
					"[%02x%02x%02x%02x%02x%02x]\n",
					buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]));
	}
	else if (len == 8) {
		YLOGD((KERN_DEBUG"[R] "
					"[%02x%02x%02x%02x%02x%02x%02x%02x]\n",
					buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]));
	}
	else if (len == 9) {
		YLOGD((KERN_DEBUG"[R] "
					"[%02x%02x%02x%02x%02x%02x%02x%02x%02x]\n",
					buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8]));
	}
	else if (len == 16) {
		YLOGD((KERN_DEBUG"[R] "
					"[%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x]\n",
					buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
					buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]));
	}
#endif

	return 0;
}


static int
geomagnetic_lock(void)
{
	struct geomagnetic_data *data = NULL;
	int rt;

	if (yas_client == NULL) {
		return -1;
	}

	data = i2c_get_clientdata(yas_client);
	rt = down_interruptible(&data->driver_lock);
	if (rt < 0) {
		up(&data->driver_lock);
	}
	return rt;
}

static int
geomagnetic_unlock(void)
{
	struct geomagnetic_data *data = NULL;

	if (yas_client == NULL) {
		return -1;
	}

	data = i2c_get_clientdata(yas_client);
	up(&data->driver_lock);
	return 0;
}

static void
geomagnetic_msleep(int ms)
{
	msleep(ms);
}

static void
geomagnetic_current_time(int32_t *sec, int32_t *msec)
{
	struct timeval tv;

	do_gettimeofday(&tv);

	*sec = tv.tv_sec;
	*msec = tv.tv_usec / 1000;
}

static struct yas_mag_driver hwdep_driver = {
	.callback = {
		.lock           = geomagnetic_lock,
		.unlock         = geomagnetic_unlock,
		.i2c_open       = geomagnetic_i2c_open,
		.i2c_close      = geomagnetic_i2c_close,
		.i2c_read       = geomagnetic_i2c_read,
		.i2c_write      = geomagnetic_i2c_write,
		.msleep         = geomagnetic_msleep,
		.current_time   = geomagnetic_current_time,
	},
};

static int
geomagnetic_multi_lock(void)
{
	struct geomagnetic_data *data = NULL;
	int rt;

	if (yas_client == NULL) {
		return -1;
	}

	data = i2c_get_clientdata(yas_client);
	rt = down_interruptible(&data->multi_lock);
	if (rt < 0) {
		up(&data->multi_lock);
	}
	return rt;
}

static int
geomagnetic_multi_unlock(void)
{
	struct geomagnetic_data *data = NULL;

	if (yas_client == NULL) {
		return -1;
	}

	data = i2c_get_clientdata(yas_client);
	up(&data->multi_lock);
	return 0;
}

static int
geomagnetic_enable(struct geomagnetic_data *data)
{
	if (!atomic_cmpxchg(&data->enable, 0, 1)) {
		schedule_delayed_work(&data->work, 0);
	}

	return 0;
}

static int
geomagnetic_disable(struct geomagnetic_data *data)
{
	if (atomic_cmpxchg(&data->enable, 1, 0)) {
		//cancel_delayed_work_sync(&data->work);
		cancel_delayed_work(&data->work); // P110420-1338 issue (phill-it)
	}

	return 0;
}

/* Sysfs interface */
static ssize_t
geomagnetic_delay_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	int delay;

	geomagnetic_multi_lock();

	delay = data->delay;

	geomagnetic_multi_unlock();

	return sprintf(buf, "%d\n", delay);
}

static ssize_t
geomagnetic_delay_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	int value = simple_strtol(buf, NULL, 10);

	if (hwdep_driver.set_delay == NULL) {
		return -ENOTTY;
	}

	value = simple_strtol(buf, NULL, 10);
	if (hwdep_driver.set_delay(value) == 0) {
		data->delay = value;
	}

	return count;
}

static ssize_t
geomagnetic_enable_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);

	return sprintf(buf, "%d\n", atomic_read(&data->enable));
}

static ssize_t
geomagnetic_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	int value;

	value = !!simple_strtol(buf, NULL, 10);
	if (hwdep_driver.set_enable == NULL) {
		return -ENOTTY;
	}

	if (geomagnetic_multi_lock() < 0) {
		return count;
	}

	if (hwdep_driver.set_enable(value) == 0) {
		if (value) {
			geomagnetic_enable(data);
		}
		else {
			geomagnetic_disable(data);
		}
	}

	geomagnetic_multi_unlock();

	return count;
}

static ssize_t
geomagnetic_filter_enable_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	int filter_enable;

	geomagnetic_multi_lock();

	filter_enable = data->filter_enable;

	geomagnetic_multi_unlock();

	return sprintf(buf, "%d\n", filter_enable);
}

static ssize_t
geomagnetic_filter_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	int value;

	if (hwdep_driver.set_filter_enable == NULL) {
		return -ENOTTY;
	}

	value = simple_strtol(buf, NULL, 10);
	if (geomagnetic_multi_lock() < 0) {
		return count;
	}

	if (hwdep_driver.set_filter_enable(value) == 0) {
		data->filter_enable = !!value;
	}

	geomagnetic_multi_unlock();

	return count;
}

static ssize_t
geomagnetic_filter_len_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	int filter_len;

	geomagnetic_multi_lock();

	filter_len = data->filter_len;

	geomagnetic_multi_unlock();

	return sprintf(buf, "%d\n", filter_len);
}

static ssize_t
geomagnetic_filter_len_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	struct yas_mag_filter filter;
	int value;


	if (hwdep_driver.get_filter == NULL || hwdep_driver.set_filter == NULL) {
		return -ENOTTY;
	}

	value = simple_strtol(buf, NULL, 10);

	if (geomagnetic_multi_lock() < 0) {
		return count;
	}

	hwdep_driver.get_filter(&filter);
	filter.len = value;
	if (hwdep_driver.set_filter(&filter) == 0) {
		data->filter_len = value;
	}

	geomagnetic_multi_unlock();

	return count;
}

static ssize_t
geomagnetic_filter_noise_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	int rt;

	geomagnetic_multi_lock();

	rt = sprintf(buf, "%d %d %d\n",
			data->filter_noise[0],
			data->filter_noise[1],
			data->filter_noise[2]);

	geomagnetic_multi_unlock();

	return rt;
}

static ssize_t
geomagnetic_filter_noise_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct yas_mag_filter filter;
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	int32_t filter_noise[3];

	geomagnetic_multi_lock();

	sscanf(buf, "%d %d %d",
			&filter_noise[0],
			&filter_noise[1],
			&filter_noise[2]);
	hwdep_driver.get_filter(&filter);
	memcpy(filter.noise, filter_noise, sizeof(filter.noise));
	if (hwdep_driver.set_filter(&filter) == 0) {
		memcpy(data->filter_noise, filter_noise, sizeof(data->filter_noise));
	}

	geomagnetic_multi_unlock();

	return count;
}

static ssize_t
geomagnetic_filter_threshold_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	int32_t filter_threshold;

	geomagnetic_multi_lock();

	filter_threshold = data->filter_threshold;

	geomagnetic_multi_unlock();

	return sprintf(buf, "%d\n", filter_threshold);
}

static ssize_t
geomagnetic_filter_threshold_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	struct yas_mag_filter filter;
	int value;


	if (hwdep_driver.get_filter == NULL || hwdep_driver.set_filter == NULL) {
		return -ENOTTY;
	}

	value = simple_strtol(buf, NULL, 10);

	if (geomagnetic_multi_lock() < 0) {
		return count;
	}

	hwdep_driver.get_filter(&filter);
	filter.threshold = value;
	if (hwdep_driver.set_filter(&filter) == 0) {
		data->filter_threshold = value;
	}

	geomagnetic_multi_unlock();

	return count;
}

static ssize_t
geomagnetic_position_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	if (hwdep_driver.get_position == NULL) {
		return -ENOTTY;
	}
	return sprintf(buf, "%d\n", hwdep_driver.get_position());
}

static ssize_t
geomagnetic_position_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	int value = simple_strtol(buf, NULL, 10);

	if (hwdep_driver.set_position == NULL) {
		return -ENOTTY;
	}
	hwdep_driver.set_position(value);

	return count;
}

static ssize_t
geomagnetic_data_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	int rt;

	rt = sprintf(buf, "%d %d %d\n",
			atomic_read(&data->last_data[0]),
			atomic_read(&data->last_data[1]),
			atomic_read(&data->last_data[2]));

	return rt;
}

static ssize_t
geomagnetic_status_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
	int rt;

	rt = sprintf(buf, "%d\n", atomic_read(&data->last_status));

	return rt;
}

static ssize_t
geomagnetic_wake_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_data);
static int16_t cnt = 1;

	input_report_abs(data->input_data, ABS_WAKE, cnt++);

	return count;
}

#if DEBUG

static int geomagnetic_suspend(struct i2c_client *client, pm_message_t mesg);
static int geomagnetic_resume(struct i2c_client *client);

static ssize_t
geomagnetic_debug_suspend_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input);

	return sprintf(buf, "%d\n", data->suspend);
}

static ssize_t
geomagnetic_debug_suspend_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long suspend = simple_strtol(buf, NULL, 10);

	if (suspend) {
		pm_message_t msg;
		memset(&msg, 0, sizeof(msg));
		geomagnetic_suspend(yas_client, msg);
	} else {
		geomagnetic_resume(yas_client);
	}

	return count;
}

#endif /* DEBUG */

static DEVICE_ATTR(delay, S_IRUGO|S_IWUSR|S_IWGRP,
		geomagnetic_delay_show, geomagnetic_delay_store);
static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP,
		geomagnetic_enable_show, geomagnetic_enable_store);
static DEVICE_ATTR(filter_enable, S_IRUGO|S_IWUSR|S_IWGRP,
		geomagnetic_filter_enable_show, geomagnetic_filter_enable_store);
static DEVICE_ATTR(filter_len, S_IRUGO|S_IWUSR|S_IWGRP,
		geomagnetic_filter_len_show, geomagnetic_filter_len_store);
static DEVICE_ATTR(filter_threshold, S_IRUGO|S_IWUSR|S_IWGRP,
		geomagnetic_filter_threshold_show, geomagnetic_filter_threshold_store);
static DEVICE_ATTR(filter_noise, S_IRUGO|S_IWUSR|S_IWGRP,
		geomagnetic_filter_noise_show, geomagnetic_filter_noise_store);
static DEVICE_ATTR(data, S_IRUGO, geomagnetic_data_show, NULL);
static DEVICE_ATTR(status, S_IRUGO, geomagnetic_status_show, NULL);
static DEVICE_ATTR(wake, S_IWUSR|S_IWGRP, NULL, geomagnetic_wake_store);
static DEVICE_ATTR(position, S_IRUGO|S_IWUSR,
		geomagnetic_position_show, geomagnetic_position_store);
#if DEBUG
static DEVICE_ATTR(debug_suspend, S_IRUGO|S_IWUSR,
		geomagnetic_debug_suspend_show, geomagnetic_debug_suspend_store);
#endif /* DEBUG */

static struct attribute *geomagnetic_attributes[] = {
	&dev_attr_delay.attr,
	&dev_attr_enable.attr,
	&dev_attr_filter_enable.attr,
	&dev_attr_filter_len.attr,
	&dev_attr_filter_threshold.attr,
	&dev_attr_filter_noise.attr,
	&dev_attr_data.attr,
	&dev_attr_status.attr,
	&dev_attr_wake.attr,
	&dev_attr_position.attr,
#if DEBUG
	&dev_attr_debug_suspend.attr,
#endif /* DEBUG */
	NULL
};

static struct attribute_group geomagnetic_attribute_group = {
	.attrs = geomagnetic_attributes
};

static ssize_t
geomagnetic_raw_threshold_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	int threshold;

	geomagnetic_multi_lock();

	threshold = data->threshold;

	geomagnetic_multi_unlock();

	return sprintf(buf, "%d\n", threshold);
}

static ssize_t
geomagnetic_raw_threshold_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	int value = simple_strtol(buf, NULL, 10);

	geomagnetic_multi_lock();

	if (0 <= value && value <= 2) {
		data->threshold = value;
		input_report_abs(data->input_raw, ABS_RAW_THRESHOLD, value);
	}

	geomagnetic_multi_unlock();

	return count;
}

static ssize_t
geomagnetic_raw_distortion_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	int rt;

	geomagnetic_multi_lock();

	rt = sprintf(buf, "%d %d %d\n",
			data->distortion[0],
			data->distortion[1],
			data->distortion[2]);

	geomagnetic_multi_unlock();

	return rt;
}

static ssize_t
geomagnetic_raw_distortion_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	int32_t distortion[3];
static int32_t val = 1;
	int i;

	geomagnetic_multi_lock();

	sscanf(buf, "%d %d %d",
			&distortion[0],
			&distortion[1],
			&distortion[2]);
	if (distortion[0] > 0 && distortion[1] > 0 && distortion[2] > 0) {
		for (i = 0; i < 3; i++) {
			data->distortion[i] = distortion[i];
		}
		input_report_abs(data->input_raw, ABS_RAW_DISTORTION, val++);
	}

	geomagnetic_multi_unlock();

	return count;
}

static ssize_t
geomagnetic_raw_shape_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	int shape;

	geomagnetic_multi_lock();

	shape = data->shape;

	geomagnetic_multi_unlock();

	return sprintf(buf, "%d\n", shape);
}

static ssize_t
geomagnetic_raw_shape_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	int value = simple_strtol(buf, NULL, 10);

	geomagnetic_multi_lock();

	if (0 <= value && value <= 1) {
		data->shape = value;
		input_report_abs(data->input_raw, ABS_RAW_SHAPE, value);
	}

	geomagnetic_multi_unlock();

	return count;
}

static ssize_t
geomagnetic_raw_offsets_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	struct yas_mag_offset offset;
	int accuracy;

	geomagnetic_multi_lock();

	offset = data->driver_offset;
	accuracy = atomic_read(&data->last_status);

	geomagnetic_multi_unlock();

	return sprintf(buf, "%d %d %d %d %d %d %d\n",
			offset.hard_offset[0],
			offset.hard_offset[1],
			offset.hard_offset[2],
			offset.calib_offset.v[0],
			offset.calib_offset.v[1],
			offset.calib_offset.v[2],
			accuracy);
}

static ssize_t
geomagnetic_raw_offsets_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct input_dev *input_raw = to_input_dev(dev);
	struct geomagnetic_data *data = input_get_drvdata(input_raw);
	struct yas_mag_offset offset;
	int32_t hard_offset[3];
	int i, accuracy;

	geomagnetic_multi_lock();

	sscanf(buf, "%d %d %d %d %d %d %d",
			&hard_offset[0],
			&hard_offset[1],
			&hard_offset[2],
			&offset.calib_offset.v[0],
			&offset.calib_offset.v[1],
			&offset.calib_offset.v[2],
			&accuracy);
	if (0 <= accuracy && accuracy <= 3) {
		for (i = 0; i < 3; i++) {
			offset.hard_offset[i] = (int8_t)hard_offset[i];
		}
		if (hwdep_driver.set_offset(&offset) == 0) {
			atomic_set(&data->last_status, accuracy);
			data->driver_offset = offset;
		}
	}

	geomagnetic_multi_unlock();

	return count;
}

static DEVICE_ATTR(threshold, S_IRUGO|S_IWUSR,
		geomagnetic_raw_threshold_show, geomagnetic_raw_threshold_store);
static DEVICE_ATTR(distortion, S_IRUGO|S_IWUSR,
		geomagnetic_raw_distortion_show, geomagnetic_raw_distortion_store);
static DEVICE_ATTR(shape, S_IRUGO|S_IWUSR,
		geomagnetic_raw_shape_show, geomagnetic_raw_shape_store);
static DEVICE_ATTR(offsets, S_IRUGO|S_IWUSR,
		geomagnetic_raw_offsets_show, geomagnetic_raw_offsets_store);

static struct attribute *geomagnetic_raw_attributes[] = {
	&dev_attr_threshold.attr,
	&dev_attr_distortion.attr,
	&dev_attr_shape.attr,
	&dev_attr_offsets.attr,
	NULL
};

static struct attribute_group geomagnetic_raw_attribute_group = {
	.attrs = geomagnetic_raw_attributes
};

/* Interface Functions for Lower Layer */

static void
geomagnetic_input_work_func(struct work_struct *work)
{
	struct geomagnetic_data *data = container_of((struct delayed_work *)work,
			struct geomagnetic_data, work);
	uint32_t time_delay_ms = 100;
	struct yas_mag_offset offset;
	struct yas_mag_data magdata;
	int rt, i, accuracy;


	if (hwdep_driver.measure == NULL || hwdep_driver.get_offset == NULL) {
		return;
	}

	rt = hwdep_driver.measure(&magdata, &time_delay_ms);
	if (rt < 0) {
		YLOGE((KERN_ERR"measure failed[%d]\n", rt));

	}
	YLOGD((KERN_DEBUG"xy1y2 [%d][%d][%d] raw[%d][%d][%d]\n",
				magdata.xy1y2.v[0], magdata.xy1y2.v[1], magdata.xy1y2.v[2],
				magdata.xyz.v[0], magdata.xyz.v[1], magdata.xyz.v[2]));

	if (rt >= 0) {
		accuracy = atomic_read(&data->last_status);

		if ((rt & YAS_REPORT_OVERFLOW_OCCURED)
				|| (rt & YAS_REPORT_HARD_OFFSET_CHANGED)
				|| (rt & YAS_REPORT_CALIB_OFFSET_CHANGED)) {
			static uint16_t count = 1;
			int code = 0;
			int value = 0;

			hwdep_driver.get_offset(&offset);

			geomagnetic_multi_lock();
			data->driver_offset = offset;
			if (rt & YAS_REPORT_OVERFLOW_OCCURED) {
				atomic_set(&data->last_status, 0);
				accuracy = 0;
			}
			geomagnetic_multi_unlock();

			/* report event */
			code |= (rt & YAS_REPORT_OVERFLOW_OCCURED);
			code |= (rt & YAS_REPORT_HARD_OFFSET_CHANGED);
			code |= (rt & YAS_REPORT_CALIB_OFFSET_CHANGED);
			value = (count++ << 16) | (code);
			input_report_abs(data->input_raw, ABS_RAW_REPORT, value);
		}

		if (rt & YAS_REPORT_DATA) {
			for (i = 0; i < 3; i++) {
				atomic_set(&data->last_data[i], magdata.xyz.v[i]);
			}

			/* report magnetic data in [nT] */
			input_report_abs(data->input_data, ABS_X, magdata.xyz.v[0]);
			input_report_abs(data->input_data, ABS_Y, magdata.xyz.v[1]);
			input_report_abs(data->input_data, ABS_Z, magdata.xyz.v[2]);
			input_report_abs(data->input_data, ABS_STATUS, accuracy);
			input_sync(data->input_data);
		}

		if (rt & YAS_REPORT_CALIB) {
			/* report raw magnetic data */
			input_report_abs(data->input_raw, ABS_X, magdata.raw.v[0]);
			input_report_abs(data->input_raw, ABS_Y, magdata.raw.v[1]);
			input_report_abs(data->input_raw, ABS_Z, magdata.raw.v[2]);
			input_sync(data->input_raw);
		}
	}
	else {
		time_delay_ms = 100;
	}

	if (time_delay_ms > 0) {
		schedule_delayed_work(&data->work, msecs_to_jiffies(time_delay_ms) + 1);
	}
	else {
		schedule_delayed_work(&data->work, 0);
	}
}

static int
geomagnetic_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct geomagnetic_data *data = i2c_get_clientdata(client);

	if (atomic_read(&data->enable)) {
		cancel_delayed_work_sync(&data->work);
	}

	if(yas_pdata) {
		if(yas_pdata->reset_n)
			yas_pdata->reset_n(0);

		if(yas_pdata->vreg_en)
			yas_pdata->vreg_en(0);
	}

#if DEBUG
	data->suspend = 1;
#endif

	return 0;
}

static int
geomagnetic_resume(struct i2c_client *client)
{
	struct geomagnetic_data *data = i2c_get_clientdata(client);

	if (atomic_read(&data->enable)) {
		schedule_delayed_work(&data->work, 0);
	}

	if(yas_pdata) {
		if(yas_pdata->vreg_en)
			yas_pdata->vreg_en(1);
	}

	yas529_reset();

#if DEBUG
	data->suspend = 0;
#endif

	return 0;
}
static void  yas529_reset(void)
{
	//int rc = 0;
	if(yas_pdata == NULL) return;

	if(yas_pdata->reset_n)
		yas_pdata->reset_n(0);

	YLOGI((KERN_INFO "Compass I: RST LOW\n"));

	udelay(120);
	if(yas_pdata->reset_n)
		yas_pdata->reset_n(1);

	YLOGI((KERN_INFO "Compass I: RST HIGH\n"));

}
static int
geomagnetic_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct geomagnetic_data *data = NULL;
	struct input_dev *input_data = NULL, *input_raw = NULL;
	int rt, sysfs_created = 0, sysfs_raw_created = 0;
	int data_registered = 0, raw_registered = 0, i;
	struct yas_mag_filter filter;

	YLOGI((KERN_INFO "geomagnetic_probe start on h/w revision %d.\n", system_rev));
	printk(KERN_INFO "geomagnetic_probe start on h/w revision %d.\n", system_rev);

	i2c_set_clientdata(client, NULL);
	data = kzalloc(sizeof(struct geomagnetic_data), GFP_KERNEL);
	if (data == NULL) {
		rt = -ENOMEM;
		goto err;
	}
	data->threshold = YAS_DEFAULT_MAGCALIB_THRESHOLD;
	for (i = 0; i < 3; i++) {
		data->distortion[i] = YAS_DEFAULT_MAGCALIB_DISTORTION;
	}
	data->shape = 0;
	atomic_set(&data->enable, 0);
	for (i = 0; i < 3; i++) {
		atomic_set(&data->last_data[i], 0);
	}
	atomic_set(&data->last_status, 0);
	INIT_DELAYED_WORK(&data->work, geomagnetic_input_work_func);
	init_MUTEX(&data->driver_lock);
	init_MUTEX(&data->multi_lock);

	input_data = input_allocate_device();
	if (input_data == NULL) {
		rt = -ENOMEM;
		YLOGE((KERN_ERR"geomagnetic_probe: Failed to allocate input_data device\n"));
		goto err;
	}

	input_data->name = GEOMAGNETIC_INPUT_NAME;
	input_data->id.bustype = BUS_I2C;
	set_bit(EV_ABS, input_data->evbit);
	input_set_capability(input_data, EV_ABS, ABS_X);
	input_set_capability(input_data, EV_ABS, ABS_Y);
	input_set_capability(input_data, EV_ABS, ABS_Z);
	input_set_capability(input_data, EV_ABS, ABS_STATUS);
	input_set_capability(input_data, EV_ABS, ABS_WAKE);
	input_data->dev.parent = &client->dev;

	rt = input_register_device(input_data);
	if (rt) {
		YLOGE((KERN_ERR"geomagnetic_probe: Unable to register input_data device: %s\n",
					input_data->name));
		goto err;
	}
	data_registered = 1;

	rt = sysfs_create_group(&input_data->dev.kobj,
			&geomagnetic_attribute_group);
	if (rt) {
		YLOGE((KERN_ERR"geomagnetic_probe: sysfs_create_group failed[%s]\n",
					input_data->name));
		goto err;
	}
	sysfs_created = 1;

	input_raw = input_allocate_device();
	if (input_raw == NULL) {
		rt = -ENOMEM;
		YLOGE((KERN_ERR"geomagnetic_probe: Failed to allocate input_raw device\n"));
		goto err;
	}

	input_raw->name = GEOMAGNETIC_INPUT_RAW_NAME;
	input_raw->id.bustype = BUS_I2C;
	set_bit(EV_ABS, input_raw->evbit);
	input_set_capability(input_raw, EV_ABS, ABS_X);
	input_set_capability(input_raw, EV_ABS, ABS_Y);
	input_set_capability(input_raw, EV_ABS, ABS_Z);
	input_set_capability(input_raw, EV_ABS, ABS_RAW_DISTORTION);
	input_set_capability(input_raw, EV_ABS, ABS_RAW_THRESHOLD);
	input_set_capability(input_raw, EV_ABS, ABS_RAW_SHAPE);
	input_set_capability(input_raw, EV_ABS, ABS_RAW_REPORT);
	input_raw->dev.parent = &client->dev;

	rt = input_register_device(input_raw);
	if (rt) {

		YLOGE((KERN_ERR"geomagnetic_probe: Unable to register input_raw device: %s\n",
					input_raw->name));
		goto err;
	}
	raw_registered = 1;

	rt = sysfs_create_group(&input_raw->dev.kobj,
			&geomagnetic_raw_attribute_group);
	if (rt) {

		YLOGE((KERN_ERR"geomagnetic_probe: sysfs_create_group failed[%s]\n",
					input_data->name));
		goto err;
	}
	sysfs_raw_created = 1;

	yas_client = client;
	yas_pdata = client->dev.platform_data;
	data->input_raw = input_raw;
	data->input_data = input_data;
	input_set_drvdata(input_data, data);
	input_set_drvdata(input_raw, data);
	i2c_set_clientdata(client, data);

	yas529_reset();


	if ((rt = yas_mag_driver_init(&hwdep_driver)) < 0) {

		YLOGE((KERN_ERR"yas_mag_driver_init failed[%d]\n", rt));
		goto err;
	}
	if (hwdep_driver.init != NULL) {
		if ((rt = hwdep_driver.init()) < 0) {

			YLOGE((KERN_ERR"hwdep_driver.init() failed[%d]\n", rt));
			goto err;
		}
	}
	if (hwdep_driver.set_position != NULL) {
		if (hwdep_driver.set_position(0) < 0) {

			YLOGE((KERN_ERR"hwdep_driver.set_position() failed[%d]\n", rt));
			goto err;
		}
	}
	if (hwdep_driver.get_offset != NULL) {
		if (hwdep_driver.get_offset(&data->driver_offset) < 0) {

			YLOGE((KERN_ERR"hwdep_driver get_driver_state failed\n"));
			goto err;
		}
	}
	if (hwdep_driver.get_delay != NULL) {
		data->delay = hwdep_driver.get_delay();
	}
	if (hwdep_driver.set_filter_enable != NULL) {
		/* default to enable */
		if (hwdep_driver.set_filter_enable(1) == 0) {
			data->filter_enable = 1;
		}
	}
	if (hwdep_driver.get_filter != NULL) {
		if (hwdep_driver.get_filter(&filter) < 0) {
			YLOGE((KERN_ERR"hwdep_driver get_filter failed\n"));
			goto err;
		}
		data->filter_len = filter.len;
		for (i = 0; i < 3; i++) {
			data->filter_noise[i] = filter.noise[i];
		}
		data->filter_threshold = filter.threshold;
	}

	YLOGI((KERN_INFO"geomagnetic_probe end \n"));

	return 0;

err:
	if (data != NULL) {
		if (input_raw != NULL) {
			if (sysfs_raw_created) {
				sysfs_remove_group(&input_raw->dev.kobj,
						&geomagnetic_raw_attribute_group);
			}
			if (raw_registered) {
				input_unregister_device(input_raw);
			}
			else {
				input_free_device(input_raw);
			}
		}
		if (input_data != NULL) {
			if (sysfs_created) {
				sysfs_remove_group(&input_data->dev.kobj,
						&geomagnetic_attribute_group);
			}
			if (data_registered) {
				input_unregister_device(input_data);
			}
			else {
				input_free_device(input_data);
			}
		}
		kfree(data);
	}

	return rt;
}

static int
geomagnetic_remove(struct i2c_client *client)
{
	struct geomagnetic_data *data = i2c_get_clientdata(client);

	if (data != NULL) {
		geomagnetic_disable(data);
		if (hwdep_driver.term != NULL) {
			hwdep_driver.term();
		}

		input_unregister_device(data->input_raw);
		sysfs_remove_group(&data->input_data->dev.kobj,
				&geomagnetic_attribute_group);
		sysfs_remove_group(&data->input_raw->dev.kobj,
				&geomagnetic_raw_attribute_group);
		input_unregister_device(data->input_data);
		kfree(data);
	}

	return 0;
}

/* I2C Device Driver */
static struct i2c_device_id geomagnetic_idtable[] = {
	{"yas529", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, geomagnetic_idtable);

static struct i2c_driver geomagnetic_i2c_driver = {
	.driver = {
		.name       = "yas529",
		.owner      = THIS_MODULE,
	},

	.id_table       = geomagnetic_idtable,
	.probe          = geomagnetic_probe,
	.remove         = geomagnetic_remove,
	.suspend        = geomagnetic_suspend,
	.resume         = geomagnetic_resume,
};

static int __init
geomagnetic_init(void)
{
	return i2c_add_driver(&geomagnetic_i2c_driver);
}

static void __exit
geomagnetic_term(void)
{
	i2c_del_driver(&geomagnetic_i2c_driver);
}

module_init(geomagnetic_init);
module_exit(geomagnetic_term);

MODULE_AUTHOR("Yamaha Corporation");
MODULE_DESCRIPTION("YAS529 Geomagnetic Sensor Driver");
MODULE_LICENSE( "GPL" );
MODULE_VERSION("3.0.401");



