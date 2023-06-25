#include <Adafruit_NeoPixel.h>
#include <vector>
#include <string>
#include <sys/stdio.h>
#include <sys/fcntl.h>
#include <sys/unistd.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(SN);

#include <zephyr/shell/shell.h>

#define STYLE_FILE_PATH "/root/styles"
static void save_styles(struct k_work *work);
class superfect_neopixel {
private:
    int _direction;
    uint8_t _num_of_pixels;
    uint32_t _num_of_index;
    uint32_t _cycle; // LED color cycle (msec)
    uint32_t _diff; // Difference between each LED start time (msec)
    uint32_t _priod; //(ms)
    uint32_t _hz; // LED frequency (Hz)
    std::vector<uint32_t> _sector_size;
    std::vector<uint32_t> _rgbs;

    std::vector<uint32_t> _cur_index;
    std::vector<uint32_t> _cur_rgb_index;
    Adafruit_NeoPixel _strip;
    struct k_mutex _mutex;

    inline uint32_t rgb_idx2index(uint32_t rgb_index) {
        return ((_cycle*_hz)*(rgb_index))/(_rgbs.size()*1000);
    }

    int start_index(uint32_t nTh_pixel) {
        return ((nTh_pixel*_diff*_hz)/1000)%((_cycle*_hz)/1000);
    }

    void calc_sector_size(void) {
        uint32_t nRGB = _rgbs.size();
        _sector_size.clear();
        for(uint8_t n = 0; n < nRGB; n++) {
            _sector_size.push_back(rgb_idx2index(1+n) - rgb_idx2index(n));
        }
    }

    uint32_t index2rgb_idx(uint32_t index) {
        uint32_t nRGB = _rgbs.size();
        for (uint8_t n = 0; n < nRGB; n++) {
            if (index < rgb_idx2index(n+1)) {
                return n;
            }
        }
        //error
        return 0xffffffff;
    }

    uint32_t calc_color(uint32_t nTh_pixel) {
        uint32_t color = 0;
        int cur_index = _cur_index[nTh_pixel];
        int cur_rgb_index = _cur_rgb_index[nTh_pixel];
        int target_rgb_index = (cur_rgb_index + 1)%_rgbs.size();
        int cnt_index = cur_index - rgb_idx2index(cur_rgb_index);

        for (int i = 0; i < 3; i++) {
            int base_color = (_rgbs[cur_rgb_index]>>(8*i))&0xff;
            int target_color = (_rgbs[target_rgb_index]>>(8*i))&0xff;
            int delta = ((target_color - base_color)*(cnt_index))/(int)_sector_size[cur_rgb_index];
            color |= (uint32_t)(delta + base_color) << (8*i);
        }
        return color;
    }

    void show(void) {
        uint32_t color = 0x0f0000;
        for (int n = 0; n < _num_of_pixels; n++) {
            color = calc_color(n);
            _strip.setPixelColor(n, color);
        }
        _strip.show();
    }

    void next(void) {
        for(int n = 0; n < _num_of_pixels; n++) {
            if (_direction < 0) {
                if (_cur_index[n] == 0) {
                    _cur_index[n] = _num_of_index;
                    _cur_rgb_index[n] = _rgbs.size();
                }
            }
            _cur_index[n] += _direction;
            if (_direction < 0) {
                if (_cur_index[n]+1 == rgb_idx2index(_cur_rgb_index[n])) {
                    _cur_rgb_index[n] += _direction;
                }
            } else {
                if (_cur_index[n] == rgb_idx2index(_cur_rgb_index[n]+_direction)) {
                    _cur_rgb_index[n] += _direction;
                }
            }

            if (_cur_index[n] == _num_of_index) {
                _cur_index[n] = 0;
            }

            if (_cur_rgb_index[n] == _rgbs.size()) {
                _cur_rgb_index[n] = 0;
            }
        }
    }

public:
    k_work_delayable _save_style_work;
    uint32_t _selected_style = -1;
    uint32_t _start_save = false;
    std::vector<std::string> _styles;

    superfect_neopixel(const struct device * _port, uint16_t n, int16_t p, neoPixelType t) : _strip(_port, n, p, t) {
        _num_of_pixels = n;
        _hz = 30;
        _priod = 1000/_hz;
    };

    void begin(void) {
        k_mutex_init(&_mutex);
        _strip.begin();
        k_work_init_delayable(&_save_style_work, save_styles);
        int fd = open(STYLE_FILE_PATH, O_RDONLY);
        if (fd < 0) {
            return;
        }

        char c;
        std::string buf;
        while(read(fd, &c, 1) > 0) {
            if (c == 0) {
                _styles.push_back(buf);
                buf.clear();
            }
            buf.push_back(c);
        }
        close(fd);
        _selected_style = _styles.size();
    }

    //cycle(ms), diff(ms)
    void configuration(const std::vector<uint32_t> &rgbs, uint32_t cycle, int diff, uint32_t hz, uint32_t num_of_pixels, uint8_t brightness) {
        k_mutex_lock(&_mutex, K_FOREVER);
        _rgbs = rgbs;
        _cycle = cycle;
        if (diff < 0) {
            _direction = -1;
        } else {
            _direction = 1;
        }
        _diff = _direction*diff;
        _hz = hz;
        _priod = 1000/hz;
        _num_of_pixels = num_of_pixels;
        _num_of_index = (_cycle*_hz)/1000;
        _cur_index.assign(_num_of_pixels, 0);
        _cur_rgb_index.assign(_num_of_pixels, 0);
        for(uint32_t n = 0; n < _num_of_pixels; n++) {
            _cur_index[n] = start_index(n);
            _cur_rgb_index[n] = index2rgb_idx(_cur_index[n]);
        }
        calc_sector_size();
        _strip.updateLength(_num_of_pixels);
        _strip.setBrightness(brightness);
        k_mutex_unlock(&_mutex);
    }

