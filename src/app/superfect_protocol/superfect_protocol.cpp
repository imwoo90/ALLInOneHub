
#include <superfect_protocol.h>
#include <zephyr/shell/shell.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(Sprotocol);

#include <Controller.h>

#define SUPERFECT_PROTOCOL_PRIORITY K_LOWEST_APPLICATION_THREAD_PRIO-1

#define PACKET_START 0X02
#define PACKET_END 0X03

struct superfect_uart_user_data {
    SuperfectFormat fmt;
    k_sem sem;
};

void serialCallback(const struct device *dev, void *user_data) {
    superfect_uart_user_data* data = (superfect_uart_user_data*)user_data;
    uint8_t packet_s, packet_e, c;
    SuperfectFormat fmt;

    if (!uart_irq_update(dev)) {
        return;
    }

    if (!uart_irq_rx_ready(dev)) {
        return;
    }

    uart_fifo_read(dev, &packet_s, 1);
    if (packet_s != PACKET_START){
        while (uart_fifo_read(dev, &c, 1) == 1); /* read until FIFO empty */
        LOG_WRN("uart fifo flush fail packet start");
        return;
    }


    uart_fifo_read(dev, &c, 1);
    fmt.command = (CommandType)c;

    uart_fifo_read(dev, (uint8_t*)&fmt.packet_length, 4);
    for (uint32_t i = 0; i < fmt.packet_length; i++) {
        uart_fifo_read(dev, &c, 1);
        fmt.body.push_back(c);
    }

    uart_fifo_read(dev, &packet_e, 1);
    if (packet_e != PACKET_END) {
        while (uart_fifo_read(dev, &c, 1) == 1); /* read until FIFO empty */
        LOG_WRN("uart fifo flush fail packet end");
        return;
    }

    data->fmt = fmt;
    k_sem_give(&data->sem);
}

void superfectSend(const struct device *dev, CommandType cmd, uint8_t body[], uint32_t length) {
    uint32_t i = 0;
    uint8_t* buf = (uint8_t*)k_malloc(7 + length);
    buf[i++] = PACKET_START;
    buf[i++] = cmd;
    buf[i++] = (length >> 24) & 0xff;
    buf[i++] = (length >> 16) & 0xff;
    buf[i++] = (length >> 8) & 0xff;
    buf[i++] = length & 0xff;
    for (; i < 6 + length; i++)
        buf[i] = body[i - 6];
    buf[i] = PACKET_END;
    uart_fifo_fill(dev, buf, 7+length);
    k_free(buf);
}

const struct device *backend_uart = DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart1));
static void superfectBackendHandler(void) {
    superfect_uart_user_data _data;
    k_sem_init(&_data.sem, 0, 1);
    uart_irq_callback_user_data_set(backend_uart, serialCallback, &_data);
    uart_irq_rx_enable(backend_uart);

    while(k_sem_take(&_data.sem,  K_FOREVER) == 0) {
        switch(_data.fmt.command) {
        case TIME_SYNC:
            LOG_INF("TIME_SYNC %s", __func__);
            break;
        case POWER_MANAGEMENT:
            LOG_INF("POWER_MANAGEMENT %s", __func__);
            break;
        case COLOR_TABLE:
            LOG_INF("COLOR_TABLE %s", __func__);
            break;
        case ACK:
            if (_data.fmt.body[0] == 0x36) {
                uint8_t body = 0x37;
                superfectSend(backend_uart, ACK, &body, 1);
            } else if (_data.fmt.body[0] == 0x00) {
                uint8_t body = 0x00;
                Controller::getInstance()->putMessage(MSG_BACKEND_PING, NULL);
                superfectSend(backend_uart, ACK, &body, 1);
            }
            break;
        case POWEROFF_SCHEDULE:
            LOG_INF("POWEROFF_SCHEDULE %s", __func__);
            break;
        default:
            LOG_ERR("Unkwon command parse %s", __func__);
            break;
        }
        // LOG_INF("command %x, len %d, body %s", _data.fmt.command, _data.fmt.packet_length, _data.fmt.body.c_str());
    }
}
K_THREAD_DEFINE(superfect_protocol_backend, 2048, superfectBackendHandler, NULL, NULL, NULL,
		SUPERFECT_PROTOCOL_PRIORITY, 0, 0);




static void timeSync(std::string &time)
{
    std::string year = time.substr(0, 4);
    std::string month = time.substr(4, 2);
    std::string day = time.substr(6, 2);
    std::string hour = time.substr(8, 2);
    std::string minute = time.substr(10, 2);
    std::string second = time.substr(12, 2);
    std::string cmd = "date set " + year + "-" + month + "-" + day + " " + hour + ":" + minute + ":" + second + "\n";
    shell_execute_cmd(NULL, cmd.c_str());
}

const struct device *config_uart = DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart2));
static void superfectConfigHandler(void)
{
    superfect_uart_user_data _data;
    k_sem_init(&_data.sem, 0, 1);
    uart_irq_callback_user_data_set(config_uart, serialCallback, &_data);
    uart_irq_rx_enable(config_uart);

    while(k_sem_take(&_data.sem,  K_FOREVER) == 0) {
        switch(_data.fmt.command) {
        case TIME_SYNC:
            timeSync(_data.fmt.body);
            break;
        case POWER_MANAGEMENT:
            LOG_INF("POWER_MANAGEMENT %s", __func__);
            break;
        case COLOR_TABLE:
            LOG_INF("COLOR_TABLE %s", __func__);
            break;
        case ACK:
            if (_data.fmt.body[0] == 0x36) {
                uint8_t body = 0x38;
                superfectSend(config_uart, ACK, &body, 1);
            }
            break;
        case POWEROFF_SCHEDULE:
            LOG_INF("POWEROFF_SCHEDULE %s", __func__);
            break;
        default:
            LOG_ERR("Unkwon command parse %s", __func__);
            break;
        }
        // LOG_INF("command %x, len %d, body %s", _data.fmt.command, _data.fmt.packet_length, _data.fmt.body.c_str());
    }
}
K_THREAD_DEFINE(superfect_protocol_config, 2048, superfectConfigHandler, NULL, NULL, NULL,
		SUPERFECT_PROTOCOL_PRIORITY, 0, 0);