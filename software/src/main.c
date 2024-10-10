#include "base.h"
#include "periph.h"
#include "sys.h"
#include "GPIO.h"
#include "USART.h"
#include "RTC.h"
#include "I2C.h"
#include "ADC.h"
#include "tim.h"

#include "AS5600/AS5600.h"


/*!<
 * pinout
 * */
#define AS5600_OUT_PORT			GPIOA			/* AS5600 out	*/
#define AS5600_OUT_PIN			0				/* ADC		I	*/
#define TMC_MS1_PORT			GPIOA			/* TMC MS1		*/
#define TMC_MS1_PIN				1				/* GPIO		O	*/
#define TMC_MS2_PORT			GPIOA			/* TMC MS2		*/
#define TMC_MS2_PIN				2				/* GPIO		O	*/
#define TMC_NEN_PORT			GPIOA			/* TMC NEN		*/
#define TMC_NEN_PIN				3				/* GPIO		O	*/
#define CTRL_SPI_NCS_DEV_PIN	0				/* SPI		I		TODO */
#define CTRL_SPI_CLK_DEV_PIN	0				/* SPI		I		TODO */
#define CTRL_SPI_MISO_DEV_PIN	0				/* SPI		O		TODO */
#define CTRL_SPI_MOSI_DEV_PIN	0				/* SPI		I		TODO */
#define STATUS_LED_PORT			GPIOA			/* status LED	*/
#define STATUS_LED_PIN			8				/* GPIO		O	*/
#define TMC_UART_TX_DEV_PIN		USART1_TX_A9	/* USART	O	*/
#define TMC_UART_RX_DEV_PIN		USART1_RX_A10	/* USART	I	*/
#define USB_DP_DEV_PIN			0				/* USB		IO		TODO */
#define USB_DN_DEV_PIN			0				/* USB		IO		TODO */
#define FLASH_SPI_NCS_PORT		GPIOA			/* flash SPI CS */
#define FLASH_SPI_NCS_PIN		15				/* GPIO		O	*/
#define TMC_DIAG_PORT			GPIOB			/* TMC DIAG		*/
#define TMC_DIAG_PIN			0				/* EXTI		I	*/
#define TMC_INDEX_PORT			GPIOB			/* TMC index	*/
#define TMC_INDEX_PIN			1				/* EXTI		I	*/
#define FLASH_SPI_CLK_DEV_PIN	0				/* SPI		O		TODO */
#define FLASH_SPI_MISO_DEV_PIN	0				/* SPI		I		TODO */
#define FLASH_SPI_MOSI_DEV_PIN	0				/* SPI		O		TODO */
#define FLASH_NWP_PORT			GPIOB			/* flash NWP	*/
#define FLASH_NWP_PIN			6				/* GPIO		O	*/
#define FLASH_NRST_PORT			GPIOB			/* flash NWP	*/
#define FLASH_NRST_PIN			7				/* GPIO		O	*/
#define AS5600_I2C_SDA_DEV_PIN	I2C2_B9_SDA		/* I2C		IO	*/
#define AS5600_I2C_SCL_DEV_PIN	I2C2_B10_SCL	/* I2C		O	*/
#define TMC_STEP_PORT			GPIOB			/* TMC step		*/
#define TMC_STEP_PIN			12				/* GPIO		O	*/
#define TMC_DIR_PORT			GPIOB			/* TMC dir		*/
#define TMC_DIR_PIN				13				/* GPIO		O	*/
#define TMC_SPREAD_PORT			GPIOB			/* TMC spread	*/
#define TMC_SPREAD_PIN			14				/* GPIO		O	*/
#define TMC_STDBY_PORT			GPIOB			/* TMC standby	*/
#define TMC_STDBY_PIN			15				/* GPIO		O	*/

/*!< IRQ
 * \line	0: TMC_DIAG
 * \line	1: ADC_JEOC
 * \line	2: TMC_INDEX
 * */


volatile uint16_t angle = 0;


void EXTI0_handler(void) {
	EXTI->PR = 0x00000001UL;
	return;
}

void EXTI1_handler(void) {
	EXTI->PR = 0x00000002UL;
	return;
}

void ADC_handler(void) {
	uint8_t status = ADC1->SR;
	ADC1->SR = ~status;
	if (status & 0b000001) {
		return;  // watchdog
	}
	if (status & 0b000100) {
		uint32_t a = ADC1->JDR1;
		angle = a + (a / 4);
		return;
	}
}

void TIM1_update_handler(void) {
	TIM1->SR &= ~0x00000001UL;
	GPIO_toggle(STATUS_LED_PORT, STATUS_LED_PIN);
	return;
}


