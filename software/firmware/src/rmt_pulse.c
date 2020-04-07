#include "rmt_pulse.h"
#include "driver/rmt.h"

static intr_handle_t gRMT_intr_handle = NULL;

// the RMT channel configuration object
static rmt_config_t row_rmt_config;

// keep track of wether the current pulse is ongoing
volatile bool rmt_tx_done = true;

/**
 * Remote peripheral interrupt. Used to signal when transmission is done.
 */
static void IRAM_ATTR rmt_interrupt_handler(void *arg) {
  rmt_tx_done = true;
  RMT.int_clr.val = RMT.int_st.val;
}

void rmt_pulse_init(gpio_num_t pin) {

  row_rmt_config.rmt_mode = RMT_MODE_TX;
  // currently hardcoded: use channel 0
  row_rmt_config.channel = RMT_CHANNEL_0;

  row_rmt_config.gpio_num = pin;
  row_rmt_config.mem_block_num = 1;

  // Divide 80MHz APB Clock by 80 -> 1us resolution delay
  row_rmt_config.clk_div = 80;

  row_rmt_config.tx_config.loop_en = false;
  row_rmt_config.tx_config.carrier_en = false;
  row_rmt_config.tx_config.carrier_level = RMT_CARRIER_LEVEL_LOW;
  row_rmt_config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
  row_rmt_config.tx_config.idle_output_en = true;

  esp_intr_alloc(ETS_RMT_INTR_SOURCE, ESP_INTR_FLAG_LEVEL3,
                 rmt_interrupt_handler, 0, &gRMT_intr_handle);

  heap_caps_print_heap_info(MALLOC_CAP_INTERNAL);

  rmt_config(&row_rmt_config);
  rmt_set_tx_intr_en(row_rmt_config.channel, true);
}

void IRAM_ATTR pulse_ckv_us(uint16_t high_time_us, uint16_t low_time_us,
                            bool wait) {
  while (!rmt_tx_done) {
  };
  volatile rmt_item32_t *rmt_mem_ptr =
      &(RMTMEM.chan[row_rmt_config.channel].data32[0]);
  if (high_time_us > 0) {
    rmt_mem_ptr->level0 = 1;
    rmt_mem_ptr->duration0 = high_time_us;
    rmt_mem_ptr->level1 = 0;
    rmt_mem_ptr->duration1 = low_time_us;
  } else {
    rmt_mem_ptr->level0 = 1;
    rmt_mem_ptr->duration0 = low_time_us;
    rmt_mem_ptr->level1 = 0;
    rmt_mem_ptr->duration1 = 0;
  }
  RMTMEM.chan[row_rmt_config.channel].data32[1].val = 0;
  rmt_tx_done = false;
  rmt_tx_start(row_rmt_config.channel, true);
  while (wait && !rmt_tx_done) {
  };
}