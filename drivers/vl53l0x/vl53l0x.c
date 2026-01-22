#include "vl53l0x.h"
#include <assert.h>
#include <error.h>

TITAN_DEBUG_FILE_MARK;

#define VL53L0X_ADDRESS                                         0x52
#define VL53L0X_REG_IDENTIFICATION_MODEL_ID                     0xC0
#define VL53L0X_REG_IDENTIFICATION_REVISION_ID                  0xC2
#define VL53L0X_REG_SYSRANGE_START                              0x00
#define VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG                      0x01
#define VL53L0X_REG_SYSTEM_INTERMEASUREMENT_PERIOD              0x04
#define VL53L0X_REG_SYSTEM_INTERRUPT_CONFIG_GPIO                0x0A
#define VL53L0X_REG_GPIO_HV_MUX_ACTIVE_HIGH                     0x84
#define VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR                      0x0B
#define VL53L0X_REG_RESULT_INTERRUPT_STATUS                     0x13
#define VL53L0X_REG_RESULT_RANGE_STATUS                         0x14
#define VL53L0X_REG_MSRC_CONFIG_CONTROL                         0x60
#define VL53L0X_REG_PRE_RANGE_CONFIG_VALID_PHASE_LOW            0x56
#define VL53L0X_REG_PRE_RANGE_CONFIG_VALID_PHASE_HIGH           0x57
#define VL53L0X_REG_FINAL_RANGE_CONFIG_VALID_PHASE_LOW          0x47
#define VL53L0X_REG_FINAL_RANGE_CONFIG_VALID_PHASE_HIGH         0x48
#define VL53L0X_REG_FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT 0x44
#define VL53L0X_REG_PRE_RANGE_CONFIG_VCSEL_PERIOD               0x50
#define VL53L0X_REG_PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI          0x51
#define VL53L0X_REG_FINAL_RANGE_CONFIG_VCSEL_PERIOD             0x70
#define VL53L0X_REG_FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI        0x71
#define VL53L0X_REG_MSRC_CONFIG_TIMEOUT_MACROP                  0x46
#define VL53L0X_REG_OSC_CALIBRATE_VAL                           0xF8
#define VL53L0X_REG_GLOBAL_CONFIG_VCSEL_WIDTH                   0x32
#define VL53L0X_REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_0            0xB0
#define VL53L0X_REG_GLOBAL_CONFIG_REF_EN_START_SELECT           0xB6
#define VL53L0X_REG_DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD         0x4E
#define VL53L0X_REG_DYNAMIC_SPAD_REF_EN_START_OFFSET            0x4F
#define VL53L0X_REG_VHV_CONFIG_PAD_SCL_SDA_EXTSUP_HV            0x89
#define VL53L0X_REG_ALGO_PHASECAL_LIM                           0x30
#define VL53L0X_REG_ALGO_PHASECAL_CONFIG_TIMEOUT                0x30

#define VL53L0X_MODEL_ID                                        0xEE

// Decode VCSEL (vertical cavity surface emitting laser) pulse period in PCLKs
// from register value
// based on VL53L0X_decode_vcsel_period()
#define decodeVcselPeriod(reg_val)      (((reg_val) + 1) << 1)

// Encode VCSEL pulse period register value from period in PCLKs
// based on VL53L0X_encode_vcsel_period()
#define encodeVcselPeriod(period_pclks) (((period_pclks) >> 1) - 1)

// Calculate macro period in *nanoseconds* from VCSEL period in PCLKs
// based on VL53L0X_calc_macro_period_ps()
// PLL_period_ps = 1655; macro_period_vclks = 2304
#define calcMacroPeriod(vcsel_period_pclks) ((((uint32_t)2304 * (vcsel_period_pclks) * 1655) + 500) / 1000)

typedef enum {
    VcselPeriodPreRange,
    VcselPeriodFinalRange
} vcselPeriodType_t;

typedef struct {
    uint8_t tcc;
    uint8_t msrc;
    uint8_t dss;
    uint8_t pre_range;
    uint8_t final_range;
} sequence_step_enables_t;

typedef struct {
    uint16_t pre_range_vcsel_period_pclks;
    uint16_t final_range_vcsel_period_pclks;

    uint16_t msrc_dss_tcc_mclks;
    uint16_t pre_range_mclks;
    uint16_t final_range_mclks;
    uint32_t msrc_dss_tcc_us;
    uint32_t pre_range_us;
    uint32_t final_range_us;
} sequence_step_timeouts_t;

static void _vl53l0x_get_sequence_step_enables(vl53l0x_t *dev, sequence_step_enables_t * enables);
static void _vl53l0x_get_sequence_step_timeouts(vl53l0x_t *dev, sequence_step_enables_t const * enables, sequence_step_timeouts_t * timeouts);
static void _vl53l0x_performSingleRefCalibration(vl53l0x_t *dev, uint8_t vhv_init_byte);

static uint16_t _vl53l0x_decodeTimeout(uint16_t value);
static uint16_t _vl53l0x_encodeTimeout(uint32_t timeout_mclks);
static uint32_t _vl53l0x_timeoutMclksToMicroseconds(uint16_t timeout_period_mclks, uint8_t vcsel_period_pclks);
static uint32_t _vl53l0x_timeoutMicrosecondsToMclks(uint32_t timeout_period_us, uint8_t vcsel_period_pclks);

static uint8_t _vl53l0x_setMeasurementTimingBudget(vl53l0x_t *dev, uint32_t budget_us);
static uint32_t _vl53l0x_getMeasurementTimingBudget(vl53l0x_t *dev);

static uint8_t _vl53l0x_setVcselPulsePeriod(vl53l0x_t *dev, vcselPeriodType_t type, uint8_t period_pclks);
static uint8_t _vl53l0x_getVcselPulsePeriod(vl53l0x_t *dev, vcselPeriodType_t type);

