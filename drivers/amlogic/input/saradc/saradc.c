#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/saradc.h>
#include <linux/delay.h>
#include "saradc_reg.h"
#ifdef CONFIG_MESON_CPU_TEMP_SENSOR
#include <mach/cpu.h>
#endif
//#define ENABLE_CALIBRATION

#ifdef CONFIG_TEMPERATURE_SENSOR_GOVERNOR
static int sar_ch6_mode = 0;                        // 0: normal measure, 1:temperature measure
struct mutex ch6_lock;
#endif

struct saradc {
	spinlock_t lock;
	struct calibration *cal;
	int cal_num;

};

static struct saradc *gp_saradc;

#define CHAN_XP	CHAN_0
#define CHAN_YP	CHAN_1
#define CHAN_XN	CHAN_2
#define CHAN_YN	CHAN_3

#define INTERNAL_CAL_NUM	5

static u8 chan_mux[SARADC_CHAN_NUM] = {0,1,2,3,4,5,6,7};


static void saradc_reset(void)
{
	int i;

	//set adc clock as 1.28Mhz
	set_clock_divider(20);
	enable_clock();
	enable_adc();

	set_sample_mode(DIFF_MODE);
	set_tempsen(0);
	disable_fifo_irq();
	disable_continuous_sample();
	disable_chan0_delta();
	disable_chan1_delta();

	set_input_delay(10, INPUT_DELAY_TB_1US);
	set_sample_delay(10, SAMPLE_DELAY_TB_1US);
	set_block_delay(10, BLOCK_DELAY_TB_1US);
	
	// channels sampling mode setting
	for(i=0; i<SARADC_CHAN_NUM; i++) {
		set_sample_sw(i, IDLE_SW);
		set_sample_mux(i, chan_mux[i]);
	}
	
	// idle mode setting
	set_idle_sw(IDLE_SW);
	set_idle_mux(chan_mux[CHAN_0]);
	
	// detect mode setting
	set_detect_sw(DETECT_SW);
	set_detect_mux(chan_mux[CHAN_0]);
	disable_detect_sw();
	disable_detect_pullup();
	set_detect_irq_pol(0);
	disable_detect_irq();

//	set_sc_phase();
	enable_sample_engine();

//	printk("ADCREG reg0 =%x\n", get_reg(SAR_ADC_REG0));
//	printk("ADCREG ch list =%x\n", get_reg(SAR_ADC_CHAN_LIST));
//	printk("ADCREG avg =%x\n", get_reg(SAR_ADC_AVG_CNTL));
//	printk("ADCREG reg3=%x\n", get_reg(SAR_ADC_REG3));
//	printk("ADCREG ch72 sw=%x\n", get_reg(SAR_ADC_AUX_SW));
//	printk("ADCREG ch10 sw=%x\n", get_reg(SAR_ADC_CHAN_10_SW));
//	printk("ADCREG detect&idle=%x\n", get_reg(SAR_ADC_DETECT_IDLE_SW));
}

#ifdef ENABLE_CALIBRATION
static int  saradc_internal_cal(struct calibration *cal)
{
	int i;
	int voltage[] = {CAL_VOLTAGE_1, CAL_VOLTAGE_2, CAL_VOLTAGE_3, CAL_VOLTAGE_4, CAL_VOLTAGE_5};
	int ref[] = {0, 256, 512, 768, 1023};
	
//	set_cal_mux(MUX_CAL);
//	enable_cal_res_array();	
	for (i=0; i<INTERNAL_CAL_NUM; i++) {
		set_cal_voltage(voltage[i]);
		msleep(100);
		cal[i].ref = ref[i];
		cal[i].val = get_adc_sample(CHAN_7);
		printk(KERN_INFO "cal[%d]=%d\n", i, cal[i].val);
		if (cal[i].val < 0) {
			return -1;
		}
	}
	
	return 0;
}

static int saradc_get_cal_value(struct calibration *cal, int num, int val)
{
	int ret = -1;
	int i;
	
	if (num < 2)
		return val;
		
	if (val <= cal[0].val)
		return cal[0].ref;

	if (val >= cal[num-1].val)
		return cal[num-1].ref;
	
	for (i=0; i<num-1; i++) {
		if (val < cal[i+1].val) {
			ret = val - cal[i].val;
			ret *= cal[i+1].ref - cal[i].ref;
			ret /= cal[i+1].val - cal[i].val;
			ret += cal[i].ref;
			break;
		}
	}
	return ret;
}
#endif

