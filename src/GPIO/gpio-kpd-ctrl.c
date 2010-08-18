#include <genlib.h>
#include <global.h>
#include "gpio.h"


//#define CONFIG_GPIO_KPD_DEBUG
#ifdef CONFIG_GPIO_KPD_DEBUG
	#define KPD_DBG(fmt, args...) printf("KPD: %s(): " fmt, __FUNCTION__, ## args)
#else
	#define KPD_DBG(fmt, args...)
#endif

#if defined(CONFIG_PC9223)
#define KPD_NUM	6
#else
#define KPD_NUM	4
#endif

static int kpd_matrix[KPD_NUM][KPD_NUM];

#if defined(CONFIG_PC9223)
static int kpd_array[KPD_NUM];
#endif

static int finish;

static int
kpd_value_convert(int val)
{
	int ret;

	KPD_DBG("val = 0x%x\n", val);

	switch (val) {
	case 1:
		ret = 0;
		break;
	case 2:
		ret = 1;
		break;
	case 4:
		ret = 2;
		break;
	case 8:
		ret = 3;
		break;
// Jerry Hsieh add for PDK_PC9223 ; 2010/04/26
	case 16:
		ret = 4;
		break;
	case 32:
		ret = 5;
		break;
	case 64:
		ret = 6;
		break;
	case 128:
		ret = 7;
		break;
	default:
		KPD_DBG("Warning! val = 0x%02x\n", val);
		ret = -1;
	}

	return ret;
}

static void
pdk_check_kpd(void)
{
	int i;
#ifndef CONFIG_PC9223
	int j;
#endif
	int count = 0;

#if defined(CONFIG_PC9223)
	printf("\nkpd_array: (O:press, X:unpress)\n");
	for (i = 0; i < KPD_NUM; i++) {
		printf("col(%d)", i);
		if (kpd_array[i] == 1) {
			count++;
			printf("       O");
		} else
			printf("       X");
		printf("\n");
	}

	if (count == KPD_NUM)
		finish = 1;

#else
	printf("\nkpd_matrix: (O:press, X:unpress)\n       ");
	for (j = 0; j < KPD_NUM; j++) {
		printf("  col(%d)", j);
	}
	printf("\n");

	for (i = 0; i < KPD_NUM; i++) {
		printf("row(%d)", i);
		for (j = 0; j < KPD_NUM; j++) {
			if (kpd_matrix[i][j] == 1) {
				count++;
				printf("       O");
			} else
				printf("       X");
		}
		printf("\n");
	}

	if (count == KPD_NUM * KPD_NUM)
		finish = 1;
#endif
}