// Write an 8-bit register
static void _vl53l0x_writeReg(vl53l0x_t *dev, uint8_t reg, uint8_t value) {
    i2c_master_write_mem(dev->i2c, VL53L0X_ADDRESS, reg, 1, &value, 1);
}

// Write a 16-bit register
static void _vl53l0x_writeReg16Bit(vl53l0x_t *dev, uint8_t reg, uint16_t value) {
    uint8_t buffer[2];
    buffer[0] = (value >> 8) & 0xFF;
    buffer[1] = value & 0xFF;
    i2c_master_write_mem(dev->i2c, VL53L0X_ADDRESS, reg, 1, buffer, 2);
}

// Write a 32-bit register
static void _vl53l0x_writeReg32Bit(vl53l0x_t *dev, uint8_t reg, uint32_t value) {
    uint8_t buffer[4];
    buffer[0] = (value >> 24) & 0xFF;
    buffer[1] = (value >> 16) & 0xFF;
    buffer[2] = (value >> 8) & 0xFF;
    buffer[3] = value & 0xFF;
    i2c_master_write_mem(dev->i2c, VL53L0X_ADDRESS, reg, 1, buffer, 4);
}

// Read an 8-bit register
static uint8_t _vl53l0x_readReg(vl53l0x_t *dev, uint8_t reg) {
    uint8_t value;
    i2c_master_read_mem(dev->i2c, VL53L0X_ADDRESS, reg, 1, &value, 1);
    return value;
}

// Read a 16-bit register
static uint16_t _vl53l0x_readReg16Bit(vl53l0x_t *dev, uint8_t reg) {
    uint16_t value;
    uint8_t buffer[2];
    i2c_master_read_mem(dev->i2c, VL53L0X_ADDRESS, reg, 1, buffer, 2);
    value = (uint16_t)buffer[0] << 8;
    value |= buffer[1];
    return value;
}

// Set the measurement timing budget in microseconds, which is the time allowed
// for one measurement; the ST API and this library take care of splitting the
// timing budget among the sub-steps in the ranging sequence. A longer timing
// budget allows for more accurate measurements. Increasing the budget by a
// factor of N decreases the range measurement standard deviation by a factor of
// sqrt(N). Defaults to about 33 milliseconds; the minimum is 20 ms.
// based on VL53L0X_set_measurement_timing_budget_micro_seconds()
static uint8_t _vl53l0x_setMeasurementTimingBudget(vl53l0x_t *dev, uint32_t budget_us) {
    sequence_step_enables_t enables;
    sequence_step_timeouts_t timeouts;

    uint16_t const StartOverhead     = 1910;
    uint16_t const EndOverhead        = 960;
    uint16_t const MsrcOverhead       = 660;
    uint16_t const TccOverhead        = 590;
    uint16_t const DssOverhead        = 690;
    uint16_t const PreRangeOverhead   = 660;
    uint16_t const FinalRangeOverhead = 550;

    uint32_t const MinTimingBudget = 20000;

    if (budget_us < MinTimingBudget) {
        return 0;
    }

    uint32_t used_budget_us = StartOverhead + EndOverhead;

    _vl53l0x_get_sequence_step_enables(dev, &enables);
    _vl53l0x_get_sequence_step_timeouts(dev, &enables, &timeouts);

    if (enables.tcc) {
        used_budget_us += (timeouts.msrc_dss_tcc_us + TccOverhead);
    }

    if (enables.dss) {
        used_budget_us += 2 * (timeouts.msrc_dss_tcc_us + DssOverhead);
    }
    else if (enables.msrc) {
        used_budget_us += (timeouts.msrc_dss_tcc_us + MsrcOverhead);
    }

    if (enables.pre_range) {
        used_budget_us += (timeouts.pre_range_us + PreRangeOverhead);
    }

    if (enables.final_range) {
        used_budget_us += FinalRangeOverhead;

        // "Note that the final range timeout is determined by the timing
        // budget and the sum of all other timeouts within the sequence.
        // If there is no room for the final range timeout, then an error
        // will be set. Otherwise the remaining time will be applied to
        // the final range."

        if (used_budget_us > budget_us) {
            // "Requested timeout too big."
            return 0;
        }

        uint32_t final_range_timeout_us = budget_us - used_budget_us;

        // set_sequence_step_timeout() begin
        // (SequenceStepId == VL53L0X_SEQUENCESTEP_FINAL_RANGE)

        // "For the final range timeout, the pre-range timeout
        //  must be added. To do this both final and pre-range
        //  timeouts must be expressed in macro periods MClks
        //  because they have different vcsel periods."

        uint32_t final_range_timeout_mclks = _vl53l0x_timeoutMicrosecondsToMclks(final_range_timeout_us, timeouts.final_range_vcsel_period_pclks);

        if (enables.pre_range) {
            final_range_timeout_mclks += timeouts.pre_range_mclks;
        }

        _vl53l0x_writeReg16Bit(dev, VL53L0X_REG_FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI,
        _vl53l0x_encodeTimeout(final_range_timeout_mclks));

        // set_sequence_step_timeout() end
        dev->measurement_timing_budget_us = budget_us; // store for internal reuse
    }
    return 1;
}

