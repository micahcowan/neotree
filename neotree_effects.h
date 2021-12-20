// neotree_effects.h
//
// Lighting effect fns to run on the light string

#include <functional>
typedef std::function<color_t(ard_time_t, int)> Effect;

color_t colorRed = pixels.Color(255, 0, 0);
color_t colorBlue = pixels.Color(0, 0, 255);
//color_t colorWhite = pixels.Color(0, 0, 0, 255);
color_t colorWhite = pixels.Color(255, 255, 255);

Effect makeSolid(color_t c) {
    return ([c](ard_time_t, int) {return c;});
}

Effect makeRainbow(
        double      runt    // how long (s) for rainbow to take to traverse entire string of lights
    ,   double      n       // how many rainbows to complete across the string
) {
    return Effect([=](ard_time_t t, int i) {
        double phase = 1. * i * n / NUM;
        phase += fmod(1. * t, runt * 1000.) / (runt * 1000.);
        phase = fmod(phase, 1.0);
        return pixels.ColorHSV(phase * 65536);
    });
}

template<class iter> iter nextIter(iter begin, iter end, iter cur) {
    ++cur;
    if (cur == end) cur = begin;
    return cur;
}

template <class iter>
class EffectCycle {
    private:
        iter          _begin;
        iter          _end;
        iter          _cur;

        ard_time_t          _perCycleMS;
        ard_time_t          _lastChange;
    public:
        EffectCycle(
                iter begin, iter end
            ,   double  perCycle    // How long (s) to spend on each effect before cycling to next
        ) : _begin(begin), _end(end), _perCycleMS(perCycle * 1000) {
            _lastChange = millis();
            _cur = _begin;
        }

        color_t operator ()(ard_time_t t, int i) {
            while (t > _lastChange + _perCycleMS) {
                _lastChange += _perCycleMS;
                _cur = nextIter(_begin, _end, _cur);
            }
            Serial.printf("_cur is %d\n", (_cur - _begin));

            return (*_cur)(t, i);
        }
};

//

Effect rainbot = makeRainbow(3., 2);

const Effect colors[]     = {makeSolid(colorRed), makeSolid(colorWhite), makeSolid(colorBlue)};
const Effect *colorsEnd   = colors + ((sizeof colors)/(sizeof colors[0]));

// XXX: misnamed now
Effect redgreen = EffectCycle<const Effect *>(&colors[0], colorsEnd, 0.25);
