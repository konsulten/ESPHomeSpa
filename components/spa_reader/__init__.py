import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import uart
from esphome.const import CONF_ID

DEPENDENCIES = ['uart']

spa_reader_ns = cg.esphome_ns.namespace('spa_reader')
SpaReader = spa_reader_ns.class_('SpaReader', cg.Component, uart.UARTDevice, cg.PollingComponent)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(SpaReader),
}).extend(cv.polling_component_schema('1s')).extend(uart.UART_DEVICE_SCHEMA)

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield uart.register_uart_device(var, config)
    cg.add(var.set_uart_pin(config[uart.CONF_TX_PIN]))
    cg.add(var.set_uart_pin(config[uart.CONF_RX_PIN]))