static int last_value[] = {-1,-1,-1,-1,-1 ,-1,-1 ,-1};
static u8 print_flag = 0; //(1<<CHAN_4)
int get_adc_sample(int chan)
{
	int count;
	int value=-1;
	int sum;
	int changed;
	
	if (!gp_saradc)
		return -1;
		
	spin_lock(&gp_saradc->lock);

	set_chan_list(chan, 1);
	set_avg_mode(chan, NO_AVG_MODE, SAMPLE_NUM_8);
	set_sample_mux(chan, chan_mux[chan]);
	set_detect_mux(chan_mux[chan]);
	set_idle_mux(chan_mux[chan]); // for revb
	enable_sample_engine();
	start_sample();

	// Read any CBUS register to delay one clock cycle after starting the sampling engine
	// The bus is really fast and we may miss that it started
	{ count = get_reg(ISA_TIMERE); }

	count = 0;
	while (delta_busy() || sample_busy() || avg_busy()) {
		if (++count > 10000) {
			printk(KERN_ERR "ADC busy error=%x.\n", READ_CBUS_REG(SAR_ADC_REG0));
			goto end;
		}
	}
    stop_sample();
    
    sum = 0;
    count = 0;
    value = get_fifo_sample();
	while (get_fifo_cnt()) {
        value = get_fifo_sample() & 0x3ff;
        //if ((value != 0x1fe) && (value != 0x1ff)) {
			sum += value & 0x3ff;
            count++;
        //}
	}
	value = (count) ? (sum / count) : (-1);

end:
	changed = (abs(value-last_value[chan])<3) ? 0 : 1;
	if (changed && ((print_flag>>chan)&1)) {
		printk("before cal: ch%d = %d\n", chan, value);
		last_value[chan] = value;
	}
#ifdef ENABLE_CALIBRATION
	if (gp_saradc->cal_num) {
		value = saradc_get_cal_value(gp_saradc->cal, gp_saradc->cal_num, value);
		if (changed && ((print_flag>>chan)&1))
			printk("after cal: ch%d = %d\n\n", chan, value);
	}
#endif
	disable_sample_engine();
//	set_sc_phase();
	spin_unlock(&gp_saradc->lock);
	return value;
}

int saradc_ts_service(int cmd)
{
	int value = -1;
	
	switch (cmd) {
	case CMD_GET_X:
		//set_sample_sw(CHAN_YP, X_SW);
		value = get_adc_sample(CHAN_YP);
		set_sample_sw(CHAN_XP, Y_SW); // preset for y
		break;

	case CMD_GET_Y:
		//set_sample_sw(CHAN_XP, Y_SW);
		value = get_adc_sample(CHAN_XP);
		break;

	case CMD_GET_Z1:
		set_sample_sw(CHAN_XP, Z1_SW);
		value = get_adc_sample(CHAN_XP);
		break;

	case CMD_GET_Z2:
		set_sample_sw(CHAN_YN, Z2_SW);
		value = get_adc_sample(CHAN_YN);
		break;

	case CMD_GET_PENDOWN:
		value = !detect_level();
		set_sample_sw(CHAN_YP, X_SW); // preset for x
		break;
	
	case CMD_INIT_PENIRQ:
		enable_detect_pullup();
		enable_detect_sw();
		value = 0;
		printk(KERN_INFO "init penirq ok\n");
		break;

	case CMD_SET_PENIRQ:
		enable_detect_pullup();
		enable_detect_sw();
		value = 0;
		break;
		
	case CMD_CLEAR_PENIRQ:
		disable_detect_pullup();
		disable_detect_sw();
		value = 0;
		break;

	default:
		break;		
	}
	
	return value;
}

static ssize_t saradc_ch0_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", get_adc_sample(0));
}
static ssize_t saradc_ch1_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", get_adc_sample(1));
}
static ssize_t saradc_ch2_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", get_adc_sample(2));
}
static ssize_t saradc_ch3_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", get_adc_sample(3));
}
static ssize_t saradc_ch4_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", get_adc_sample(4));
}
static ssize_t saradc_ch5_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", get_adc_sample(5));
}