// application
void main(void) {
	set_SYS_hardware_config(PWR_LVL_NOM, 25000000);												// 3v3, HSE@25MHz
	set_SYS_oscilator_config(0, 1, 0, 0, 1, 0, 0, 0, 0);										// enable HSE, CSS and the backup-domain
	set_SYS_PLL_config(PLL_CLK_SRC_HSE, 12U, 96U, PLL_P_DIV_4, 0, 0);							// enable PLL @ 25MHz / 12 * 96 / 2 -> 100MHz
	set_SYS_clock_config(SYS_CLK_SRC_PLL_P, AHB_CLK_NO_DIV, APBx_CLK_DIV2, APBx_CLK_NO_DIV);	// configure sys / peripheral clocks
	set_SYS_tick_config(1);																		// enable SYS_tick with interrupt
	sys_init();																					// write settings

	// GPIO
	config_GPIO(TMC_MS1_PORT, TMC_MS1_PIN, GPIO_output | GPIO_no_pull | GPIO_push_pull);		// TMC_MS1
	config_GPIO(TMC_MS2_PORT, TMC_MS2_PIN, GPIO_output | GPIO_no_pull | GPIO_push_pull);		// TMC_MS2
	config_GPIO(TMC_NEN_PORT, TMC_NEN_PIN, GPIO_output | GPIO_no_pull | GPIO_push_pull);		// TMC_NEN
	config_GPIO(STATUS_LED_PORT, STATUS_LED_PIN, GPIO_output | GPIO_no_pull | GPIO_push_pull);	// status LED
	config_GPIO(FLASH_NWP_PORT, FLASH_NWP_PIN, GPIO_output | GPIO_no_pull | GPIO_push_pull);	// FLASH_NWP
	config_GPIO(FLASH_NRST_PORT, FLASH_NRST_PIN, GPIO_output | GPIO_no_pull | GPIO_push_pull);	// FLASH_NRST
	config_GPIO(TMC_STEP_PORT, TMC_STEP_PIN, GPIO_output | GPIO_no_pull | GPIO_push_pull);		// TMC_STEP
	config_GPIO(TMC_DIR_PORT, TMC_DIR_PIN, GPIO_output | GPIO_no_pull | GPIO_push_pull);		// TMC_DIR
	config_GPIO(TMC_SPREAD_PORT, TMC_SPREAD_PIN, GPIO_output | GPIO_no_pull | GPIO_push_pull);	// TMC_SPREAD
	config_GPIO(TMC_STDBY_PORT, TMC_STDBY_PIN, GPIO_output | GPIO_no_pull | GPIO_push_pull);	// TMC_STDBY
	GPIO_write(TMC_NEN_PORT, TMC_NEN_PIN, 1);			// disable TMC chip
	GPIO_write(TMC_MS1_PORT, TMC_MS1_PIN, 0);			// set MS1 setting for 1/8
	GPIO_write(TMC_MS2_PORT, TMC_MS2_PIN, 0);			// set MS2 setting for 1/8
	GPIO_write(STATUS_LED_PORT, STATUS_LED_PIN, 1);		// status LED off

	// EXTI
	config_EXTI_GPIO(TMC_DIAG_PORT, TMC_DIAG_PIN, 1, 0);										// TMC_DIAG
	NVIC_set_IRQ_priority(EXTI0_IRQn, 0);														// TMC_DIAG-IRQ
	config_EXTI_GPIO(TMC_INDEX_PORT, TMC_INDEX_PIN, 1, 0);										// TMC_INDEX
	NVIC_set_IRQ_priority(EXTI1_IRQn, 2);														// TMC_INDEX-IRQ
	start_EXTI(TMC_DIAG_PIN); start_EXTI(TMC_INDEX_PIN);										// start TMC interrupts

	// ADC
	config_ADC(ADC_CLK_DIV2 | ADC_INJ_DISC | ADC_RES_12B | ADC_EOC_SINGLE | ADC_INJ_TRIG_TIM1_TRGO | ADC_INJ_TRIG_MODE_RISING);
	config_ADC_watchdog(AS5600_OUT_PIN, ADC_WDG_TYPE_INJECTED, 200, 3900);
	config_ADC_IRQ(1, ADC_IRQ_JEOC | ADC_IRQ_WDG);
	config_ADC_GPIO_inj_channel(AS5600_OUT_PORT, AS5600_OUT_PIN, ADC_SAMPLE_28_CYCLES, 409, 0);	// AS5600 out

	// TIM
	config_TIM_master(TIM1, 10000, 100, TIM_TRGO_UPDATE);										// 100 Hz
	start_TIM_update_irq(TIM1);
	// TODO: is it possible to combine PWM and UPDATE triggers to make sure move and read are not done psudo-simultanuisly

	// USART
	config_UART(TMC_UART_TX_DEV_PIN, TMC_UART_RX_DEV_PIN, 115200);								// TMC_UART

	// I2C
	config_I2C(AS5600_I2C_SCL_DEV_PIN, AS5600_I2C_SDA_DEV_PIN, 0x00);							// AS5600_I2C

	// TODO: SPI (flash)
	// TODO: SPI (slave)
	// TODO: USB

	/*!< test apps */
	while (config_AS5600(
		I2C2, AS5600_POW_NOM | AS5600_HYST_2LSB | AS5600_MODE_REDUCED_ANALOG |
		AS5600_SFILTER_2 | AS5600_FFILTER_10LSB | AS5600_WDG_ON, 10
	)) { delay_ms(10); }
	//volatile uint8_t stat = AS5600_get_status(I2C2, 10);

	start_ADC(0, 1);									// start ADC injected channels
	start_TIM(TIM1);									// start ADC polling timer
	GPIO_write(TMC_NEN_PORT, TMC_NEN_PIN, 0);			// enable TMC chip

	while (1) {
		GPIO_toggle(TMC_STEP_PORT, TMC_STEP_PIN);
		delay_ms(1);
	}
}