// Get the measurement timing budget in microseconds
// based on VL53L0X_get_measurement_timing_budget_micro_seconds()
// in us
static uint32_t _vl53l0x_getMeasurementTimingBudget(vl53l0x_t *dev) {
    sequence_step_enables_t enables;
    sequence_step_timeouts_t timeouts;

    uint16_t const StartOverhead     = 1910;
    uint16_t const EndOverhead        = 960;
    uint16_t const MsrcOverhead       = 660;
    uint16_t const TccOverhead        = 590;
    uint16_t const DssOverhead        = 690;
    uint16_t const PreRangeOverhead   = 660;
    uint16_t const FinalRangeOverhead = 550;

    // "Start and end overhead times always present"
    uint32_t budget_us = StartOverhead + EndOverhead;

    _vl53l0x_get_sequence_step_enables(dev, &enables);
    _vl53l0x_get_sequence_step_timeouts(dev, &enables, &timeouts);

    if (enables.tcc) {
        budget_us += (timeouts.msrc_dss_tcc_us + TccOverhead);
    }

    if (enables.dss) {
        budget_us += 2 * (timeouts.msrc_dss_tcc_us + DssOverhead);
    }
    else if (enables.msrc) {
        budget_us += (timeouts.msrc_dss_tcc_us + MsrcOverhead);
    }

    if (enables.pre_range) {
        budget_us += (timeouts.pre_range_us + PreRangeOverhead);
    }

    if (enables.final_range) {
        budget_us += (timeouts.final_range_us + FinalRangeOverhead);
    }

    dev->measurement_timing_budget_us = budget_us; // store for internal reuse
    return budget_us;
}

// Set the VCSEL (vertical cavity surface emitting laser) pulse period for the
// given period type (pre-range or final range) to the given value in PCLKs.
// Longer periods seem to increase the potential range of the sensor.
// Valid values are (even numbers only):
//  pre:  12 to 18 (initialized default: 14)
//  final: 8 to 14 (initialized default: 10)
// based on VL53L0X_set_vcsel_pulse_period()
static uint8_t _vl53l0x_setVcselPulsePeriod(vl53l0x_t *dev, vcselPeriodType_t type, uint8_t period_pclks) {
    uint8_t vcsel_period_reg = encodeVcselPeriod(period_pclks);

    sequence_step_enables_t enables;
    sequence_step_timeouts_t timeouts;

    _vl53l0x_get_sequence_step_enables(dev, &enables);
    _vl53l0x_get_sequence_step_timeouts(dev, &enables, &timeouts);

    // "Apply specific settings for the requested clock period"
    // "Re-calculate and apply timeouts, in macro periods"

    // "When the VCSEL period for the pre or final range is changed,
    // the corresponding timeout must be read from the device using
    // the current VCSEL period, then the new VCSEL period can be
    // applied. The timeout then must be written back to the device
    // using the new VCSEL period.
    //
    // For the MSRC timeout, the same applies - this timeout being
    // dependant on the pre-range vcsel period."


    if (type == VcselPeriodPreRange) {
        // "Set phase check limits"
        switch (period_pclks) {
            case 12:
                _vl53l0x_writeReg(dev, VL53L0X_REG_PRE_RANGE_CONFIG_VALID_PHASE_HIGH, 0x18);
                break;

            case 14:
                _vl53l0x_writeReg(dev, VL53L0X_REG_PRE_RANGE_CONFIG_VALID_PHASE_HIGH, 0x30);
                break;

            case 16:
                _vl53l0x_writeReg(dev, VL53L0X_REG_PRE_RANGE_CONFIG_VALID_PHASE_HIGH, 0x40);
                break;

            case 18:
                _vl53l0x_writeReg(dev, VL53L0X_REG_PRE_RANGE_CONFIG_VALID_PHASE_HIGH, 0x50);
                break;

            default:
                // invalid period
                return 0;
                break;
        }
        _vl53l0x_writeReg(dev, VL53L0X_REG_PRE_RANGE_CONFIG_VALID_PHASE_LOW, 0x08);

        // apply new VCSEL period
        _vl53l0x_writeReg(dev, VL53L0X_REG_PRE_RANGE_CONFIG_VCSEL_PERIOD, vcsel_period_reg);

        // update timeouts

        // set_sequence_step_timeout() begin
        // (SequenceStepId == VL53L0X_SEQUENCESTEP_PRE_RANGE)

        uint16_t new_pre_range_timeout_mclks = _vl53l0x_timeoutMicrosecondsToMclks(timeouts.pre_range_us, period_pclks);

        _vl53l0x_writeReg16Bit(dev, VL53L0X_REG_PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI, _vl53l0x_encodeTimeout(new_pre_range_timeout_mclks));

        // set_sequence_step_timeout() end

        // set_sequence_step_timeout() begin
        // (SequenceStepId == VL53L0X_SEQUENCESTEP_MSRC)

        uint16_t new_msrc_timeout_mclks = _vl53l0x_timeoutMicrosecondsToMclks(timeouts.msrc_dss_tcc_us, period_pclks);

        _vl53l0x_writeReg(dev, VL53L0X_REG_MSRC_CONFIG_TIMEOUT_MACROP, (new_msrc_timeout_mclks > 256) ? 255 : (new_msrc_timeout_mclks - 1));

        // set_sequence_step_timeout() end
    }
    else if (type == VcselPeriodFinalRange) {
        switch (period_pclks) {
            case 8:
                _vl53l0x_writeReg(dev, VL53L0X_REG_FINAL_RANGE_CONFIG_VALID_PHASE_HIGH, 0x10);
                _vl53l0x_writeReg(dev, VL53L0X_REG_FINAL_RANGE_CONFIG_VALID_PHASE_LOW,  0x08);
                _vl53l0x_writeReg(dev, VL53L0X_REG_GLOBAL_CONFIG_VCSEL_WIDTH, 0x02);
                _vl53l0x_writeReg(dev, VL53L0X_REG_ALGO_PHASECAL_CONFIG_TIMEOUT, 0x0C);
                _vl53l0x_writeReg(dev, 0xFF, 0x01);
                _vl53l0x_writeReg(dev, VL53L0X_REG_ALGO_PHASECAL_LIM, 0x30);
                _vl53l0x_writeReg(dev, 0xFF, 0x00);
                break;

            case 10:
                _vl53l0x_writeReg(dev, VL53L0X_REG_FINAL_RANGE_CONFIG_VALID_PHASE_HIGH, 0x28);
                _vl53l0x_writeReg(dev, VL53L0X_REG_FINAL_RANGE_CONFIG_VALID_PHASE_LOW,  0x08);
                _vl53l0x_writeReg(dev, VL53L0X_REG_GLOBAL_CONFIG_VCSEL_WIDTH, 0x03);
                _vl53l0x_writeReg(dev, VL53L0X_REG_ALGO_PHASECAL_CONFIG_TIMEOUT, 0x09);
                _vl53l0x_writeReg(dev, 0xFF, 0x01);
                _vl53l0x_writeReg(dev, VL53L0X_REG_ALGO_PHASECAL_LIM, 0x20);
                _vl53l0x_writeReg(dev, 0xFF, 0x00);
                break;

            case 12:
                _vl53l0x_writeReg(dev, VL53L0X_REG_FINAL_RANGE_CONFIG_VALID_PHASE_HIGH, 0x38);
                _vl53l0x_writeReg(dev, VL53L0X_REG_FINAL_RANGE_CONFIG_VALID_PHASE_LOW,  0x08);
                _vl53l0x_writeReg(dev, VL53L0X_REG_GLOBAL_CONFIG_VCSEL_WIDTH, 0x03);
                _vl53l0x_writeReg(dev, VL53L0X_REG_ALGO_PHASECAL_CONFIG_TIMEOUT, 0x08);
                _vl53l0x_writeReg(dev, 0xFF, 0x01);
                _vl53l0x_writeReg(dev, VL53L0X_REG_ALGO_PHASECAL_LIM, 0x20);
                _vl53l0x_writeReg(dev, 0xFF, 0x00);
                break;

            case 14:
                _vl53l0x_writeReg(dev, VL53L0X_REG_FINAL_RANGE_CONFIG_VALID_PHASE_HIGH, 0x48);
                _vl53l0x_writeReg(dev, VL53L0X_REG_FINAL_RANGE_CONFIG_VALID_PHASE_LOW,  0x08);
                _vl53l0x_writeReg(dev, VL53L0X_REG_GLOBAL_CONFIG_VCSEL_WIDTH, 0x03);
                _vl53l0x_writeReg(dev, VL53L0X_REG_ALGO_PHASECAL_CONFIG_TIMEOUT, 0x07);
                _vl53l0x_writeReg(dev, 0xFF, 0x01);
                _vl53l0x_writeReg(dev, VL53L0X_REG_ALGO_PHASECAL_LIM, 0x20);
                _vl53l0x_writeReg(dev, 0xFF, 0x00);
                break;

            default:
                // invalid period
                return 0;
                break;
        }

        // apply new VCSEL period
        _vl53l0x_writeReg(dev, VL53L0X_REG_FINAL_RANGE_CONFIG_VCSEL_PERIOD, vcsel_period_reg);

        // update timeouts

        // set_sequence_step_timeout() begin
        // (SequenceStepId == VL53L0X_SEQUENCESTEP_FINAL_RANGE)

        // "For the final range timeout, the pre-range timeout
        //  must be added. To do this both final and pre-range
        //  timeouts must be expressed in macro periods MClks
        //  because they have different vcsel periods."

        uint16_t new_final_range_timeout_mclks = _vl53l0x_timeoutMicrosecondsToMclks(timeouts.final_range_us, period_pclks);

        if (enables.pre_range) {
            new_final_range_timeout_mclks += timeouts.pre_range_mclks;
        }

        _vl53l0x_writeReg16Bit(dev, VL53L0X_REG_FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI, _vl53l0x_encodeTimeout(new_final_range_timeout_mclks));

        // set_sequence_step_timeout end
    }
    else {
        // invalid type
        return 0;
    }

    // "Finally, the timing budget must be re-applied"
    _vl53l0x_setMeasurementTimingBudget(dev, dev->measurement_timing_budget_us);

    // "Perform the phase calibration. This is needed after changing on vcsel period."
    // VL53L0X_perform_phase_calibration() begin

    uint8_t sequence_config = _vl53l0x_readReg(dev, VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG);
    _vl53l0x_writeReg(dev, VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG, 0x02);
    _vl53l0x_performSingleRefCalibration(dev, 0x0);
    _vl53l0x_writeReg(dev, VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG, sequence_config);

    // VL53L0X_perform_phase_calibration() end

    return 1;
}