static void
pdk_gpio_kpd_isr(void *param)
{
	int tmp1, col, *count = (int *)param;
#ifndef CONFIG_PC9223
	int tmp2, row;
#endif
	KPD_DBG("\n");
//printf("@@--%s--start\n", __func__);
#if defined CONFIG_PC7210	//20080109 leonid+ for PDK-PC7210
	// read col pins
	// PA4, PA5
	socle_gpio_get_value_with_mask(PA, 0x30);
	// PF4, PF5
	socle_gpio_get_value_with_mask(PF, 0x30);

	// read status
	tmp1 = socle_gpio_get_interrupt_status_with_port(PA);
	tmp2 = socle_gpio_get_interrupt_status_with_port(PF);

	// clear interrupt
	socle_gpio_set_interrupt_clear(PA, tmp1);
	socle_gpio_set_interrupt_clear(PF, tmp2);

	KPD_DBG("tmp1 = 0x%x, tmp2 = 0x%x\n", tmp1, tmp2);
#if (KPD_NUM == 4)
	// 4 X 4
	col = kpd_value_convert((tmp1 & 0xf));
	row = kpd_value_convert(((tmp1 & 0x30) >> 4) | (tmp2 & 0x30) >> 2);
#endif
#if (KPD_NUM == 3)
	// 3 X 3
	col = kpd_value_convert(tmp1 & 0x7);
	row = kpd_value_convert(((tmp1 & 0x30) >> 4) | (tmp2 & 0x10) >> 2);
#endif

#elif defined (CONFIG_PC9220)
	// read col pins
	socle_gpio_get_value_with_mask(PJ, 0xf0);

	// read status
	tmp1 = socle_gpio_get_interrupt_status_with_port(PJ);
	tmp2 = socle_gpio_get_value_with_mask(PJ, 0xf0);

	// clear interrupt
	socle_gpio_set_interrupt_clear(PJ, tmp1);

	KPD_DBG("tmp1 = 0x%x \n", tmp1);
	// 4 X 4
	col = kpd_value_convert((tmp1 & 0xf));
	row = kpd_value_convert((tmp2 & 0xf0) >> 4);

#elif defined(CONFIG_MDKFHD)
	// read col pins
	socle_gpio_get_value_with_mask(PB, 0xf0);

	// read status
	tmp1 = socle_gpio_get_interrupt_status_with_port(PB);
	tmp2 = socle_gpio_get_value_with_mask(PB, 0xf0);

	// clear interrupt
	socle_gpio_set_interrupt_clear(PB, tmp1);

	KPD_DBG("tmp1 = 0x%x \n", tmp1);
	// 4 X 4
	col = kpd_value_convert((tmp1 & 0xf));
	row = kpd_value_convert((tmp2 & 0xf0) >> 4);

#elif defined (CONFIG_PC9223)
	//read status
	tmp1 = socle_gpio_get_interrupt_status_with_port(PN);
//printf("@@--%s--tmp1 = 0x%x\n", __func__, tmp1);
	// clear interrupt
	//socle_gpio_set_interrupt_clear(PN, tmp1);
	socle_gpio_set_interrupt_mask_with_mask(PN, 0x0, 0xfc);

	col = kpd_value_convert((tmp1 & 0xfc)) - 2;

	printf("Press - key number = %d   ", col);
	
	while (1) {
		int temp = 0;
		
		temp = socle_gpio_get_value_with_mask(PN, 0xfc);
		temp = (~temp) & 0xfc;
//printf("@@--%s--temp = 0x%x\n", __func__, temp);
		if (temp == 0)
			break;
	}
//printf("@@--%s--col = 0x%x\n", __func__, col);
#else	//PDK-PC700x
	// read col pins
	// PA3, PA4, PA5
	socle_gpio_get_value_with_mask(PA, 0x38);
	// PC1
	socle_gpio_get_value_with_mask(PC, 0x2);

//	MSDELAY(300);

	// read status
	tmp1 = socle_gpio_get_interrupt_status_with_port(PA);
	tmp2 = socle_gpio_get_interrupt_status_with_port(PC);

	// clear interrupt
	socle_gpio_set_interrupt_clear(PA, tmp1);
	socle_gpio_set_interrupt_clear(PC, tmp2);

	KPD_DBG("tmp1 = 0x%x, tmp2 = 0x%x\n", tmp1, tmp2);
#if (KPD_NUM == 4)
	// 4 X 4
	col = kpd_value_convert((tmp1 & 0x7)  | ((tmp2 & 0x1) << 3));
	row = kpd_value_convert(((tmp1 & 0x38) >> 3) | (tmp2 & 0x2) << 2);
#endif
#if (KPD_NUM == 3)
	// 3 X 3
	col = kpd_value_convert(tmp1 & 0x7);
	row = kpd_value_convert((tmp1 & 0x38) >> 3);
#endif
#endif

#if defined (CONFIG_PC9223)
	KPD_DBG("col = %d\n", col);
	if (-1 != col) {
		kpd_array[col] = 1;
		(*count)++;
		printf("Release - count = %d\n", *count);
	}

	if (*count >= KPD_NUM) {
		pdk_check_kpd();
	}

	socle_gpio_set_interrupt_mask_with_mask(PN, 0xfc, 0xfc);

#else
	KPD_DBG("row = %d, col = %d\n", row, col);

	if ((-1 != row) && (-1 != col)) {
		kpd_matrix[row][col] = 1;
		(*count)++;
		printf("count = %d, row = %d, col = %d\n", *count, row, col);
	}

	if (*count >= KPD_NUM * KPD_NUM) {
		pdk_check_kpd();
	}
#endif

	MSDELAY(100);

#if defined CONFIG_PC7210	//20080109 leonid+ for PDK-PC7210
	// write col pins as 0
	// PA4, PA5
	socle_gpio_set_value_with_mask(PA, 0x0, 0x30);
	// PF4, PF5
	socle_gpio_set_value_with_mask(PF, 0x0, 0x30);
#elif defined (CONFIG_PC9220) || defined (CONFIG_PC9223)
	// write col pins as 0
	// PJ4, PJ5, PJ6, PJ7	
	socle_gpio_set_value_with_mask(PJ, 0x0, 0xf0);
#else	//PDK-PC700x
	// write col pins as 0
	// PA3, PA4, PA5
	socle_gpio_set_value_with_mask(PA, 0x0, 0x38);
	// PC1
	socle_gpio_set_value_with_mask(PC, 0x0, 0x2);
#endif
}

