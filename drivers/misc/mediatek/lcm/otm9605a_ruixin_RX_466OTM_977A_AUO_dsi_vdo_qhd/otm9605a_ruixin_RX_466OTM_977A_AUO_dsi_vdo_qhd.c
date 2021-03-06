#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#include <platform/mt_pmic.h>

#define Lcd_Log printf
#else
#include <linux/string.h>
#include <linux/kernel.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>

#define Lcd_Log printk
#endif

#include "lcm_drv.h"

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(540)
#define FRAME_HEIGHT 										(960)

#define REGFLAG_DELAY             							0XFFE
#define REGFLAG_END_OF_TABLE      							0xFFF   // END OF REGISTERS MARKER

#define LCM_ID	0x9605

#define LCM_DSI_CMD_MODE									0
#define GPIO_LCD_ID_PIN GPIO112

#ifndef TRUE
#define   TRUE     1
#endif

#ifndef FALSE
#define   FALSE    0
#endif

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


static struct LCM_setting_table lcm_initialization_setting[] = {

            /*
            Note :
            Data ID will depends on the following rule.
            	count of parameters > 1	=> Data ID = 0x39
            	count of parameters = 1	=> Data ID = 0x15
            	count of parameters = 0	=> Data ID = 0x05
            Structure Format :
            {DCS command, count of parameters, {parameter list}}
            {REGFLAG_DELAY, milliseconds of time, {}},
            ...
            Setting ending by predefined flag
            {REGFLAG_END_OF_TABLE, 0x00, {}}
            */

            {0x00,1,{0x00}},
            {0xFF,3,{0x96,0x05,0x01}},

            {0x00,1,{0x80}},
            {0xFF,2,{0x96,0x05}},

            {0x00,1,{0x92}}, // mipi 2 lane
            {0xFF,2,{0x10,0x02}},

            {0x00,1,{0x80}},
            {0xC1,2,{0x36,0x66}}, //65Hz

            {0x00,1,{0xB1}},
            {0xC5,1,{0x28}},//VDD18=1.9V

            {0x00,1,{0x89}},
            {0xC0,1,{0x01}},// TCON OSC turbo mode
            ////auo request
            {0x00,1,{0xB4}},
            {0xC0,1,{0x10}},// 1+2 dot  10


            {0x00,1,{0x80}},
            {0xC4,1,{0x9c}},

            //Address Shift//Power Control Setting 2 for Normal Mode
            {0x00,1,{0x90}},
           {0xC5,7,{0x96,0x79,0x01,0x79,0x33,0x33,0x34}},//VGH VHL

            {0x00,1,{0x00}},
            //{0xD8,2,{0x65,0x65}},//gvdd=4.4V
            {0xD8,2,{0x70,0x70}},//gvdd=4.4V

            //Address Shift//VCOM setting
            {0x00,1,{0x00}},
            //{0xD9,1,{0x41}},//68
            {0xD9,1,{0x48}},//68

            {0x00,1,{0xA6}},//zigzag off
            {0xB3,1,{0x27}},

