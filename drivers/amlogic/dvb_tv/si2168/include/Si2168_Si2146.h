#ifndef  TUNER_Si2146_SPECIFICS
#define  TUNER_Si2146_SPECIFICS

    #include "Si2146_L2_API.h"

#ifdef    SILABS_EVB
    #define Si2168_CLOCK_MODE_TER              Si2168_START_CLK_CMD_CLK_MODE_CLK_CLKIO
    #define Si2168_REF_FREQUENCY_TER           24
#else  /* SILABS_EVB */
    #define Si2168_CLOCK_MODE_TER              Si2168_START_CLK_CMD_CLK_MODE_CLK_CLKIO
    #define Si2168_REF_FREQUENCY_TER           24
#endif /* SILABS_EVB */

    #define TUNER_ADDRESS_TER           0xc0
    #define TUNER_IF_MHZ_TER            5.000000
    #define LIF_MODE_TER
    #define TUNERTER_API

    #define CONTROL_IF_AGC
    /* TER_IF_AGC_POLA : use 'mp_a_not_inverted' when 'gain vs voltage' curve slope is positive, 'mp_a_inverted' otherwise */
    #define TER_IF_AGC_POLA             mp_a_inv_not_inverted

    #define TER_TUNER_CONTEXT            L1_Si2146_Context
    #define TUNER_ADDRESS_TER            0xc0

    #define TER_TUNER_INIT(api)           Si2146_Init(api)
    #define TER_TUNER_WAKEUP(api)         Si2146_pollForCTS(api)
    #define TER_TUNER_STANDBY(api)        Si2146_L1_STANDBY(api)
    #define TER_TUNER_CLOCK_OFF(api)      Si2146_ClockOff(api)
    #define TER_TUNER_CLOCK_ON(api)       Si2146_ClockOn(api)
    #define TER_TUNER_ERROR_TEXT(res)     Si2146_L1_API_ERROR_TEXT(res)
    #define TER_TUNER_MENU(full)          Si2146_menu(full)
    #define TER_TUNER_LOOP(api,choice)    Si2146_demoLoop(api,choice)

    #define TER_TUNER_MODULATION_DVBT     Si2146_DTV_MODE_PROP_MODULATION_DVBT
    #define TER_TUNER_MODULATION_DVBC     Si2146_DTV_MODE_PROP_MODULATION_DVBC

    #define L1_RF_TER_TUNER_Init(api,add) Si2146_L1_API_Init(api, add);
    #define L1_RF_TER_TUNER_Tune(api,rf)  Si2146_DTVTune(api, rf, bw, modulation, Si2146_DTV_MODE_PROP_INVERT_SPECTRUM_NORMAL)

    #define L1_RF_TER_TUNER_Get_RF

    #define L1_RF_TER_TUNER_MODULATION_DVBC  Si2146_DTV_MODE_PROP_MODULATION_DVBC
    #define L1_RF_TER_TUNER_MODULATION_DVBT  Si2146_DTV_MODE_PROP_MODULATION_DVBT
    #define L1_RF_TER_TUNER_MODULATION_DVBT2 Si2146_DTV_MODE_PROP_MODULATION_DVBT
    #define   L1_RF_TER_TUNER_FEF_MODE_FREEZE_PIN_SETUP(api) \
    api->cmd->config_pins.gpio1_mode               = Si2146_CONFIG_PINS_CMD_GPIO1_MODE_DISABLE;\
    api->cmd->config_pins.gpio1_read               = Si2146_CONFIG_PINS_CMD_GPIO1_READ_DO_NOT_READ;\
    api->cmd->config_pins.gpio2_mode               = Si2146_CONFIG_PINS_CMD_GPIO2_MODE_DISABLE;\
    api->cmd->config_pins.gpio2_read               = Si2146_CONFIG_PINS_CMD_GPIO2_READ_DO_NOT_READ;\
    api->cmd->config_pins.gpio3_mode               = Si2146_CONFIG_PINS_CMD_GPIO3_MODE_DISABLE;\
    api->cmd->config_pins.gpio3_read               = Si2146_CONFIG_PINS_CMD_GPIO3_READ_DO_NOT_READ;\
    api->cmd->config_pins.bclk1_mode               = Si2146_CONFIG_PINS_CMD_BCLK1_MODE_NO_CHANGE;\
    api->cmd->config_pins.bclk1_read               = Si2146_CONFIG_PINS_CMD_BCLK1_READ_DO_NOT_READ;\
    api->cmd->config_pins.xout_mode                = Si2146_CONFIG_PINS_CMD_XOUT_MODE_NO_CHANGE;\
    api->prop->dtv_initial_agc_speed.agc_decim     = Si2146_DTV_INITIAL_AGC_SPEED_PROP_AGC_DECIM_OFF;\
    api->prop->dtv_initial_agc_speed.if_agc_speed  = Si2146_DTV_INITIAL_AGC_SPEED_PROP_IF_AGC_SPEED_AUTO;\
    api->prop->dtv_initial_agc_speed_period.period = 0;\
    api->prop->dtv_agc_speed.agc_decim             = Si2146_DTV_AGC_SPEED_PROP_AGC_DECIM_OFF;\
    api->prop->dtv_agc_speed.if_agc_speed          = Si2146_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_AUTO;\
    Si2146_L1_SendCommand2(api, Si2146_CONFIG_PINS_CMD_CODE);\
    Si2146_L1_SetProperty2(api, Si2146_DTV_INITIAL_AGC_SPEED_PERIOD_PROP_CODE);\
    Si2146_L1_SetProperty2(api, Si2146_DTV_AGC_SPEED_PROP_CODE);

