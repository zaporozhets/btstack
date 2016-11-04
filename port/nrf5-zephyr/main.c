/**
 * BTstack port for Zephyr Bluetooth Controller
 */

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <zephyr.h>
#include <arch/cpu.h>
#include <misc/byteorder.h>
#include <misc/sys_log.h>
#include <misc/util.h>

#include <device.h>
#include <init.h>
#include <uart.h>

#include <net/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/buf.h>
#include <bluetooth/hci_raw.h>
#include "ll.h"

#include <misc/kernel_event_logger.h>

#include "btstack_event.h"
#include "btstack_memory.h"
#include "btstack_run_loop.h"
#include "hci.h"
#include "hci_dump.h"
#include "hci_transport.h"
#include "btstack_run_loop_embedded.h"

/* incoming events and data from the controller */
static struct nano_fifo rx_queue;

static btstack_packet_callback_registration_t hci_event_callback_registration;

static void (*transport_packet_handler)(uint8_t packet_type, uint8_t *packet, uint16_t size);

/* HCI command buffers */
#define CMD_BUF_SIZE (CONFIG_BLUETOOTH_HCI_SEND_RESERVE + \
		      sizeof(struct bt_hci_cmd_hdr) + \
		      CONFIG_BLUETOOTH_MAX_CMD_LEN)

static struct nano_fifo avail_cmd_tx;
static NET_BUF_POOL(cmd_tx_pool, CONFIG_BLUETOOTH_HCI_CMD_COUNT, CMD_BUF_SIZE,
		    &avail_cmd_tx, NULL, BT_BUF_USER_DATA_MIN);

#define BT_L2CAP_MTU 64
/** Data size needed for ACL buffers */
#define BT_BUF_ACL_SIZE (CONFIG_BLUETOOTH_HCI_RECV_RESERVE + \
			 sizeof(struct bt_hci_acl_hdr) + \
			 4 /* L2CAP header size */ + \
			 BT_L2CAP_MTU)

static struct nano_fifo avail_acl_tx;
static NET_BUF_POOL(acl_tx_pool, CONFIG_BLUETOOTH_CONTROLLER_TX_BUFFERS,
		    BT_BUF_ACL_SIZE, &avail_acl_tx, NULL, BT_BUF_USER_DATA_MIN);

static struct nano_fifo tx_queue;

// hal_cpu.h
// TODO: implement
#include "hal_cpu.h"
void hal_cpu_disable_irqs(void){}
void hal_cpu_enable_irqs(void){}
void hal_cpu_enable_irqs_and_sleep(void){}

// hal_time_ms.h
#include <sys_clock.h>
static int sys_clock_ms_per_tick = 1000 / sys_clock_ticks_per_sec;
uint32_t hal_time_ms(void){
	return sys_tick_get_32() * sys_clock_ms_per_tick;
}

static void deliver_controller_packet(void){
	struct net_buf *buf;
	buf = net_buf_get_timeout(&rx_queue, 0, TICKS_NONE); // TICKS_NONE vs TICKS_UNLIMITED
	if (buf){
		uint16_t    size = buf->len;
		uint8_t * packet = buf->data;
		switch (bt_buf_get_type(buf)) {
			case BT_BUF_ACL_IN:
				transport_packet_handler(HCI_ACL_DATA_PACKET, packet, size);
				break;
			case BT_BUF_EVT:
				transport_packet_handler(HCI_EVENT_PACKET, packet, size);
				break;
			default:
				printf("Unknown type %u\n", bt_buf_get_type(buf));
				net_buf_unref(buf);
				break;
		}
		net_buf_unref(buf);
	}
}

// hci_transport_zephyr.c

static btstack_data_source_t hci_transport_data_source;

static void transport_run(btstack_data_source_t *ds, btstack_data_source_callback_type_t callback_type){
	deliver_controller_packet();
}

/**
 * init transport
 * @param transport_config
 */
