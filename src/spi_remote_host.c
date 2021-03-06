#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include "binary_semaphore.h"
#include "spi_proto.h"
#include "spi_remote_api.h"
#include "spi_remote_host.h"
#include "spi_chunk_defines.h"

//TODO temp
#define CHUNK_LEN_DAC CHUNK_LEN_DAC_S2M
#define CHUNK_LEN_ADC CHUNK_LEN_ADC_S2M
#define CHUNK_LEN_GPIO CHUNK_LEN_GPIO_S2M

//TODO temp for compilation
int unknown_chunk_type_msg_count;
extern int out_of_range_chunks;
uint16_t bad_chunk_counter;


//TODO possible issue, when the thread takes on the semaphore it could end up getting the value of a previous read that some other thread triggered
//a sempahore with a queue where only one taker is released per give would solve this issue

void
host_remote_init(struct host_remote *r)
{
	//currently only initialize the semaphores
	
	for (int i = 0; i < ADC_NUM; i++) {
		bisem_init(&r->adc[i].sem);
	}
	for (int i = 0; i < GPIO_NUM; i++) {
		bisem_init(&r->gpio[i].sem);
	}
}

//TODO add flow sensor stuff

int
remote_chunk_handler(struct host_remote *r, uint8_t *buf, size_t len)
{
	//go through chunk types and bounce semaphores, maybe store a value
	if (len < 2) return -1; // length zero isn't a real chunk, length 1 can't carry data
	switch(buf[1]) {
	case CHUNK_TYPE_GPIO:
		if (len < CHUNK_LEN_GPIO) {bad_chunk_counter++;break;}
		// [LEN|TYPE|ID|CMD|VAL]
		struct gpio_response gpiocmd;
		gpiocmd.gpio_id = buf[2];
		gpiocmd.cmd = buf[3];
		gpiocmd.val = buf[4];
		gpio_handle_master(r->gpio, GPIO_NUM, &gpiocmd);
		break;
	case CHUNK_TYPE_ADC:
		if (len < CHUNK_LEN_ADC) {bad_chunk_counter++;break;}
		// [LEN|TYPE|ID|CMD|VAL|VAL|VAL|VAL]
		struct adc_response adccmd;
		adccmd.adc_id = buf[2];
		adccmd.cmd = buf[3];
		adccmd.val = buf[4] + (buf[5]<<8) + (buf[6]<<16) + (buf[7]<<24);
		adc_handle_master(r->adc, ADC_NUM, &adccmd);
		break;
	case CHUNK_TYPE_DAC:
		if (len < CHUNK_LEN_DAC) {bad_chunk_counter++;break;}
		// [LEN|TYPE|ID|CMD]
		struct dac_response daccmd;
		daccmd.dac_id = buf[2];
		daccmd.cmd = buf[3];
		dac_handle_master(r->dac, DAC_NUM, &daccmd);
		break;
	//TODO other chunk types
	default:
		unknown_chunk_type_msg_count++; // TODO think of better name
		return -2;
	}
	return 0;
}

void
adc_handle_master(struct host_adc *adc, size_t n, struct adc_response *a)
{
	if (a->adc_id > ADC_NUM) {out_of_range_chunks++;return;}
	if (a->cmd == OP_GET) {
		adc[a->adc_id].last_read = a->val;
		bisem_post(&adc[a->adc_id].sem);
	} else {
		//TODO other ops
		puts("TODO other case in adc_handle_master");
	}
}

void
print_gpio_response(struct gpio_response *c)
{
	printf("\tgpio_id:\t%d\n", c->gpio_id);
	printf("\tcmd:\t%d\n", c->cmd);
	printf("\tval:\t0x%04x\n", c->val);
}

void
gpio_handle_master(struct host_gpio *gpio, size_t n, struct gpio_response *c)
{
	if (c->gpio_id > GPIO_NUM) {out_of_range_chunks++;return;}
	if (c->cmd == OP_GET) {
		//printf("GET gpio: \n");
		//print_gpio_response(c);
		gpio[c->gpio_id].last_read = c->val;
		//printf("posting gpio sem %d\n", c->gpio_id);
		bisem_post(&gpio[c->gpio_id].sem);
	} else if (c->cmd == OP_SET || c->cmd == OP_SET_META) {
		//printf("posting gpio sem %d\n", c->gpio_id);
		bisem_post(&gpio[c->gpio_id].sem);
	} else {
		//TODO other ops
		puts("TODO other case in gpio_handle_master");
	}
}

void
dac_handle_master(struct host_dac *dac, size_t n, struct dac_response *c)
{
	if (c->dac_id > DAC_NUM) {out_of_range_chunks++;return;}
	if (c->cmd == OP_SET) {
		bisem_post(&dac[c->dac_id].sem);
	} else {
		//TODO other ops
		puts("TODO other case in dac_handle_master");
	}
}
