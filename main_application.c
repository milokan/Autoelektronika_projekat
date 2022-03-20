/* Standard includes. */
#include <stdio.h> 
#include <conio.h>
#include <string.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include "extint.h"

/* Hardware simulator utility functions */
#include "HW_access.h"

/* SERIAL SIMULATOR CHANNEL TO USE */
#define COM_CH_0 (0)
#define COM_CH_1 (1)

	/* TASK PRIORITIES */
#define	TASK_SERIAL_SEND_PRI ( tskIDLE_PRIORITY + 2 )
#define TASK_SERIAL_REC_PRI	 ( tskIDLE_PRIORITY + 3 )
#define	SERVICE_TASK_PRI     ( tskIDLE_PRIORITY + 1 )

/* TASKS: FORWARD DECLARATIONS */
static void led_bar_tsk(void* pvParameters);
static void mjerenje_brzine(void* pvParameters);
static void ukupni_predjeni_put(void* pvParameters);
static void SerialSend_Task0(void* pvParameters);
static void SerialReceive_Task0(void* pvParameters);
static void SerialSend_Task1(void* pvParameters);
static void SerialReceive_Task1(void* pvParameters);
void main_demo(void);

/* TRASNMISSION DATA - CONSTANT IN THIS APPLICATION */
static uint16_t inkrementi;
static uint16_t obrtaji = 0;
static uint16_t obim_tocka;
static uint16_t ukupni_put = 0;
static uint16_t brzina = 0;
static uint8_t d2;

/* RECEPTION DATA BUFFER */
#define R_BUF_SIZE (32)
static char r_buffer0[R_BUF_SIZE];
static char r_buffer1[R_BUF_SIZE];
static uint8_t volatile r_point0;
static uint8_t volatile r_point1;
static char obim[R_BUF_SIZE];

/* 7-SEG NUMBER DATABASE - ALL HEX DIGITS */
static const uint8_t hexnum[] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F };

/* GLOBAL OS-HANDLES */
static SemaphoreHandle_t RXC_BinarySemaphore0;
static SemaphoreHandle_t RXC_BinarySemaphore1;
static QueueHandle_t inkrementi_q;

/* RXC - RECEPTION COMPLETE - INTERRUPT HANDLER */
static uint32_t prvProcessRXCInterrupt(void)
{
	BaseType_t xHigherPTW = pdFALSE;

	if (get_RXC_status(0) != 0)
	{
		xSemaphoreGiveFromISR(RXC_BinarySemaphore0, &xHigherPTW);
	}

	if (get_RXC_status(1) != 0)
	{
		xSemaphoreGiveFromISR(RXC_BinarySemaphore1, &xHigherPTW);
	}

	portYIELD_FROM_ISR((uint32_t)xHigherPTW);
}

static void ukupni_predjeni_put(void* pvParameters)
{
	uint16_t rec_buf;
	uint8_t d;
	uint16_t ink_tren = 0;

	while (1)
	{
		vTaskDelay(pdMS_TO_TICKS(1000));
		xQueueReceive(inkrementi_q, &rec_buf, portMAX_DELAY);
		
		ink_tren = ink_tren + rec_buf;
		printf("Trenutni inkrementi: %d\n", ink_tren);

		get_LED_BAR(2, &d);

		if (d == (uint8_t)1) //ukljucenjem, a zatim iskljucenjem 1. diode od dole u 3. stupcu
		{                    //resetuje se put na 0, a zatim ponovo izracunava
			obrtaji = (uint16_t)0;

			if (ink_tren >= (uint16_t)36000)
			{
				obrtaji = obrtaji + ink_tren / (uint16_t)36000;
				ink_tren = ink_tren % (uint16_t)36000;
			}
		}
		else
		{
			if (ink_tren >= (uint16_t)36000)
			{
				obrtaji = obrtaji + ink_tren / (uint16_t)36000;
				ink_tren = ink_tren % (uint16_t)36000;
			}
		}

		ukupni_put = obim_tocka * obrtaji;
		printf("Predjeni put je: %dcm\n", ukupni_put);
	}
}