static ssize_t saradc_ch7_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", get_adc_sample(7));
}
static ssize_t saradc_print_flag_store(struct class *cla, struct class_attribute *attr, const char *buf, size_t count)
{
		sscanf(buf, "%x", (int*)&print_flag);
    printk("print_flag=%d\n", print_flag);
    return count;
}
#ifndef CONFIG_MESON_CPU_TEMP_SENSOR
static ssize_t saradc_ch6_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", get_adc_sample(6));
}
static ssize_t saradc_temperature_store(struct class *cla, struct class_attribute *attr, const char *buf, size_t count)
{
		u8 tempsen;
		sscanf(buf, "%x", (int*)&tempsen);
#ifdef CONFIG_TEMPERATURE_SENSOR_GOVERNOR
    /*
     * prevent resource access conflict
     */
    mutex_lock(&ch6_lock);
#endif
		if (tempsen) {
        #ifdef CONFIG_TEMPERATURE_SENSOR_GOVERNOR
            sar_ch6_mode = 1;
        #endif
    	temp_sens_sel(1);
    	set_tempsen(2);
    	printk("enter temperature mode, please get the value from chan6\n");
		}
		else {
        #ifdef CONFIG_TEMPERATURE_SENSOR_GOVERNOR
            sar_ch6_mode = 0;
        #endif
     	temp_sens_sel(0);
   		set_tempsen(0);
    	printk("exit temperature mode\n");
  	}
#ifdef CONFIG_TEMPERATURE_SENSOR_GOVERNOR
    mutex_unlock(&ch6_lock);
#endif
    return count;
}
#else

static int get_celius(void)
{
    int x,y;
    unsigned div=100000;
    /**
     * .x=0.304991,.y=-87.883549
.x=0.304991,.y=-87.578558
.x=0.305556,.y=-87.055556
.x=0.230769,.y=-87.769231
.x=0.230769,.y=-87.538462
.x=0.231092,.y=-86.911765
.x=0.288967,.y=-99.527145
.x=0.288967,.y=-99.238179
.x=0.289982,.y=-98.866432
     *
     */
    ///@todo fix it later
    if(aml_read_reg32(P_SAR_ADC_REG3)&(1<<29))
    {
        x=23077;
        y=-88;

    }else{
        x=28897;
        y=-100;

    }
    return (int)((get_adc_sample(6))*x/div + y);
}
static unsigned get_saradc_ch6(void)
{
    int val=get_adc_sample(6);
    return  (unsigned)(val<0?0:val);
}
static ssize_t temperature_raw_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", get_adc_sample(6));
}
static ssize_t temperature_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", get_cpu_temperature());
}
static ssize_t temperature_mode_show(struct class *cla, struct class_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", aml_read_reg32(P_SAR_ADC_REG3)&(1<<29)?1:0);
}
static ssize_t temperature_mode_store(struct class *cla, struct class_attribute *attr, const char *buf, ssize_t count)
{
    u8 tempsen;
    sscanf(buf, "%x", (int*)&tempsen);
    if (tempsen==0) {
        set_tempsen(0);
    }
    else if(tempsen==1) {
        set_tempsen(2);
    }else{
        printk("only support 1 or 0\n");
    }
    return count;
}
#endif


#ifdef CONFIG_TEMPERATURE_SENSOR_GOVERNOR
/*
 * Meson6 has a temperature sensor in chip, it can be used to measure internal temperature
 * of chip, this value can be feed back to cpu frequent governor to determine max running
 * frequnet of system. This can keep cpu from working long time under high temperature. 
 */
// base tempertatue when doing FT
static int temperature_base   = 318;            // 31.8 c
static int linear_coefficient = 17785;          // 1.7785, 10000 times scale up
static int efuse_base = 0;                      // sar adc offset saved in efuse 

static int __init chip_temp_base(char *str)
{
    int temp_base = 0;
    sscanf(str, "0x%x", &temp_base);
    if (temp_base > 0) {
    	//printk("############ temp_base = %d ################\n",temp_base);
    	efuse_base = temp_base;
    }
    return 0;
}
early_param("temp_base", chip_temp_base);

