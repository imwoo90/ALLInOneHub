// #pragma once

#include <Adafruit_NeoPixel.h>
#include <vector>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(SN);

class superfect_neopixel {
private:
    int _direction;
    uint8_t _begun;
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
    superfect_neopixel(const struct device * _port, uint16_t n, int16_t p, neoPixelType t) : _strip(_port, n, p, t) {
        _begun = false;
        _num_of_pixels = n;
        _hz = 30;
        _priod = 1000/_hz;
    };

    void begin(void) {
        k_mutex_init(&_mutex);
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
        _cur_index.clear();
        _cur_rgb_index.clear();
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

static void neopixel_task(void) {
    superfect_neopixel sn(DEVICE_DT_GET(DT_NODELABEL(gpio0)), 15, CONFIG_NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
    std::vector<uint32_t> rgbs = {
        Adafruit_NeoPixel::Color(255, 0, 0),
        Adafruit_NeoPixel::Color(0, 255, 0),
        Adafruit_NeoPixel::Color(0, 0, 255),
    };
    sn.begin();
    sn.configuration(rgbs, 10*1000, 10*1000/15, 30, 15, 50);
    sn.run();
}
K_THREAD_DEFINE(neopixel, 4096, neopixel_task, NULL, NULL, NULL,
        K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);