            {0x00,1,{0xA0}},//panel mode
            {0xB3,1,{0x10}},
            //GOA
            {0x00,1,{0x80}},
            {0xCB,10,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
            {0x00,1,{0x90}},
            {0xCB,15,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
            {0x00,1,{0xA0}},
            {0xCB,15,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
            {0x00,1,{0xB0}},
            {0xCB,10,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
            {0x00,1,{0xC0}},
            {0xCB,15,{0x00,0x00,0x04,0x04,0x04,0x04,0x04,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
            {0x00,1,{0xD0}},
            {0xCB,15,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x04,0x04,0x04,0x04,0x04,0x00,0x00}},
            {0x00,1,{0xE0}},
            {0xCB,10,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
            {0x00,1,{0xF0}},
            {0xCB,10,{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}},

            {0x00,1,{0x80}},
            {0xCC,10,{0x00,0x00,0x01,0x0F,0x0D,0x0B,0x09,0x05,0x00,0x00}},
            {0x00,1,{0x90}},
            {0xCC,15,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x10,0x0E}},
            {0x00,1,{0xA0}},
            {0xCC,15,{0x0C,0x0A,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},

            {0x00,1,{0x80}},
            {0xCE,12,{0x81,0x01,0x0E,0x82,0x01,0x0E,0x00,0x0F,0x00,0x00,0x0F,0x00}},
            {0x00,1,{0x90}},
            {0xCE,14,{0x13,0xBE,0x0E,0x13,0xBF,0x0E,0xF0,0x00,0x00,0xF0,0x00,0x00,0x00,0x00}},
            {0x00,1,{0xA0}},
            {0xCE,14,{0x18,0x01,0x03,0xBC,0x00,0x0E,0x00,0x18,0x00,0x03,0xBD,0x00,0x0E,0x00}},
            {0x00,1,{0xB0}},
            {0xCE,14,{0x10,0x00,0x03,0xBE,0x00,0x0E,0x00,0x10,0x01,0x03,0xBF,0x00,0x0E,0x00}},
            {0x00,1,{0xC0}},
            {0xCE,14,{0x38,0x03,0x03,0xC0,0x00,0x09,0x05,0x38,0x02,0x03,0xC1,0x00,0x09,0x05}},
            {0x00,1,{0xD0}},
            {0xCE,14,{0x30,0x00,0x03,0xBC,0x00,0x09,0x05,0x30,0x01,0x03,0xBD,0x00,0x09,0x05}},
            {0x00,1,{0xC0}},
            {0xCF,10,{0x01,0x01,0x00,0x00,0x00,0x00,0x01,0x02,0x00,0x00}},
            ////auo request
            //Address Shift//Gamma +
            {0x00,1,{0x00}},
            //{0xE1,16,{0x05,0x0B,0x10,0x0C,0x05,0x0D,0x0B,0x0A,0x04,0x07,0x0D,0x07,0x0E,0x13,0x0E,0x00}},
            {0xE1,16,{0x02,0x10,0x16,0x0F,0x07,0x12,0x0A,0x09,0x04,0x08,0x0A,0x08,0x0F,0x12,0x0E,0x08}},

            //Address Shift//Gamma -
            {0x00,1,{0x00}},
            //{0xE2,16,{0x05,0x0B,0x10,0x0C,0x05,0x0D,0x0B,0x0A,0x04,0x07,0x0D,0x07,0x0E,0x13,0x0E,0x00}},
            {0xE2,16,{0x02,0x10,0x16,0x0F,0x07,0x12,0x0A,0x09,0x04,0x08,0x0A,0x08,0x0F,0x12,0x0E,0x08}},

            {0x11,1,{0x00}},//SLEEP OUT
            {REGFLAG_DELAY,120,{}},

            {0x29,1,{0x00}},//Display ON
            {REGFLAG_DELAY,20,{}},
            // Note
            // Strongly recommend not to set Sleep out / Display On here. That will cause messed frame to be shown as later the backlight is on.


            // Setting ending by predefined flag
            {REGFLAG_END_OF_TABLE, 0x00, {}}
        };

static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
	{0x11, 0, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    // Display ON
	{0x29, 0, {0x00}},
    {REGFLAG_DELAY, 100, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	// Display off sequence
	{0x28, 0, {0x00}},
        {REGFLAG_DELAY, 120, {}},
    // Sleep Mode On
	{0x10, 0, {0x00}},
        {REGFLAG_DELAY, 120, {}},//20
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;

    for (i = 0; i < count; i++) {

        unsigned cmd;
        cmd = table[i].cmd;

        switch (cmd) {

        case REGFLAG_DELAY :
            MDELAY(table[i].count);
            break;

        case REGFLAG_END_OF_TABLE :
            break;

        default:
            dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
        }
    }

}


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
    memset(params, 0, sizeof(LCM_PARAMS));

    params->type   = LCM_TYPE_DSI;

    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;

    // enable tearing-free
    params->dbi.te_mode 				= LCM_DBI_TE_MODE_DISABLED;  //LCM_DBI_TE_MODE_VSYNC_ONLY;
    params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;

#if (LCM_DSI_CMD_MODE)
    params->dsi.mode   = CMD_MODE;
#else
    params->dsi.mode   = SYNC_PULSE_VDO_MODE;
#endif

    // DSI
    /* Command mode setting */
    params->dsi.LANE_NUM				= LCM_TWO_LANE;
    //The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
    params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
    params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    // Highly depends on LCD driver capability.
    // Not support in MT6573
    params->dsi.packet_size=256;

    // Video mode setting
    params->dsi.intermediat_buffer_num = 2;

    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

    params->dsi.vertical_sync_active				= 4;
    params->dsi.vertical_backporch					= 16;
    params->dsi.vertical_frontporch					= 15;
    params->dsi.vertical_active_line				= FRAME_HEIGHT;

    params->dsi.horizontal_sync_active				= 10;
    params->dsi.horizontal_backporch				= 64;
    params->dsi.horizontal_frontporch				= 64;
    params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

    // Bit rate calculation
    params->dsi.pll_div1=1;		// div1=0,1,2,3;div1_real=1,2,4,4
    params->dsi.pll_div2=0;		// div2=0,1,2,3;div2_real=1,2,4,4
    params->dsi.fbk_div =15;		// fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)

    /* ESD or noise interference recovery For video mode LCM only. */ // Send TE packet to LCM in a period of n frames and check the response.
    params->dsi.lcm_int_te_monitor = FALSE;
    params->dsi.lcm_int_te_period = 1; // Unit : frames

    // Need longer FP for more opportunity to do int. TE monitor applicably.
    if (params->dsi.lcm_int_te_monitor)
        params->dsi.vertical_frontporch *= 2;

    // Monitor external TE (or named VSYNC) from LCM once per 2 sec. (LCM VSYNC must be wired to baseband TE pin.)
    params->dsi.lcm_ext_te_monitor = FALSE;
    // Non-continuous clock
    params->dsi.noncont_clock = TRUE;
    params->dsi.noncont_clock_period = 2; // Unit : frames
}


static void lcm_init(void)
{
#ifdef BUILD_LK
    pmic_config_interface(0x0532,5,0x7,5);//add by libo for VGP2 POWER ON
    pmic_config_interface(0x050C,1,0x1,15);
#else
    hwPowerOn(MT6323_POWER_LDO_VGP2,VOL_2800,"LCM");
#endif
    MDELAY(100);


    SET_RESET_PIN(1);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(50);

    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void)
{
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);

#ifdef BUILD_LK
    pmic_config_interface(0x0532,0,0x7,5);//add by libo for VGP2 POWER OFF
    pmic_config_interface(0x050C,0,0x1,15);
#else
    hwPowerDown(MT6323_POWER_LDO_VGP2,"LCM");
#endif



}


static void lcm_resume(void)
{
    Lcd_Log("\n %s \n",__func__);

    lcm_init();

//	push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
}

static int get_lcd_id(void)
{
    mt_set_gpio_mode(GPIO_LCD_ID_PIN,0);
    mt_set_gpio_dir(GPIO_LCD_ID_PIN,0);
    mt_set_gpio_pull_enable(GPIO_LCD_ID_PIN,1);
    mt_set_gpio_pull_select(GPIO_LCD_ID_PIN,1);
    MDELAY(1);

    return mt_get_gpio_in(GPIO_LCD_ID_PIN);
}


static unsigned int lcm_compare_id(void)
{
	int   array[4];
		char  buffer[5];
		unsigned int id=0;
	
		SET_RESET_PIN(0);
		MDELAY(200);
		SET_RESET_PIN(1);
		MDELAY(200);
		
	array[0] = 0x00083700;// read id return two byte,version and id
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0xA1,buffer,4);
	
	id=(buffer[2]<<8)+buffer[3];

#if defined(BUILD_LK)
	printf("wqcat %s, id2 = 0x%08x, id3 = 0x%08x id = 0x%08x\n", __func__, buffer[2], buffer[3],id);
#endif

#ifndef BUILD_LK
	printk("wqcat %s, id2 = 0x%08x, id3 = 0x%08x id = 0x%08x\n", __func__, buffer[2], buffer[3],id);
#endif

   return (LCM_ID == id&&get_lcd_id()==0)?1:0;
//	 return (LCM_ID == id)?1:0;
}


LCM_DRIVER otm9605a_ruixin_RX_466OTM_977A_AUO_dsi_vdo_qhd_lcm_drv =
    {
        .name			= "otm9605a_ruixin_RX_466OTM_977A_AUO_dsi_vdo_qhd",
        .set_util_funcs = lcm_set_util_funcs,
        .get_params     = lcm_get_params,
        .init           = lcm_init,
        .suspend        = lcm_suspend,
        .resume         = lcm_resume,
        .compare_id    = lcm_compare_id,
#if (LCM_DSI_CMD_MODE)
        .set_backlight	= lcm_setbacklight,
        .update         = lcm_update,
#endif
    };