/*
 * return SAR ADC raw data of temperature sensor
 */
int get_temperature_raw_data(void)
{
    int ret;
    mutex_lock(&ch6_lock);
    if (!sar_ch6_mode) {
    	temp_sens_sel(1);                       // set ch6 to measure temperature temporary
    	set_tempsen(2);
    } 
    ret = get_adc_sample(6);
    if (!sar_ch6_mode) {
    	temp_sens_sel(0);                       // set ch6 to back to its old state 
    	set_tempsen(0);
    }
    mutex_unlock(&ch6_lock);
    return ret;
}
EXPORT_SYMBOL_GPL(get_temperature_raw_data);

/*
 * return chip temperature calculated from sar adc.
 * resolution is 0.1c, example: 441 means 44.1c
 */
int get_chip_temperature(void)
{
    int ret;
    int raw_adc;

    if (efuse_base == 0) {                      // return 0 when not get efuse data
        return 0;    
    }
    
    raw_adc = get_temperature_raw_data();
    ret = (raw_adc - efuse_base) * linear_coefficient + temperature_base * 10000;
    ret = ret / 10000;

    return ret;
}
EXPORT_SYMBOL_GPL(get_chip_temperature);

/*
 * export sysfs to debug
 */
static ssize_t temperature_raw_show(struct class *cla, struct class_attribute *attr, char *buf, ssize_t count)
{
    return sprintf(buf, "%d\n", get_temperature_raw_data());
}

static ssize_t chip_temperature_show(struct class *cla, struct class_attribute *attr, char *buf, ssize_t count)
{
    return sprintf(buf, "%d\n", get_chip_temperature());
}

static ssize_t temperature_base_show(struct class *cla, struct class_attribute *attr, char *buf, ssize_t count)
{
    return sprintf(buf, "%d\n", temperature_base);
}

static ssize_t temperature_base_store(struct class *cla, struct class_attribute *attr, char *buf, ssize_t count)
{
    int base_tmp;
    
    sscanf(buf, "%d", &base_tmp); 
    if (base_tmp > 10240000 || base_tmp < 0) {
        printk("ERROR, wrong base value input:%d\n", base_tmp);    
        return count;
    }
    temperature_base = base_tmp;
    printk("temperature_base is set to %d\n", temperature_base);
    return count;
}

static ssize_t linear_coefficient_show(struct class *cla, struct class_attribute *attr, char *buf, ssize_t count)
{
    return sprintf(buf, "%d\n", linear_coefficient);
}

static ssize_t linear_coefficient_store(struct class *cla, struct class_attribute *attr, char *buf, ssize_t count)
{
    int base_tmp;

    sscanf(buf, "%d", &base_tmp); 
    linear_coefficient = base_tmp;
    printk("linear_coefficient is set to %d\n", linear_coefficient);
    return count;
}

static ssize_t efuse_base_show(struct class *cla, struct class_attribute *attr, char *buf, ssize_t count)
{
    return sprintf(buf, "%d\n", efuse_base);
}

static ssize_t efuse_base_store(struct class *cla, struct class_attribute *attr, char *buf, ssize_t count)
{
    int base_tmp;
    
    sscanf(buf, "%d", &base_tmp); 
    if (base_tmp > 1024 || base_tmp < 0) {
        printk("ERROR, bad value of efuse_base:%d\n", base_tmp);
        return count;
    }

    efuse_base = base_tmp;
    printk("efuse_base is set to %d\n", efuse_base);
    return count;
}
#endif  /* CONFIG_TEMPERATURE_SENSOR_GOVERNOR */