// Get the VCSEL pulse period in PCLKs for the given period type.
// based on VL53L0X_get_vcsel_pulse_period()
static uint8_t _vl53l0x_getVcselPulsePeriod(vl53l0x_t *dev, vcselPeriodType_t type) {
    if (type == VcselPeriodPreRange) {
        return decodeVcselPeriod(_vl53l0x_readReg(dev, VL53L0X_REG_PRE_RANGE_CONFIG_VCSEL_PERIOD));
    }
    else if (type == VcselPeriodFinalRange) {
        return decodeVcselPeriod(_vl53l0x_readReg(dev, VL53L0X_REG_FINAL_RANGE_CONFIG_VCSEL_PERIOD));
    }
    else { 
        return 255;
    }
}

// Get sequence step enables
// based on VL53L0X_Getsequence_step_enables_t()
static void _vl53l0x_get_sequence_step_enables(vl53l0x_t *dev, sequence_step_enables_t * enables) {
  uint8_t sequence_config = _vl53l0x_readReg(dev, VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG);

  enables->tcc          = (sequence_config >> 4) & 0x1;
  enables->dss          = (sequence_config >> 3) & 0x1;
  enables->msrc         = (sequence_config >> 2) & 0x1;
  enables->pre_range    = (sequence_config >> 6) & 0x1;
  enables->final_range  = (sequence_config >> 7) & 0x1;
}