#ifdef    Si2146_DTV_AGC_FREEZE_INPUT_PROP_LEVEL_HIGH
    #define   L1_RF_TER_TUNER_FEF_MODE_FREEZE_PIN(api,fef) \
    api->prop->dtv_agc_freeze_input.level          = Si2146_DTV_AGC_FREEZE_INPUT_PROP_LEVEL_HIGH;\
    if (fef == 0) {\
      api->prop->dtv_agc_freeze_input.pin          = Si2146_DTV_AGC_FREEZE_INPUT_PROP_PIN_NONE;\
    } else {\
      api->prop->dtv_agc_freeze_input.pin          = Si2146_DTV_AGC_FREEZE_INPUT_PROP_PIN_GPIO1;\
    }\
    Si2146_L1_SetProperty2(api, Si2146_DTV_AGC_FREEZE_INPUT_PROP_CODE);
#endif /* Si2146_DTV_AGC_FREEZE_INPUT_PROP_LEVEL_HIGH */

    #define   L1_RF_TER_TUNER_FEF_MODE_SLOW_INITIAL_AGC_SETUP(api,fef) \
      if (fef == 0) {\
        api->prop->dtv_initial_agc_speed.agc_decim     = Si2146_DTV_INITIAL_AGC_SPEED_PROP_AGC_DECIM_OFF;\
        api->prop->dtv_initial_agc_speed.if_agc_speed  = Si2146_DTV_INITIAL_AGC_SPEED_PROP_IF_AGC_SPEED_AUTO;\
        api->prop->dtv_initial_agc_speed_period.period = 0;\
        api->prop->dtv_agc_speed.agc_decim             = Si2146_DTV_AGC_SPEED_PROP_AGC_DECIM_OFF;\
        api->prop->dtv_agc_speed.if_agc_speed          = Si2146_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_AUTO;\
      } else {\
        api->prop->dtv_initial_agc_speed.agc_decim     = Si2146_DTV_INITIAL_AGC_SPEED_PROP_AGC_DECIM_OFF;\
        api->prop->dtv_initial_agc_speed.if_agc_speed  = Si2146_DTV_INITIAL_AGC_SPEED_PROP_IF_AGC_SPEED_AUTO;\
        api->prop->dtv_initial_agc_speed_period.period = 310;\
        api->prop->dtv_agc_speed.agc_decim             = Si2146_DTV_AGC_SPEED_PROP_AGC_DECIM_4;\
        api->prop->dtv_agc_speed.if_agc_speed          = Si2146_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_39;\
      }\
      Si2146_L1_SetProperty2(api, Si2146_DTV_INITIAL_AGC_SPEED_PERIOD_PROP_CODE);\
      Si2146_L1_SetProperty2(api, Si2146_DTV_AGC_SPEED_PROP_CODE);


    #define   L1_RF_TER_TUNER_FEF_MODE_SLOW_NORMAL_AGC_SETUP(api,fef) \
      api->prop->dtv_initial_agc_speed_period.period = 0;\
      if (fef == 0) {\
        api->prop->dtv_agc_speed.agc_decim             = Si2146_DTV_AGC_SPEED_PROP_AGC_DECIM_OFF;\
        api->prop->dtv_agc_speed.if_agc_speed          = Si2146_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_AUTO;\
      } else {\
        api->prop->dtv_agc_speed.if_agc_speed          = Si2146_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_39;\
        api->prop->dtv_agc_speed.agc_decim             = Si2146_DTV_AGC_SPEED_PROP_AGC_DECIM_4;\
      }\
      Si2146_L1_SetProperty2(api, Si2146_DTV_INITIAL_AGC_SPEED_PERIOD_PROP_CODE);\
      Si2146_L1_SetProperty2(api, Si2146_DTV_AGC_SPEED_PROP_CODE);

    #define   L1_RF_TER_TUNER_FEF_MODE_SLOW_NORMAL_AGC(api,fef) \
      if (fef == 0) {\
        api->prop->dtv_agc_speed.agc_decim             = Si2146_DTV_AGC_SPEED_PROP_AGC_DECIM_OFF;\
        api->prop->dtv_agc_speed.if_agc_speed          = Si2146_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_AUTO;\
      } else {\
        api->prop->dtv_agc_speed.if_agc_speed          = Si2146_DTV_AGC_SPEED_PROP_IF_AGC_SPEED_39;\
        api->prop->dtv_agc_speed.agc_decim             = Si2146_DTV_AGC_SPEED_PROP_AGC_DECIM_4;\
      }\
      Si2146_L1_SetProperty2(api, Si2146_DTV_AGC_SPEED_PROP_CODE);

    #define TER_TUNER_ATV_LO_INJECTION(api) \
      api->prop->tuner_lo_injection.band_1 = Si2146_TUNER_LO_INJECTION_PROP_BAND_1_HIGH_SIDE;\
      api->prop->tuner_lo_injection.band_2 = Si2146_TUNER_LO_INJECTION_PROP_BAND_2_HIGH_SIDE;\
      api->prop->tuner_lo_injection.band_3 = Si2146_TUNER_LO_INJECTION_PROP_BAND_3_HIGH_SIDE;\
      Si2146_L1_SetProperty2(api, Si2146_TUNER_LO_INJECTION_PROP_CODE);

    #define TER_TUNER_DTV_LO_INJECTION(api) \
      api->prop->tuner_lo_injection.band_1 = Si2146_TUNER_LO_INJECTION_PROP_BAND_1_HIGH_SIDE;\
      api->prop->tuner_lo_injection.band_2 = Si2146_TUNER_LO_INJECTION_PROP_BAND_2_LOW_SIDE;\
      api->prop->tuner_lo_injection.band_3 = Si2146_TUNER_LO_INJECTION_PROP_BAND_3_LOW_SIDE;\
      Si2146_L1_SetProperty2(api, Si2146_TUNER_LO_INJECTION_PROP_CODE);

#endif /* TUNER_Si2146_SPECIFICS */