static void mjerenje_brzine(void* pvParameters)
{
	uint16_t rec_buf;
	float ink_cm = 0.000;
	uint8_t ispis_brzine[9];
	static uint8_t cnt = 0;

	while (1)
	{
		vTaskDelay(pdMS_TO_TICKS(1000));
		xQueueReceive(inkrementi_q, &rec_buf, portMAX_DELAY);

		ink_cm = (float)obim_tocka / 36000;
		//printf("Jedan ink je: %.3fcm\n", ink_cm);

		brzina = (ink_cm * rec_buf) / 0.2;
		printf("Brzina je: %dcm/s\n", brzina);

		uint16_t jedinica = brzina % (uint16_t)10;
		uint16_t desetica = (brzina / (uint16_t)10) % (uint16_t)10;
		uint16_t stotina = (brzina / (uint16_t)100) % (uint16_t)10;
		uint16_t hiljada = (brzina / (uint16_t)1000) % (uint16_t)10;

		ispis_brzine[0] = (uint16_t)hiljada + (uint16_t)'0';
		ispis_brzine[1] = (uint16_t)stotina + (uint16_t)'0';
		ispis_brzine[2] = (uint16_t)desetica + (uint16_t)'0';
		ispis_brzine[3] = (uint16_t)jedinica + (uint16_t)'0';
		ispis_brzine[4] = (uint16_t)'c';
		ispis_brzine[5] = (uint16_t)'m';
		ispis_brzine[6] = (uint16_t)'/';
		ispis_brzine[7] = (uint16_t)'s';
		ispis_brzine[8] = (uint16_t)'\n';

		get_LED_BAR(2, &d2);

		if (d2 == (uint8_t)2)
		{
			cnt++;

			if (brzina != (uint8_t)0)
			{

				if (cnt == (uint8_t)1)
				{
					send_serial_character(COM_CH_1, ispis_brzine[0]);
				}
				else if (cnt == (uint8_t)2)
				{
					send_serial_character(COM_CH_1, ispis_brzine[1]);
				}
				else if (cnt == (uint8_t)3)
				{
					send_serial_character(COM_CH_1, ispis_brzine[2]);
				}
				else if (cnt == (uint8_t)4)
				{
					send_serial_character(COM_CH_1, ispis_brzine[3]);
				}
				else if (cnt == (uint8_t)5)
				{
					send_serial_character(COM_CH_1, ispis_brzine[4]);
				}
				else if (cnt == (uint8_t)6)
				{
					send_serial_character(COM_CH_1, ispis_brzine[5]);
				}
				else if (cnt == (uint8_t)7)
				{
					send_serial_character(COM_CH_1, ispis_brzine[6]);
				}
				else if (cnt == (uint8_t)8)
				{
					send_serial_character(COM_CH_1, ispis_brzine[7]);
				}
				else if (cnt == (uint8_t)9)
				{
					send_serial_character(COM_CH_1, ispis_brzine[8]);
					cnt = (uint8_t)0;
				}
			}
		}
		else
		{
			printf("Ispisivanje puta na kanalu 1\n");
		}
	}
}