// Get sequence step timeouts
// based on get_sequence_step_timeout(),
// but gets all timeouts instead of just the requested one, and also stores
// intermediate values
static void _vl53l0x_get_sequence_step_timeouts(vl53l0x_t *dev, sequence_step_enables_t const * enables, sequence_step_timeouts_t * timeouts) {
    timeouts->pre_range_vcsel_period_pclks = _vl53l0x_getVcselPulsePeriod(dev, VcselPeriodPreRange);
    timeouts->msrc_dss_tcc_mclks = _vl53l0x_readReg(dev, VL53L0X_REG_MSRC_CONFIG_TIMEOUT_MACROP) + 1;
    timeouts->msrc_dss_tcc_us = _vl53l0x_timeoutMclksToMicroseconds(timeouts->msrc_dss_tcc_mclks, timeouts->pre_range_vcsel_period_pclks);
    timeouts->pre_range_mclks = _vl53l0x_decodeTimeout(_vl53l0x_readReg16Bit(dev, VL53L0X_REG_PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI));
    timeouts->pre_range_us = _vl53l0x_timeoutMclksToMicroseconds(timeouts->pre_range_mclks, timeouts->pre_range_vcsel_period_pclks);
    timeouts->final_range_vcsel_period_pclks = _vl53l0x_getVcselPulsePeriod(dev, VcselPeriodFinalRange);
    timeouts->final_range_mclks = _vl53l0x_decodeTimeout(_vl53l0x_readReg16Bit(dev, VL53L0X_REG_FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI));

    if (enables->pre_range) {
        timeouts->final_range_mclks -= timeouts->pre_range_mclks;
    }

    timeouts->final_range_us = _vl53l0x_timeoutMclksToMicroseconds(timeouts->final_range_mclks, timeouts->final_range_vcsel_period_pclks);
}

// Decode sequence step timeout in MCLKs from register value
// based on VL53L0X_decode_timeout()
// Note: the original function returned a uint32_t, but the return value is
// always stored in a uint16_t.
static uint16_t _vl53l0x_decodeTimeout(uint16_t reg_val) {
    // format: "(LSByte * 2^MSByte) + 1"
    return (uint16_t)((reg_val & 0x00FF) << (uint16_t)((reg_val & 0xFF00) >> 8)) + 1;
}

// Encode sequence step timeout register value from timeout in MCLKs
// based on VL53L0X_encode_timeout()
static uint16_t _vl53l0x_encodeTimeout(uint32_t timeout_mclks) {
    // format: "(LSByte * 2^MSByte) + 1"
    uint32_t ls_byte = 0;
    uint16_t ms_byte = 0;

    if (timeout_mclks > 0) {
        ls_byte = timeout_mclks - 1;
        while ((ls_byte & 0xFFFFFF00) > 0) {
            ls_byte >>= 1;
            ms_byte++;
        }

        return (ms_byte << 8) | (ls_byte & 0xFF);
    }
    else { 
        return 0;
    }
}

// Convert sequence step timeout from MCLKs to microseconds with given VCSEL period in PCLKs
// based on VL53L0X_calc_timeout_us()
static uint32_t _vl53l0x_timeoutMclksToMicroseconds(uint16_t timeout_period_mclks, uint8_t vcsel_period_pclks) {
    uint32_t macro_period_ns = calcMacroPeriod(vcsel_period_pclks);
    return ((timeout_period_mclks * macro_period_ns) + 500) / 1000;
}

// Convert sequence step timeout from microseconds to MCLKs with given VCSEL period in PCLKs
// based on VL53L0X_calc_timeout_mclks()
static uint32_t _vl53l0x_timeoutMicrosecondsToMclks(uint32_t timeout_period_us, uint8_t vcsel_period_pclks) {
    uint32_t macro_period_ns = calcMacroPeriod(vcsel_period_pclks);
    return (((timeout_period_us * 1000) + (macro_period_ns / 2)) / macro_period_ns);
}