    void run(void) {
        uint32_t run_time;
        while(1) {
            run_time = k_uptime_get_32();
            k_mutex_lock(&_mutex, K_FOREVER);
            show();
            next();
            k_mutex_unlock(&_mutex);
            run_time = k_uptime_get_32() - run_time;
            k_msleep(_priod - run_time);
        }
    }

    void setBrightness(uint8_t b) {
        _strip.setBrightness(b);
    }
};


#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

superfect_neopixel sn(DEVICE_DT_GET(DT_NODELABEL(gpio0)), 15, CONFIG_NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
static void neopixel_task(void) {
    std::vector<uint32_t> default_rgbs = {
        Adafruit_NeoPixel::Color(255, 0, 0),
        Adafruit_NeoPixel::Color(0, 255, 0),
        Adafruit_NeoPixel::Color(0, 0, 255),
        Adafruit_NeoPixel::Color(255, 0, 0),
        Adafruit_NeoPixel::Color(0, 255, 0),
        Adafruit_NeoPixel::Color(0, 0, 255),
        Adafruit_NeoPixel::Color(255, 0, 0),
        Adafruit_NeoPixel::Color(0, 255, 0),
        Adafruit_NeoPixel::Color(0, 0, 255),
    };
    sn.begin();
    sn.configuration(default_rgbs, 10*1000, 10*1000/15, 30, 15, 50);
    if (sn._selected_style != (uint32_t)-1) {
        sn.setBrightness(0);
    }
    sn.run();
}
K_THREAD_DEFINE(neopixel, 4096, neopixel_task, NULL, NULL, NULL,
        K_LOWEST_APPLICATION_THREAD_PRIO-2, 0, 0);


std::vector<std::string> splitString(const std::string& input, char delimiter)
{
    std::vector<std::string> result;
    std::string::size_type start = 0;
    std::string::size_type end = input.find(delimiter);

    while (end != std::string::npos)
    {
        result.push_back(input.substr(start, end - start));
        start = end + 1;
        end = input.find(delimiter, start);
    }

    result.push_back(input.substr(start));

    return result;
}

std::vector<uint32_t> convertToRGBVector(const std::string& rgbString)
{
    std::vector<uint32_t> rgbVector;

    if (rgbString.length() % 9 != 0)
    {
        return rgbVector;
    }

    for (std::size_t i = 0; i < rgbString.length(); i += 9)
    {
        std::string rgbValue = rgbString.substr(i, 9);
        uint32_t value = 0;

        for (int j = 0; j < 3; ++j)
        {
            std::string byteString = rgbValue.substr(j * 3, 3);
            uint32_t byte = std::stoi(byteString);
            value |= (byte << (16 - j * 8));
        }
        rgbVector.push_back(value);
    }

    return rgbVector;
}

static void save_styles(struct k_work *work)
{
    unlink(STYLE_FILE_PATH);
    int fd = open(STYLE_FILE_PATH, O_WRONLY | O_CREAT | O_TRUNC);
    for (uint32_t i = 0; i < sn._styles.size(); i++) {
        write(fd, sn._styles[i].c_str(), sn._styles[i].length()+1); // include null character
    }
    close(fd);
    sn._start_save = false;
    sn.setBrightness(0);
    sn._selected_style = sn._styles.size();
}

static int cmd_save_neopixel_styles(const struct shell *sh, size_t argc, char **argv)
{
    if (argc > 2){
        shell_print(sh, "Invalid arguments");
        return 0;
    }

    if (sn._start_save == false) {
        sn._styles.clear();
    }
    sn._start_save = true;
    sn._styles.push_back(argv[1]);
    k_work_reschedule(&sn._save_style_work, K_MSEC(70));
    return 0;
}

static int cmd_neopixel_set(const struct shell *sh, size_t argc, char **argv)
{
    if (argc > 1){
        shell_print(sh, "Invalid arguments");
        return 0;
    }

    if (sn._styles.size() == 0){
        shell_print(sh, "number of styles is 0");
        return 0;
    }

    sn._selected_style += 1;
    if (sn._styles.size() == sn._selected_style) {
        sn.setBrightness(0);
        return 0;
    } else if (sn._styles.size() < sn._selected_style) {
        sn._selected_style = 0;
    }
    std::vector<std::string> tokens = splitString(sn._styles[sn._selected_style], ',');

    if (tokens.size() != 6) {
        shell_print(sh, "Invalid style");
        return 0;
    }

    uint32_t num_of_rgb_list = std::stoi(tokens[1]);
    std::vector<uint32_t> rgbs = convertToRGBVector(tokens[2]);
    if (num_of_rgb_list != rgbs.size()) {
        shell_print(sh, "Fail convertToRGBVector");
        return 0;
    }

    uint8_t brightness = std::stoi(tokens[3]);
    int cycle = std::stoi(tokens[4]);
    uint32_t option = std::stoi(tokens[5]);

    // Convert brightness
    brightness = 50 + 150*(brightness-1)/9;
    // Convert cycle
    cycle = 11 - cycle;

    // Convert option
    int direction = 0;
    if (option == 0 || option == 3) {
        direction = 0;
    } else if (option == 1) {
        direction = -1;
    } else if (option == 2) {
        direction = 1;
    }

    sn.configuration(rgbs, cycle*1000, (direction*cycle*1000)/15, 30, 15, brightness);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_neopixel,
    SHELL_CMD(set, NULL, "style,nRGB,RGBs,brightness,cycle,option", cmd_save_neopixel_styles),
	SHELL_CMD(rotate_style, NULL, "style number", cmd_neopixel_set),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(neopixel, &sub_neopixel, "Superfect Neopixel", NULL);