static void led_bar_tsk(void* pvParameters)
{
	uint8_t d, d1;
	uint16_t start_put = 0;
	uint16_t stop_put = 0;
	uint16_t prosao_put = 0;
	int i = 0;

	while (1)
	{
		vTaskDelay(pdMS_TO_TICKS(100));

		get_LED_BAR(0, &d);
		get_LED_BAR(1, &d1);

		if (d == (uint8_t)1)
		{
			uint16_t jedinica_up = ukupni_put % (uint16_t)10;
			uint16_t desetica_up = (ukupni_put / (uint16_t)10) % (uint16_t)10;
			uint16_t stotina_up = (ukupni_put / (uint16_t)100) % (uint16_t)10;
			uint16_t hiljada_up = (ukupni_put / (uint16_t)1000) % (uint16_t)10;
			uint16_t d_hiljada_up = ukupni_put / (uint16_t)10000;

			select_7seg_digit(4);
			set_7seg_digit(hexnum[jedinica_up]);
			select_7seg_digit(3);
			set_7seg_digit(hexnum[desetica_up]);
			select_7seg_digit(2);
			set_7seg_digit(hexnum[stotina_up]);
			select_7seg_digit(1);
			set_7seg_digit(hexnum[hiljada_up]);
			select_7seg_digit(0);
			set_7seg_digit(hexnum[d_hiljada_up]);
		}
		else if (d == (uint8_t)2)
		{
			uint16_t jedinica_b = brzina % (uint16_t)10;
			uint16_t desetica_b = (brzina / (uint16_t)10) % (uint16_t)10;
			uint16_t stotina_b = (brzina / (uint16_t)100) % (uint16_t)10;
			uint16_t hiljada_b = (brzina / (uint16_t)1000) % (uint16_t)10;

			select_7seg_digit(4);
			set_7seg_digit(hexnum[jedinica_b]);
			select_7seg_digit(3);
			set_7seg_digit(hexnum[desetica_b]);
			select_7seg_digit(2);
			set_7seg_digit(hexnum[stotina_b]);
			select_7seg_digit(1);
			set_7seg_digit(hexnum[hiljada_b]);
		}
		else if (d == (uint8_t)4)
		{
			uint16_t jedinica_pp = prosao_put % (uint16_t)10;
			uint16_t desetica_pp = (prosao_put / (uint16_t)10) % (uint16_t)10;
			uint16_t stotina_pp = (prosao_put / (uint16_t)100) % (uint16_t)10;
			uint16_t hiljada_pp = (prosao_put / (uint16_t)1000) % (uint16_t)10;
			uint16_t d_hiljada_pp = prosao_put / (uint16_t)10000;

			select_7seg_digit(4);
			set_7seg_digit(hexnum[jedinica_pp]);
			select_7seg_digit(3);
			set_7seg_digit(hexnum[desetica_pp]);
			select_7seg_digit(2);
			set_7seg_digit(hexnum[stotina_pp]);
			select_7seg_digit(1);
			set_7seg_digit(hexnum[hiljada_pp]);
			select_7seg_digit(0);
			set_7seg_digit(hexnum[d_hiljada_pp]);
		}
		else
		{
			for (i = 0; i < 9; i++)
			{
				select_7seg_digit((uint8_t)i);
				set_7seg_digit((uint8_t)0);
			}
		}

		if (d1 == (uint8_t)1)
		{
			start_put = ukupni_put;
			set_LED_BAR(3, 0x08);
		}
		else if (d1 == (uint8_t)2)
		{
			stop_put = ukupni_put;
			prosao_put = stop_put - start_put;
			set_LED_BAR(3, 0x00);
			printf("Prosao put je %d\n", prosao_put);
		}
	}
}

static void SerialSend_Task0(void* pvParameters)
{
	uint8_t trigger = (uint8_t)'i';

	while (1)
	{
		vTaskDelay(pdMS_TO_TICKS(200));
		send_serial_character(COM_CH_0, trigger);
	}

}

static void SerialReceive_Task0(void* pvParameters)
{
	uint8_t cc;

	while (1)
	{
		xSemaphoreTake(RXC_BinarySemaphore0, portMAX_DELAY);
		get_serial_character(COM_CH_0, &cc);

		if (cc == (uint8_t)'I')
		{
			r_point0 = 0;
		}
		else if (cc == (uint8_t)'.')
		{
			char* ostali;
			//printf("Broj inkremenata je %s\n", r_buffer0);
			inkrementi = (uint16_t)strtol(r_buffer0, &ostali, 10);
			xQueueSend(inkrementi_q, &inkrementi, 0);

			r_buffer0[0] = '\0';
			r_buffer0[1] = '\0';
			r_buffer0[2] = '\0';
			r_buffer0[3] = '\0';
			r_buffer0[4] = '\0';
			r_buffer0[5] = '\0';
			r_buffer0[6] = '\0';
		}
		else
		{
			r_buffer0[r_point0++] = (char)cc;
		}
	}
}

