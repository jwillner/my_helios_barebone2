/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/console/console.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/adc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 

#define BUFFER_LENGTH 255

/* SW0_NODE is the devicetree node identifier for the node with alias "sw0" */
#define SW0_NODE	DT_ALIAS(sw0) 
#define SW1_NODE	DT_ALIAS(sw1) 
static const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET(SW0_NODE, gpios);
static const struct gpio_dt_spec button2 = GPIO_DT_SPEC_GET(SW1_NODE, gpios);

/* LED0_NODE is the devicetree node identifier for the node with alias "led0". */
#define LED0_NODE	DT_ALIAS(led3)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* PWM nodes with alias pwm_led0, pwm_led1, pwm_led2                           */
static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));
static const struct pwm_dt_spec pwm_led1 = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led1));
static const struct pwm_dt_spec pwm_led2 = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led2));
#define PWM_PERIOD 10000 

/* ADC Nodes from devicetree */
#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
	ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels,
			     DT_SPEC_AND_COMMA)
};


static char command_string[BUFFER_LENGTH];
static char address_string[BUFFER_LENGTH];
static char data_string[BUFFER_LENGTH];
static char option_string[BUFFER_LENGTH];


//static char input_string[BUFFER_LENGTH] ; 
char *input_string;
uint8_t helios_state1=0;
uint8_t helios_state2=0;

char version[]="Helios TIIMax SW200219A (c) joachim willner 22.8.2023\n";

#define HELP_MEMBERS 9
char help_text[HELP_MEMBERS][BUFFER_LENGTH]=
{
"h\n",
"\n",
"\n",
"\n",
"\n",
"\n",
"\n",
"\n",
"\n"
};

void button1_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    gpio_pin_toggle_dt(&led);
	helios_state1++;
}

void button2_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    gpio_pin_toggle_dt(&led);
	helios_state2++ ;
}

static struct gpio_callback button1_cb_data;
static struct gpio_callback button2_cb_data;

void interrupt_and_gpio_init(void)
{
	int ret;
	if (!device_is_ready(led.port)) {
		return;
	}
	if (!device_is_ready(button1.port)) {
		return;
	}
	if (!device_is_ready(button2.port)) {
		return;
	}
	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return;
	}
	ret = gpio_pin_configure_dt(&button1, GPIO_INPUT);
	if (ret < 0) {
		return;
	}
	ret = gpio_pin_configure_dt(&button2, GPIO_INPUT);
	if (ret < 0) {
		return;
	}
	ret = gpio_pin_interrupt_configure_dt(&button1, GPIO_INT_EDGE_TO_ACTIVE);
	ret = gpio_pin_interrupt_configure_dt(&button2, GPIO_INT_EDGE_TO_ACTIVE);	
    gpio_init_callback(&button1_cb_data, button1_pressed, BIT(button1.pin)); 	
    gpio_init_callback(&button2_cb_data, button2_pressed, BIT(button2.pin)); 
	gpio_add_callback(button1.port, &button1_cb_data);
	gpio_add_callback(button2.port, &button2_cb_data);	
}          

void pwm_init(void)
{
	if (!device_is_ready(pwm_led0.dev)) {
		printk("Error: PWM device %s is not ready\n",
				pwm_led0.dev->name);
	}

	if (!device_is_ready(pwm_led1.dev)) {
		printk("Error: PWM device %s is not ready\n",
				pwm_led1.dev->name);
		}
	
	if (!device_is_ready(pwm_led2.dev)) {
		printk("Error: PWM device %s is not ready\n",
				pwm_led2.dev->name);
	}
}

void adc_init(void)
{
	int err;
	/* Configure channels individually prior to sampling. */

		if (!device_is_ready(adc_channels[0].dev)) {
			printk("ADC controller device %s not ready\n", adc_channels[0].dev->name);
			return 0;
		}


		err = adc_channel_setup_dt(&adc_channels[0]);
		if (err < 0) {
			printk("Could not setup channel #0 (%d)\n", err);
		}
		err = adc_channel_setup_dt(&adc_channels[1]);
		if (err < 0) {
			printk("Could not setup channel #1 (%d)\n", err);
		}
		err = adc_channel_setup_dt(&adc_channels[2]);
		if (err < 0) {
			printk("Could not setup channel #2 (%d)\n", err);
		}
        if(err<0) return; 
}

//--adc--------------------------------------------------------------------------------------------------