static void
gpio_kpd_init(void)
{
	int i, j;
	KPD_DBG("\n");

	for (i = 0; i < KPD_NUM; i++)
		for (j = 0; j < KPD_NUM; j++)
			kpd_matrix[i][j] = 0;

#if defined (CONFIG_PC7210)	//20080109 leonid+ for PDK-PC7210
	// normal mode
	socle_gpio_test_mode_en(PA, 0);
	socle_gpio_test_mode_en(PF, 0);
	// single low edge trigger (row - PA0, PA1, PA2, PA3)
	// PA0, PA1, PA2, PA3
	socle_gpio_set_interrupt_sense_with_mask(PA, 0x0, 0xf);
	socle_gpio_set_interrupt_both_edges_with_mask(PA, 0x0, 0xf);
	socle_gpio_set_interrupt_event_with_mask(PA, 0x0, 0xf);

	// single high edge trigger (col - PA4, PA5, PF4, PF5)
	// PA4, PA5,
	socle_gpio_set_interrupt_sense_with_mask(PA, 0x0, 0x30);
	socle_gpio_set_interrupt_both_edges_with_mask(PA, 0x0, 0x30);
	socle_gpio_set_interrupt_event_with_mask(PA, 0x30, 0x30);
	// PF4, PF5
	socle_gpio_set_interrupt_sense_with_mask(PF, 0x0, 0x30);
	socle_gpio_set_interrupt_both_edges_with_mask(PF, 0x0, 0x30);
	socle_gpio_set_interrupt_event_with_mask(PF, 0x30, 0x30);

	// set row as input
	// PA0, PA1, PA2, PA3
	socle_gpio_get_value_with_mask(PA, 0xf);

	// write col pins as 0
	// PA4, PA5,
	socle_gpio_set_value_with_mask(PA, 0x0, 0x30);
	// PF4, PF5
	socle_gpio_set_value_with_mask(PF, 0x0, 0x30);

	// enable all interrupt
	socle_gpio_set_interrupt_mask_with_mask(PA, 0x3F, 0x3F);
	socle_gpio_set_interrupt_mask_with_mask(PF, 0x30, 0x30);

#elif defined (CONFIG_PC9220) 	//20080109 leonid+ for PDK-PC7210
	// normal mode
	socle_gpio_test_mode_en(PJ, 0);

	// single low edge trigger (row - PJ0, PJ1, PJ2, PJ3)
	// PJ0, PJ1, PJ2, PJ3
	socle_gpio_set_interrupt_sense_with_mask(PJ, 0x0, 0xf);
	socle_gpio_set_interrupt_both_edges_with_mask(PJ, 0x0, 0xf);
	socle_gpio_set_interrupt_event_with_mask(PJ, 0x0, 0xf);

	// single high edge trigger (col - PJ4, PJ5, PJ6, PJ7)
	// PJ4, PJ5, PJ6, PJ7
	socle_gpio_set_interrupt_sense_with_mask(PJ, 0x0, 0xf0);
	socle_gpio_set_interrupt_both_edges_with_mask(PJ, 0x0, 0xf0);
	socle_gpio_set_interrupt_event_with_mask(PJ, 0xf0, 0xf0);

	// set row as input
	// PJ0, PJ1, PJ2, PJ3
	socle_gpio_get_value_with_mask(PJ, 0xf);

	// write col pins as 0
	// PJ4, PJ5,
	socle_gpio_set_value_with_mask(PJ, 0x0, 0xf0);

	// enable all interrupt
	socle_gpio_set_interrupt_mask_with_mask(PJ, 0xff, 0xff);

#elif defined(CONFIG_MDKFHD)
	// normal mode
	socle_gpio_test_mode_en(PB, 0);

	// single low edge trigger (row - PB0, PB1, PB2, PB3)
	// PB0, PB1, PB2, PB3
	socle_gpio_set_interrupt_sense_with_mask(PB, 0x0, 0xf);
	socle_gpio_set_interrupt_both_edges_with_mask(PB, 0x0, 0xf);
	socle_gpio_set_interrupt_event_with_mask(PB, 0x0, 0xf);

	// single high edge trigger (col - PB4, PB5, PB6, PB7)
	// PB4, PB5, PB6, PB7
	socle_gpio_set_interrupt_sense_with_mask(PB, 0x0, 0xf0);
	socle_gpio_set_interrupt_both_edges_with_mask(PB, 0x0, 0xf0);
	socle_gpio_set_interrupt_event_with_mask(PB, 0xf0, 0xf0);

	// set row as input
	// PB0, PB1, PB2, PB3
	socle_gpio_get_value_with_mask(PB, 0xf);

	// write col pins as 0
	// PB4, PB5,
	socle_gpio_set_value_with_mask(PB, 0x0, 0xf0);

	// enable all interrupt
	socle_gpio_set_interrupt_mask_with_mask(PB, 0xff, 0xff);

#elif defined (CONFIG_PC9223)	//Jerry Hsieh add for PDK-PC9223 ; 2010/04/26
	//normal mode
	socle_gpio_test_mode_en(PN, 0);

	//single low edge trigger
	// PI2, PI3, PI4, PI5, PI6, PI7
	socle_gpio_set_interrupt_sense_with_mask(PN, 0x0, 0xfc);
	socle_gpio_set_interrupt_both_edges_with_mask(PN, 0x0, 0xfc);
	socle_gpio_set_interrupt_event_with_mask(PN, 0x0, 0xfc);

	//set gpio as input
	socle_gpio_get_value_with_mask(PN, 0xfc);

	// enable all interrupt
	socle_gpio_set_interrupt_mask_with_mask(PN, 0xff, 0xff);

#else	//PDK-PC700x
	// normal mode
	socle_gpio_test_mode_en(PA, 0);
	// single low edge trigger (row - PA0, PA1, PA2, PC0)
	// PA0, PA1, PA2
	socle_gpio_set_interrupt_sense_with_mask(PA, 0x0, 0x7);
	socle_gpio_set_interrupt_both_edges_with_mask(PA, 0x0, 0x7);
	socle_gpio_set_interrupt_event_with_mask(PA, 0x0, 0x7);
	// PC0
	socle_gpio_set_interrupt_sense_with_mask(PC, 0x0, 0x1);
	socle_gpio_set_interrupt_both_edges_with_mask(PC, 0x0, 0x1);
	socle_gpio_set_interrupt_event_with_mask(PC, 0x0, 0x1);

	// single high edge trigger (col - PA3, PA4, PA5, PC1)
	// PA3, PA4, PA5,
	socle_gpio_set_interrupt_sense_with_mask(PA, 0x0, 0x38);
	socle_gpio_set_interrupt_both_edges_with_mask(PA, 0x0, 0x38);
	socle_gpio_set_interrupt_event_with_mask(PA, 0x38, 0x38);
	// PC1
	socle_gpio_set_interrupt_sense_with_mask(PC, 0x0, 0x2);
	socle_gpio_set_interrupt_both_edges_with_mask(PC, 0x0, 0x2);
	socle_gpio_set_interrupt_event_with_mask(PC, 0x2, 0x2);

	// set row as input
	// PA0, PA1, PA2
	socle_gpio_get_value_with_mask(PA, 0x7);
	// PC0
	socle_gpio_get_value_with_mask(PC, 0x1);

	// write col pins as 0
	// PA3, PA4, PA5
	socle_gpio_set_value_with_mask(PA, 0x0, 0x38);
	// PC1
	socle_gpio_set_value_with_mask(PC, 0x0, 0x2);

	// enable all interrupt
	socle_gpio_set_interrupt_mask_with_mask(PA, 0x3F, 0x3F);
	socle_gpio_set_interrupt_mask_with_mask(PC, 0x3, 0x3);
#endif
}


extern int
socle_gpio_kpd_test(int autotest)
{
	int ret = 0, count = 0, irq;

#if defined (CONFIG_PC9220)
	irq = socle_gpio_get_irq(PJ);
#elif defined(CONFIG_PC9223)
	irq = socle_gpio_get_irq(PN);
#else
	irq = socle_gpio_get_irq(PA);
#endif
	request_irq(irq, pdk_gpio_kpd_isr, (void*)&count);

	gpio_kpd_init();

	finish = 0;
	printf("Please press all keys and then exit.\n");
	while (1) {
		if (finish)
			break;
	};
	finish = 0;

	free_irq(irq);
	socle_init_gpio_irq();

	return ret;
}