static void SerialSend_Task1(void* pvParameters)
{
	uint8_t ispis_puta[8];
	static uint8_t counter = 0;

	while (1)
	{
		vTaskDelay(pdMS_TO_TICKS(1000));

		uint16_t jedinica = ukupni_put % (uint16_t)10;
		uint16_t desetica = (ukupni_put / (uint16_t)10) % (uint16_t)10;
		uint16_t stotina = (ukupni_put / (uint16_t)100) % (uint16_t)10;
		uint16_t hiljada = (ukupni_put / (uint16_t)1000) % (uint16_t)10;
		uint16_t d_hiljada = ukupni_put / (uint16_t)10000;

		ispis_puta[0] = (uint16_t)d_hiljada + (uint16_t)'0';
		ispis_puta[1] = (uint16_t)hiljada + (uint16_t)'0';
		ispis_puta[2] = (uint16_t)stotina + (uint16_t)'0';
		ispis_puta[3] = (uint16_t)desetica + (uint16_t)'0';
		ispis_puta[4] = (uint16_t)jedinica + (uint16_t)'0';
		ispis_puta[5] = (uint16_t)'c';
		ispis_puta[6] = (uint16_t)'m';
		ispis_puta[7] = (uint16_t)'\n';

		get_LED_BAR(2, &d2);

		if (d2 == (uint8_t)2)
		{
			printf("Ispisivanje brzine na kanalu 1\n");
		}
		else
		{
			counter++;

			if (ukupni_put != (uint8_t)0)
			{
				if (counter == (uint8_t)1)
				{
					send_serial_character(COM_CH_1, ispis_puta[0]);
				}
				else if (counter == (uint8_t)2)
				{
					send_serial_character(COM_CH_1, ispis_puta[1]);
				}
				else if (counter == (uint8_t)3)
				{
					send_serial_character(COM_CH_1, ispis_puta[2]);
				}
				else if (counter == (uint8_t)4)
				{
					send_serial_character(COM_CH_1, ispis_puta[3]);
				}
				else if (counter == (uint8_t)5)
				{
					send_serial_character(COM_CH_1, ispis_puta[4]);
				}
				else if (counter == (uint8_t)6)
				{
					send_serial_character(COM_CH_1, ispis_puta[5]);
				}
				else if (counter == (uint8_t)7)
				{
					send_serial_character(COM_CH_1, ispis_puta[6]);
				}
				else if (counter == (uint8_t)8)
				{
					send_serial_character(COM_CH_1, ispis_puta[7]);
					counter = (uint8_t)0;
				}
			}
		}	
	}
}

static void SerialReceive_Task1(void* pvParameters)
{
	uint8_t cc;
	uint8_t cnt = 0;

	while (1)
	{
		xSemaphoreTake(RXC_BinarySemaphore1, portMAX_DELAY);
		get_serial_character(COM_CH_1, &cc);

		if (cc == (uint8_t)0x00)
		{
			cnt = 0;
			r_point1 = 0;
		}
		else if (cc == (uint8_t)13)
		{
			if (r_buffer1[0] == 'O' && r_buffer1[1] == 'M')
			{
				size_t i;
				for (i = (size_t)0; i < strlen(r_buffer1); i++)
				{
					if (r_buffer1[i] == '0' || r_buffer1[i] == '1' || r_buffer1[i] == '2' || r_buffer1[i] == '3' || r_buffer1[i] == '4' || r_buffer1[i] == '5' || r_buffer1[i] == '6' || r_buffer1[i] == '7' || r_buffer1[i] == '8' || r_buffer1[i] == '9')
					{
						obim[cnt] = r_buffer1[i];
						cnt++;
					}
				}
			}

			char* ostali;
			obim_tocka = (uint16_t)strtol(obim, &ostali, 10);
			printf("Obim tocka je %d\n", obim_tocka);

			r_buffer1[0] = '\0';
			r_buffer1[1] = '\0';
			r_buffer1[2] = '\0';
			r_buffer1[3] = '\0';
			r_buffer1[4] = '\0';
			r_buffer1[5] = '\0';
			r_buffer1[6] = '\0';
			r_buffer1[7] = '\0';
			r_buffer1[8] = '\0';
			r_buffer1[9] = '\0';
			r_buffer1[10] = '\0';
			r_buffer1[11] = '\0';
		}
		else
		{
			r_buffer1[r_point1++] = (char)cc;
		}
	}
}