static void transport_init(const void *transport_config){
	/* Initialize the buffer pools */
	net_buf_pool_init(cmd_tx_pool);
	net_buf_pool_init(acl_tx_pool);

	/* Initialize the FIFOs */
	nano_fifo_init(&tx_queue);
	nano_fifo_init(&rx_queue);

	/* startup Controller */
	bt_enable_raw(&rx_queue);

	// use 11:22:33:44:55:66
	uint8_t addr[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
	ll_address_set(0, addr);
}

/**
 * open transport connection
 */
static int transport_open(void){
    btstack_run_loop_set_data_source_handler(&hci_transport_data_source, &transport_run);
    btstack_run_loop_enable_data_source_callbacks(&hci_transport_data_source, DATA_SOURCE_CALLBACK_POLL);
    btstack_run_loop_add_data_source(&hci_transport_data_source);
    return 0;
}

/**
 * close transport connection
 */
static int transport_close(void){
    btstack_run_loop_remove_data_source(&hci_transport_data_source);
    return 0;
}

/**
 * register packet handler for HCI packets: ACL, SCO, and Events
 */
static void transport_register_packet_handler(void (*handler)(uint8_t packet_type, uint8_t *packet, uint16_t size)){
    transport_packet_handler = handler;
}

static void send_hardware_error(uint8_t error_code){
    // hci_outgoing_event[0] = HCI_EVENT_HARDWARE_ERROR;
    // hci_outgoing_event[1] = 1;
    // hci_outgoing_event[2] = error_code;
    // hci_outgoing_event_ready = 1;
}

static int transport_send_packet(uint8_t packet_type, uint8_t *packet, int size){
	struct net_buf *buf;
    switch (packet_type){
        case HCI_COMMAND_DATA_PACKET:
			buf = net_buf_get(&avail_cmd_tx, 0);
			if (buf) {
				bt_buf_set_type(buf, BT_BUF_CMD);
				memcpy(net_buf_add(buf, size), packet, size);
				bt_send(buf);
			} else {
				printf("No available command buffers!\n");
			}
            break;
        case HCI_ACL_DATA_PACKET:
			buf = net_buf_get(&avail_acl_tx, 0);
			if (buf) {
				bt_buf_set_type(buf, BT_BUF_ACL_OUT);
				memcpy(net_buf_add(buf, size), packet, size);
				bt_send(buf);
			} else {
				printf("No available ACL buffers!\n");
			}
            break;
        default:
            send_hardware_error(0x01);  // invalid HCI packet
            break;
    }

    return 0;   
}

static const hci_transport_t transport = {
    /* const char * name; */                                        "nRF5-Zephyr",
    /* void   (*init) (const void *transport_config); */            &transport_init,
    /* int    (*open)(void); */                                     &transport_open,
    /* int    (*close)(void); */                                    &transport_close,
    /* void   (*register_packet_handler)(void (*handler)(...); */   &transport_register_packet_handler,
    /* int    (*can_send_packet_now)(uint8_t packet_type); */       NULL,
    /* int    (*send_packet)(...); */                               &transport_send_packet,
    /* int    (*set_baudrate)(uint32_t baudrate); */                NULL,
    /* void   (*reset_link)(void); */                               NULL,
};

static const hci_transport_t * transport_get_instance(void){
	return &transport;
}

// hal_time_ms.c


// main.c

static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    if (packet_type != HCI_EVENT_PACKET) return;
    if (hci_event_packet_get_type(packet) != BTSTACK_EVENT_STATE) return;
    if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
    printf("BTstack up and running.\n");
}

int btstack_main(void);

void main(void)
{
	printf("BTstack booting up..\n");

	// start with BTstack init - especially configure HCI Transport
    btstack_memory_init();
    btstack_run_loop_init(btstack_run_loop_embedded_get_instance());

    // enable full log output while porting
    hci_dump_open(NULL, HCI_DUMP_STDOUT);

    // init HCI
    hci_init(transport_get_instance(), NULL);

    // inform about BTstack state
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);
    
    // hand over to btstack embedded code 
    btstack_main();

    // go
    btstack_run_loop_execute();

    while (1){};
}