int32_t read_adc(uint8_t channel)
{
	int err;
	uint16_t buf;
	int32_t val_mv;
	struct adc_sequence sequence = {
		.buffer = &buf,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(buf),
	};

    switch(channel){
	case 1:
		(void)adc_sequence_init_dt(&adc_channels[0], &sequence);
			err = adc_read(adc_channels[0].dev, &sequence);
			if (err < 0) {
				printk("Could not read channel0 err=(%d)\n", err);
            	return;
			}
			val_mv = (int32_t)buf;
		break;
		
    case 2: 
				(void)adc_sequence_init_dt(&adc_channels[1], &sequence);
			err = adc_read(adc_channels[1].dev, &sequence);
			if (err < 0) {
				printk("Could not read channel1 err=(%d)\n", err);
            	return;
			}
			val_mv = (int32_t)buf;
		break;
	case 3:
			(void)adc_sequence_init_dt(&adc_channels[2], &sequence);
			err = adc_read(adc_channels[2].dev, &sequence);
			if (err < 0) {
				printk("Could not read channel2(%d)\n", err);
			    return;
			}
			val_mv = (int32_t)buf;
		break;
    default:
	        val_mv = -32768;
	}
	return(val_mv);
}


void command_interpreter(void)
{
	char *input_string = console_getline();
	input_string = strupr(input_string);
	sscanf(input_string,"%s %s %s %s",command_string,address_string,data_string,option_string);
	printk("command=%s address=%s data=%s option=%s\n",command_string,address_string,data_string,option_string);

 //--help---------------------------------------------------------------------------------------
  if((strcmp("HELP",command_string)==0)||(strcmp("?",command_string)==0))
  {
      for (int i = 0; i < HELP_MEMBERS; i++)
      {
        printk("%s", help_text[i]);
      }
      return;
  }

 //--version----------------------------------------------------------------------------------
  if((strcmp("VERSION",command_string)==0)||(strcmp("V",command_string)==0))
  {
      printk("%s", version);
      return;
  }

   //--state----------------------------------------------------------------------------------
  if((strcmp("STATE",command_string)==0)||(strcmp("S",command_string)==0))
  {
	  printk("Helios-State1 = %d Helios-State2 = %d\n",helios_state1,helios_state2);
      return;
  }

   //--pwm----------------------------------------------------------------------------------
  if((strcmp("PWM",command_string)==0)||(strcmp("P",command_string)==0))
  {
      if((strcmp("CH1",address_string)==0)||(strcmp("1",address_string)==0))
	  {
		uint8_t pwm_value;
		pwm_value = atoi(data_string);
		if((pwm_value>-1)||(pwm_value<101))
		{
			pwm_set_dt(&pwm_led0, PWM_PERIOD, 100*pwm_value);
			return;
		}
		printk("out of value range");
		return;
	  }

      if((strcmp("CH2",address_string)==0)||(strcmp("2",address_string)==0))
	  {
		uint8_t pwm_value;
		pwm_value = atoi(data_string);
		if((pwm_value>-1)||(pwm_value<101))
		{
			pwm_set_dt(&pwm_led1, PWM_PERIOD, 100*pwm_value);
			return;
		}
		printk("out of value range");
		return;
	  }

	  if((strcmp("CH3",address_string)==0)||(strcmp("3",address_string)==0))
	  {
		uint8_t pwm_value;
		pwm_value = atoi(data_string);
		if((pwm_value>-1)||(pwm_value<101))
		{
			pwm_set_dt(&pwm_led2, PWM_PERIOD, 100*pwm_value);
			return;
		}
		printk("out of value range");
		return;
	  }
      return;
  }

   //--adc----------------------------------------------------------------------------------
  if((strcmp("ADC",command_string)==0)||(strcmp("A",command_string)==0))
  {
	  int32_t adc_value;
      if((strcmp("CH1",address_string)==0)||(strcmp("1",address_string)==0))
	  {
		adc_value = read_adc(1);
		printk("%d\n",adc_value);
		return;
	  }

      if((strcmp("CH2",address_string)==0)||(strcmp("2",address_string)==0))
	  {
		adc_value = read_adc(2);
		printk("%d\n",adc_value);
		return;
	  }

	  if((strcmp("CH3",address_string)==0)||(strcmp("3",address_string)==0))
	  {
		adc_value = read_adc(3);
		printk("%d\n",adc_value);
		return;
	  }
      return;
  }

  //--no valid command----------------------------------------------------------------------------------
   printk("no valid command found\n");
}


int main(void)
{	
	interrupt_and_gpio_init();
	pwm_init();
	adc_init();

	console_getline_init();
	printk("enter some text for gpio control...\n");
	while(1)
	{
			//char *s = console_getline();
            command_interpreter();
			k_msleep(100);
	} 
}