static struct class_attribute saradc_class_attrs[] = {
    __ATTR_RO(saradc_ch0),
    __ATTR_RO(saradc_ch1),
    __ATTR_RO(saradc_ch2),
    __ATTR_RO(saradc_ch3),
    __ATTR_RO(saradc_ch4),
    __ATTR_RO(saradc_ch5),
#ifndef CONFIG_MESON_CPU_TEMP_SENSOR
    __ATTR_RO(saradc_ch6),
    __ATTR(saradc_temperature, S_IRUGO | S_IWUSR, NULL, saradc_temperature_store),
#else
    __ATTR_RO(temperature_raw),
    __ATTR_RO(temperature),
    __ATTR(temperature_mode, S_IRUGO | S_IWUSR, temperature_mode_show, temperature_mode_store),
#endif
#ifdef CONFIG_TEMPERATURE_SENSOR_GOVERNOR
    __ATTR_RO(temperature_raw),
    __ATTR_RO(chip_temperature),
    __ATTR(temperature_base, 0644, temperature_base_show, temperature_base_store),
    __ATTR(linear_coefficient, 0644, linear_coefficient_show, linear_coefficient_store),
    __ATTR(efuse_base, 0644, efuse_base_show, efuse_base_store),
#endif  /* CONFIG_TEMPERATURE_SENSOR_GOVERNOR */
    __ATTR_RO(saradc_ch7),    
    __ATTR(saradc_print_flag, S_IRUGO | S_IWUSR,NULL, saradc_print_flag_store),
    __ATTR_NULL
};
static struct class saradc_class = {
    .name = "saradc",
    .class_attrs = saradc_class_attrs,
};

static int __devinit saradc_probe(struct platform_device *pdev)
{
	int err;
	struct saradc *saradc;
#ifdef ENABLE_CALIBRATION
	struct saradc_platform_data *pdata = pdev->dev.platform_data;
#endif
	saradc = kzalloc(sizeof(struct saradc), GFP_KERNEL);
	if (!saradc) {
		err = -ENOMEM;
		goto err_free_mem;
	}

	saradc_reset();
	gp_saradc = saradc;

	saradc->cal = 0;
	saradc->cal_num = 0;
#ifdef ENABLE_CALIBRATION
	if (pdata && pdata->cal) {
		saradc->cal = pdata->cal;
		saradc->cal_num = pdata->cal_num;
		printk(KERN_INFO "saradc use signed calibration data\n");
	}
	else {
		printk(KERN_INFO "saradc use internal calibration\n");
		saradc->cal = kzalloc(sizeof(struct calibration) * 
				INTERNAL_CAL_NUM, GFP_KERNEL);
		if (saradc->cal) {
			if (saradc_internal_cal(saradc->cal) < 0) {
				kfree(saradc->cal);
				saradc->cal = 0;
				printk(KERN_INFO "saradc calibration fail\n");
			}
			else {
				saradc->cal_num = INTERNAL_CAL_NUM;
				printk(KERN_INFO "saradc calibration ok\n");
			}
		}
	}
#endif
	set_cal_voltage(7);
	spin_lock_init(&saradc->lock);	
#ifdef	CONFIG_MESON_CPU_TEMP_SENSOR
	temp_sens_sel(1);
	get_cpu_temperature_celius=get_celius;
#endif
#ifdef CONFIG_TEMPERATURE_SENSOR_GOVERNOR
    mutex_init(&ch6_lock);
    /*
     * TODO: read efuse_base from efuse
     */
    // efuse_base = get_efuse(addr);
#endif
	return 0;

err_free_mem:
	kfree(saradc);
	printk(KERN_INFO "saradc probe error\n");	
	return err;
}

static int __devexit saradc_remove(struct platform_device *pdev)
{
	struct saradc *saradc = platform_get_drvdata(pdev);
	disable_adc();
	disable_sample_engine();
	gp_saradc = 0;
	kfree(saradc);
	return 0;
}

static struct platform_driver saradc_driver = {
	.probe      = saradc_probe,
	.remove     = saradc_remove,
	.suspend    = NULL,
	.resume     = NULL,
	.driver     = {
		.name   = "saradc",
	},
};

static int __devinit saradc_init(void)
{
	printk(KERN_INFO "SARADC Driver init.\n");
	class_register(&saradc_class);
	return platform_driver_register(&saradc_driver);
}

static void __exit saradc_exit(void)
{
	printk(KERN_INFO "SARADC Driver exit.\n");
	platform_driver_unregister(&saradc_driver);
	class_unregister(&saradc_class);
}

module_init(saradc_init);
module_exit(saradc_exit);

MODULE_AUTHOR("aml");
MODULE_DESCRIPTION("SARADC Driver");
MODULE_LICENSE("GPL");