/* MAIN - SYSTEM STARTUP POINT */
void main_demo( void )
{
	//incijalizacija periferija
	init_7seg_comm(); //inicijalizacija 7seg displeja
	init_LED_comm(); //inicijalizacija LED bara
	init_serial_uplink(COM_CH_0); // inicijalizacija serijske TX na kanalu 0
	init_serial_downlink(COM_CH_0); // inicijalizacija serijske TX na kanalu 0
	init_serial_uplink(COM_CH_1); // inicijalizacija serijske TX na kanalu 1
	init_serial_downlink(COM_CH_1); // inicijalizacija serijske TX na kanalu 1
	
	/* SERIAL RECEPTION INTERRUPT HANDLER */
	vPortSetInterruptHandler(portINTERRUPT_SRL_RXC, prvProcessRXCInterrupt); //prekid za prijem sa serijske komunikacije
	
	/* Kreiranje semafora */
	RXC_BinarySemaphore0 = xSemaphoreCreateBinary();
	if (RXC_BinarySemaphore0 == NULL) { while (1); } //ako se semafor nije pravilno kreirao zakucavamo program

	RXC_BinarySemaphore1 = xSemaphoreCreateBinary();
	if (RXC_BinarySemaphore1 == NULL) { while (1); } //ako se semafor nije pravilno kreirao zakucavamo programs

	/* Kreiranje redova */
	inkrementi_q = xQueueCreate(10, sizeof(uint16_t));
	if (inkrementi_q == NULL) { while (1); }

	/* Kreiranje taskova */
	BaseType_t status;

	status = xTaskCreate(
		mjerenje_brzine,
		"task za brzinu",
		configMINIMAL_STACK_SIZE,
		NULL,
		(UBaseType_t)SERVICE_TASK_PRI,
		NULL
	);
	if (status != pdPASS) { while (1); } //ako task nije pravilno kreiran zakucavamo program

	status = xTaskCreate(
		ukupni_predjeni_put,
		"task za put",
		configMINIMAL_STACK_SIZE,
		NULL,
		(UBaseType_t)SERVICE_TASK_PRI,
		NULL
	);
	if (status != pdPASS) { while (1); } //ako task nije pravilno kreiran zakucavamo program

	status = xTaskCreate(
		led_bar_tsk,
		"led bar task",
		configMINIMAL_STACK_SIZE,
		NULL,
		(UBaseType_t)SERVICE_TASK_PRI,
		NULL
	);
	if (status != pdPASS) { while (1); } //ako task nije pravilno kreiran zakucavamo program

	status = xTaskCreate(
		SerialSend_Task0,
		"STx",
		configMINIMAL_STACK_SIZE,
		NULL,
		(UBaseType_t)TASK_SERIAL_SEND_PRI,
		NULL
	);
	if (status != pdPASS) { while (1); } //ako task nije pravilno kreiran zakucavamo program

	status = xTaskCreate(
		SerialSend_Task1,
		"STx",
		configMINIMAL_STACK_SIZE,
		NULL,
		(UBaseType_t)TASK_SERIAL_SEND_PRI,
		NULL
	);
	if (status != pdPASS) { while (1); } //ako task nije pravilno kreiran zakucavamo program

	status = xTaskCreate(
		SerialReceive_Task0,
		"SRx",
		configMINIMAL_STACK_SIZE,
		NULL,
		(UBaseType_t)TASK_SERIAL_REC_PRI,
		NULL
	);
	if (status != pdPASS) { while (1); } //ako task nije pravilno kreiran zakucavamo program

	status = xTaskCreate(
		SerialReceive_Task1,
		"SRx",
		configMINIMAL_STACK_SIZE,
		NULL,
		(UBaseType_t)TASK_SERIAL_REC_PRI,
		NULL
	);
	if (status != pdPASS) { while (1); } //ako task nije pravilno kreiran zakucavamo program

	r_point0 = 0;
	r_point1 = 0;

	vTaskStartScheduler();

	while (1);
}