// based on VL53L0X_perform_single_ref_calibration()
static void _vl53l0x_performSingleRefCalibration(vl53l0x_t *dev, uint8_t vhv_init_byte) {
    _vl53l0x_writeReg(dev, VL53L0X_REG_SYSRANGE_START, 0x01 | vhv_init_byte); // VL53L0X_REG_SYSRANGE_MODE_START_STOP
    while ((_vl53l0x_readReg(dev, VL53L0X_REG_RESULT_INTERRUPT_STATUS) & 0x07) == 0) {
    }
    _vl53l0x_writeReg(dev, VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
    _vl53l0x_writeReg(dev, VL53L0X_REG_SYSRANGE_START, 0x00);
}

uint32_t vl53l0x_init(vl53l0x_t *dev, i2c_t i2c, mutex_t *i2c_mutex, uint8_t voltage2v8) {
    assert(dev != 0);

    uint32_t ret = 0;
    uint8_t buffer[2];

    dev->i2c = i2c;
    dev->i2c_mutex = i2c_mutex;
    
    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    ret = i2c_master_read_mem(dev->i2c, VL53L0X_ADDRESS, VL53L0X_REG_IDENTIFICATION_MODEL_ID, 1, buffer, 1);

    if(ret != 0) {
        ret = EIO;
    }
    else {
        if(buffer[0] != VL53L0X_MODEL_ID) {
            ret = ENODEV;
        }
        else {
            dev->revision = _vl53l0x_readReg(dev, VL53L0X_REG_IDENTIFICATION_REVISION_ID);

            // VL53L0X_DataInit() begin
            // sensor uses 1V8 mode for I/O by default; switch to 2V8 mode if necessary
            if (voltage2v8) {
                _vl53l0x_writeReg(dev, VL53L0X_REG_VHV_CONFIG_PAD_SCL_SDA_EXTSUP_HV, _vl53l0x_readReg(dev, VL53L0X_REG_VHV_CONFIG_PAD_SCL_SDA_EXTSUP_HV) | 0x01); // set bit 0
            }

            // "Set I2C standard mode"
            _vl53l0x_writeReg(dev, 0x88, 0x00);

            _vl53l0x_writeReg(dev, 0x80, 0x01);
            _vl53l0x_writeReg(dev, 0xFF, 0x01);
            _vl53l0x_writeReg(dev, 0x00, 0x00);
            dev->stop_variable = _vl53l0x_readReg(dev, 0x91);
            _vl53l0x_writeReg(dev, 0x00, 0x01);
            _vl53l0x_writeReg(dev, 0xFF, 0x00);
            _vl53l0x_writeReg(dev, 0x80, 0x00);

            // disable SIGNAL_RATE_MSRC (bit 1) and SIGNAL_RATE_PRE_RANGE (bit 4) limit checks
            _vl53l0x_writeReg(dev, VL53L0X_REG_MSRC_CONFIG_CONTROL, _vl53l0x_readReg(dev, VL53L0X_REG_MSRC_CONFIG_CONTROL) | 0x12);

            // set final range signal rate limit to 0.25 MCPS (million counts per second. 1 mcps = 128)
            _vl53l0x_writeReg16Bit(dev, VL53L0X_REG_FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT, 32);
            _vl53l0x_writeReg(dev, VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG, 0xFF);
            // VL53L0X_DataInit() end

            // VL53L0X_StaticInit() begin
            uint8_t spad_count;
            uint8_t spad_type_is_aperture;
            _vl53l0x_writeReg(dev, 0x80, 0x01);
            _vl53l0x_writeReg(dev, 0xFF, 0x01);
            _vl53l0x_writeReg(dev, 0x00, 0x00);

            _vl53l0x_writeReg(dev, 0xFF, 0x06);
            _vl53l0x_writeReg(dev, 0x83, _vl53l0x_readReg(dev, 0x83) | 0x04);
            _vl53l0x_writeReg(dev, 0xFF, 0x07);
            _vl53l0x_writeReg(dev, 0x81, 0x01);

            _vl53l0x_writeReg(dev, 0x80, 0x01);

            _vl53l0x_writeReg(dev, 0x94, 0x6b);
            _vl53l0x_writeReg(dev, 0x83, 0x00);
            while (_vl53l0x_readReg(dev, 0x83) == 0x00) {
            }
            _vl53l0x_writeReg(dev, 0x83, 0x01);
            spad_type_is_aperture = _vl53l0x_readReg(dev, 0x92);

            spad_count = spad_type_is_aperture & 0x7f;
            spad_type_is_aperture = (spad_type_is_aperture >> 7) & 0x01;

            _vl53l0x_writeReg(dev, 0x81, 0x00);
            _vl53l0x_writeReg(dev, 0xFF, 0x06);
            _vl53l0x_writeReg(dev, 0x83, _vl53l0x_readReg(dev, 0x83) & ~0x04);
            _vl53l0x_writeReg(dev, 0xFF, 0x01);
            _vl53l0x_writeReg(dev, 0x00, 0x01);
            _vl53l0x_writeReg(dev, 0xFF, 0x00);
            _vl53l0x_writeReg(dev, 0x80, 0x00);

            // The SPAD map (RefGoodSpadMap) is read by VL53L0X_get_info_from_device() in
            // the API, but the same data seems to be more easily readable from
            // VL53L0X_REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_0 through _6, so read it from there
            uint8_t ref_spad_map[6];
            i2c_master_read_mem(dev->i2c, VL53L0X_ADDRESS, VL53L0X_REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_0, 1, ref_spad_map, 6);

            // -- VL53L0X_set_reference_spads() begin (assume NVM values are valid)
            _vl53l0x_writeReg(dev, 0xFF, 0x01);
            _vl53l0x_writeReg(dev, VL53L0X_REG_DYNAMIC_SPAD_REF_EN_START_OFFSET, 0x00);
            _vl53l0x_writeReg(dev, VL53L0X_REG_DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD, 0x2C);
            _vl53l0x_writeReg(dev, 0xFF, 0x00);
            _vl53l0x_writeReg(dev, VL53L0X_REG_GLOBAL_CONFIG_REF_EN_START_SELECT, 0xB4);

            uint8_t first_spad_to_enable = spad_type_is_aperture ? 12 : 0; // 12 is the first aperture spad
            uint8_t spads_enabled = 0;

            for (uint8_t i = 0; i < 48; i++) {
                if (i < first_spad_to_enable || spads_enabled == spad_count) {
                    // This bit is lower than the first one that should be enabled, or
                    // (reference_spad_count) bits have already been enabled, so zero this bit
                    ref_spad_map[i / 8] &= ~(1 << (i % 8));
                }
                else if ((ref_spad_map[i / 8] >> (i % 8)) & 0x1) {
                    spads_enabled++;
                }
            }

            i2c_master_write_mem(dev->i2c, VL53L0X_ADDRESS, VL53L0X_REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_0, 1, ref_spad_map, 6);
            // -- VL53L0X_set_reference_spads() end

            // -- VL53L0X_load_tuning_settings() begin
            // DefaultTuningSettings from vl53l0x_tuning.h
            _vl53l0x_writeReg(dev, 0xFF, 0x01);
            _vl53l0x_writeReg(dev, 0x00, 0x00);

            _vl53l0x_writeReg(dev, 0xFF, 0x00);
            _vl53l0x_writeReg(dev, 0x09, 0x00);
            _vl53l0x_writeReg(dev, 0x10, 0x00);
            _vl53l0x_writeReg(dev, 0x11, 0x00);

            _vl53l0x_writeReg(dev, 0x24, 0x01);
            _vl53l0x_writeReg(dev, 0x25, 0xFF);
            _vl53l0x_writeReg(dev, 0x75, 0x00);

            _vl53l0x_writeReg(dev, 0xFF, 0x01);
            _vl53l0x_writeReg(dev, 0x4E, 0x2C);
            _vl53l0x_writeReg(dev, 0x48, 0x00);
            _vl53l0x_writeReg(dev, 0x30, 0x20);

            _vl53l0x_writeReg(dev, 0xFF, 0x00);
            _vl53l0x_writeReg(dev, 0x30, 0x09);
            _vl53l0x_writeReg(dev, 0x54, 0x00);
            _vl53l0x_writeReg(dev, 0x31, 0x04);
            _vl53l0x_writeReg(dev, 0x32, 0x03);
            _vl53l0x_writeReg(dev, 0x40, 0x83);
            _vl53l0x_writeReg(dev, 0x46, 0x25);
            _vl53l0x_writeReg(dev, 0x60, 0x00);
            _vl53l0x_writeReg(dev, 0x27, 0x00);
            _vl53l0x_writeReg(dev, 0x50, 0x06);
            _vl53l0x_writeReg(dev, 0x51, 0x00);
            _vl53l0x_writeReg(dev, 0x52, 0x96);
            _vl53l0x_writeReg(dev, 0x56, 0x08);
            _vl53l0x_writeReg(dev, 0x57, 0x30);
            _vl53l0x_writeReg(dev, 0x61, 0x00);
            _vl53l0x_writeReg(dev, 0x62, 0x00);
            _vl53l0x_writeReg(dev, 0x64, 0x00);
            _vl53l0x_writeReg(dev, 0x65, 0x00);
            _vl53l0x_writeReg(dev, 0x66, 0xA0);

            _vl53l0x_writeReg(dev, 0xFF, 0x01);
            _vl53l0x_writeReg(dev, 0x22, 0x32);
            _vl53l0x_writeReg(dev, 0x47, 0x14);
            _vl53l0x_writeReg(dev, 0x49, 0xFF);
            _vl53l0x_writeReg(dev, 0x4A, 0x00);

            _vl53l0x_writeReg(dev, 0xFF, 0x00);
            _vl53l0x_writeReg(dev, 0x7A, 0x0A);
            _vl53l0x_writeReg(dev, 0x7B, 0x00);
            _vl53l0x_writeReg(dev, 0x78, 0x21);

            _vl53l0x_writeReg(dev, 0xFF, 0x01);
            _vl53l0x_writeReg(dev, 0x23, 0x34);
            _vl53l0x_writeReg(dev, 0x42, 0x00);
            _vl53l0x_writeReg(dev, 0x44, 0xFF);
            _vl53l0x_writeReg(dev, 0x45, 0x26);
            _vl53l0x_writeReg(dev, 0x46, 0x05);
            _vl53l0x_writeReg(dev, 0x40, 0x40);
            _vl53l0x_writeReg(dev, 0x0E, 0x06);
            _vl53l0x_writeReg(dev, 0x20, 0x1A);
            _vl53l0x_writeReg(dev, 0x43, 0x40);

            _vl53l0x_writeReg(dev, 0xFF, 0x00);
            _vl53l0x_writeReg(dev, 0x34, 0x03);
            _vl53l0x_writeReg(dev, 0x35, 0x44);

            _vl53l0x_writeReg(dev, 0xFF, 0x01);
            _vl53l0x_writeReg(dev, 0x31, 0x04);
            _vl53l0x_writeReg(dev, 0x4B, 0x09);
            _vl53l0x_writeReg(dev, 0x4C, 0x05);
            _vl53l0x_writeReg(dev, 0x4D, 0x04);

            _vl53l0x_writeReg(dev, 0xFF, 0x00);
            _vl53l0x_writeReg(dev, 0x44, 0x00);
            _vl53l0x_writeReg(dev, 0x45, 0x20);
            _vl53l0x_writeReg(dev, 0x47, 0x08);
            _vl53l0x_writeReg(dev, 0x48, 0x28);
            _vl53l0x_writeReg(dev, 0x67, 0x00);
            _vl53l0x_writeReg(dev, 0x70, 0x04);
            _vl53l0x_writeReg(dev, 0x71, 0x01);
            _vl53l0x_writeReg(dev, 0x72, 0xFE);
            _vl53l0x_writeReg(dev, 0x76, 0x00);
            _vl53l0x_writeReg(dev, 0x77, 0x00);

            _vl53l0x_writeReg(dev, 0xFF, 0x01);
            _vl53l0x_writeReg(dev, 0x0D, 0x01);

            _vl53l0x_writeReg(dev, 0xFF, 0x00);
            _vl53l0x_writeReg(dev, 0x80, 0x01);
            _vl53l0x_writeReg(dev, 0x01, 0xF8);

            _vl53l0x_writeReg(dev, 0xFF, 0x01);
            _vl53l0x_writeReg(dev, 0x8E, 0x01);
            _vl53l0x_writeReg(dev, 0x00, 0x01);
            _vl53l0x_writeReg(dev, 0xFF, 0x00);
            _vl53l0x_writeReg(dev, 0x80, 0x00);
            // -- VL53L0X_load_tuning_settings() end

            // "Set interrupt config to new sample ready"
            // -- VL53L0X_SetGpioConfig() begin
            _vl53l0x_writeReg(dev, VL53L0X_REG_SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04);
            _vl53l0x_writeReg(dev, VL53L0X_REG_GPIO_HV_MUX_ACTIVE_HIGH, _vl53l0x_readReg(dev, VL53L0X_REG_GPIO_HV_MUX_ACTIVE_HIGH) & ~0x10); // active low
            _vl53l0x_writeReg(dev, VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
            // -- VL53L0X_SetGpioConfig() end

            dev->measurement_timing_budget_us = _vl53l0x_getMeasurementTimingBudget(dev);

            // "Disable MSRC and TCC by default"
            // MSRC = Minimum Signal Rate Check
            // TCC = Target CentreCheck
            // -- VL53L0X_SetSequenceStepEnable() begin
            _vl53l0x_writeReg(dev, VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG, 0xE8);
            // -- VL53L0X_SetSequenceStepEnable() end

            // "Recalculate timing budget"
            _vl53l0x_setMeasurementTimingBudget(dev, dev->measurement_timing_budget_us);

            // VL53L0X_StaticInit() end

            // VL53L0X_PerformRefCalibration() begin (VL53L0X_perform_ref_calibration())

            // -- VL53L0X_perform_vhv_calibration() begin
            _vl53l0x_writeReg(dev, VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG, 0x01);
            _vl53l0x_performSingleRefCalibration(dev, 0x40);
            // -- VL53L0X_perform_vhv_calibration() end

            // -- VL53L0X_perform_phase_calibration() begin
            _vl53l0x_writeReg(dev, VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG, 0x02);
            _vl53l0x_performSingleRefCalibration(dev, 0x00);
            // -- VL53L0X_perform_phase_calibration() end

            // "restore the previous Sequence Config"
            _vl53l0x_writeReg(dev, VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG, 0xE8);
            // VL53L0X_PerformRefCalibration() end
        }   
    }

    dev->mode = VL53L0X_MODE_OFF;

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}

uint32_t vl53l0x_turn_on(vl53l0x_t *dev, vl53l0x_mode_t mode, uint32_t cont_period_ms, uint16_t limit_Mcps, uint32_t timing_budget, uint8_t prerange_period_pclks, uint8_t finalrange_period_pclks) {
    assert(dev != 0);

    uint32_t ret = 0;
    
    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    // set final range signal rate limit to limit_Mcps (million counts per second. 1 mcps = 128)
    _vl53l0x_writeReg16Bit(dev, VL53L0X_REG_FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT, limit_Mcps);

    // set laser pulse periods
    _vl53l0x_setVcselPulsePeriod(dev, VcselPeriodPreRange, prerange_period_pclks);
    _vl53l0x_setVcselPulsePeriod(dev, VcselPeriodFinalRange, finalrange_period_pclks);

    // "Recalculate timing budget"
    _vl53l0x_setMeasurementTimingBudget(dev, timing_budget);

    dev->mode = mode;

    switch(mode) {
        case VL53L0X_MODE_SINGLE:
            break;

        case VL53L0X_MODE_CONTINUOUS:
            _vl53l0x_writeReg(dev, 0x80, 0x01);
            _vl53l0x_writeReg(dev, 0xFF, 0x01);
            _vl53l0x_writeReg(dev, 0x00, 0x00);
            _vl53l0x_writeReg(dev, 0x91, dev->stop_variable);
            _vl53l0x_writeReg(dev, 0x00, 0x01);
            _vl53l0x_writeReg(dev, 0xFF, 0x00);
            _vl53l0x_writeReg(dev, 0x80, 0x00);

            if (cont_period_ms != 0) {
                // continuous timed mode
                // VL53L0X_SetInterMeasurementPeriodMilliSeconds() begin
                uint16_t osc_calibrate_val = _vl53l0x_readReg16Bit(dev, VL53L0X_REG_OSC_CALIBRATE_VAL);
                if (osc_calibrate_val != 0) {
                    cont_period_ms *= osc_calibrate_val;
                }
                _vl53l0x_writeReg32Bit(dev, VL53L0X_REG_SYSTEM_INTERMEASUREMENT_PERIOD, cont_period_ms);
                // VL53L0X_SetInterMeasurementPeriodMilliSeconds() end
                _vl53l0x_writeReg(dev, VL53L0X_REG_SYSRANGE_START, 0x04); // VL53L0X_REG_SYSRANGE_MODE_TIMED
            }
            else {
                // continuous back-to-back mode
                _vl53l0x_writeReg(dev, VL53L0X_REG_SYSRANGE_START, 0x02); // VL53L0X_REG_SYSRANGE_MODE_BACKTOBACK
            }
            break;

        default:
            ret = 1;
            break;
    }

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}

uint32_t vl53l0x_turn_on_default(vl53l0x_t *dev, vl53l0x_mode_t mode, uint32_t cont_period_ms) {
    return vl53l0x_turn_on(dev, mode, cont_period_ms, 32, 200000, 14, 10);
}

uint32_t vl53l0x_turn_off(vl53l0x_t *dev) {
    assert(dev != 0);

    uint32_t ret = 0;
    
    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    if(dev->mode == VL53L0X_MODE_CONTINUOUS) {
        _vl53l0x_writeReg(dev, VL53L0X_REG_SYSRANGE_START, 0x01); // VL53L0X_REG_SYSRANGE_MODE_SINGLESHOT

        _vl53l0x_writeReg(dev, 0xFF, 0x01);
        _vl53l0x_writeReg(dev, 0x00, 0x00);
        _vl53l0x_writeReg(dev, 0x91, 0x00);
        _vl53l0x_writeReg(dev, 0x00, 0x01);
        _vl53l0x_writeReg(dev, 0xFF, 0x00);
    }

    dev->mode = VL53L0X_MODE_OFF;

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}

uint32_t vl53l0x_single_read_trigger(vl53l0x_t *dev) {
    assert(dev != 0);

    uint32_t ret = 0;
    
    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    _vl53l0x_writeReg(dev, 0x80, 0x01);
    _vl53l0x_writeReg(dev, 0xFF, 0x01);
    _vl53l0x_writeReg(dev, 0x00, 0x00);
    _vl53l0x_writeReg(dev, 0x91, dev->stop_variable);
    _vl53l0x_writeReg(dev, 0x00, 0x01);
    _vl53l0x_writeReg(dev, 0xFF, 0x00);
    _vl53l0x_writeReg(dev, 0x80, 0x00);

    _vl53l0x_writeReg(dev, VL53L0X_REG_SYSRANGE_START, 0x01);

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}

uint32_t vl53l0x_read(vl53l0x_t *dev, uint16_t *mm) {
    assert(dev != 0);
    assert(mm != 0);

    uint32_t ret = 0;
    
    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    if((_vl53l0x_readReg(dev, VL53L0X_REG_RESULT_INTERRUPT_STATUS) & 0x07) == 0) {
        ret = -1;
    }

    // assumptions: Linearity Corrective Gain is 1000 (default);
    // fractional ranging is not enabled
    *mm = _vl53l0x_readReg16Bit(dev, VL53L0X_REG_RESULT_RANGE_STATUS + 10);
    _vl53l0x_writeReg(dev, VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01);

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}
