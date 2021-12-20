// neotree_effects.h
//
// Lighting effect fns to run on the light string

#include <functional>
#include <list>
typedef std::function<color_t(ard_time_t, int)> Effect;

void doPixels(Effect &effect, ard_time_t t = millis()) {
    for (unsigned int i = 0; i != NUM; ++i) {
        pixels.setPixelColor(i, effect(t, i));
    }
    pixels.show();
}

color_t colorRed    = pixels.Color(255, 0, 0);
color_t colorOrange = pixels.Color(255, 128, 0);
color_t colorYellow = pixels.Color(255, 255, 0);
color_t colorGreen  = pixels.Color(0, 255, 0);
color_t colorCyan   = pixels.Color(0, 255, 255);
color_t colorBlue   = pixels.Color(0, 0, 255);
color_t colorIndigo = pixels.Color(128, 0, 255);
color_t colorViolet = pixels.Color(255, 0, 255);

color_t colorBlack  = pixels.Color(0, 0, 0);
//color_t colorWhite = pixels.Color(0, 0, 0, 255);
color_t colorWhite = pixels.Color(255, 255, 255);

Effect makeSolid(color_t c) {
    return ([c](ard_time_t, int) {return c;});
}

Effect  solidRed    = makeSolid(colorRed);
Effect  solidOrange = makeSolid(colorOrange);
Effect  solidYellow = makeSolid(colorYellow);
Effect  solidGreen  = makeSolid(colorGreen);
Effect  solidCyan   = makeSolid(colorCyan);
Effect  solidBlue   = makeSolid(colorBlue);
Effect  solidIndigo = makeSolid(colorIndigo);
Effect  solidViolet = makeSolid(colorViolet);
Effect  solidWhite  = makeSolid(colorWhite);
Effect  solidBlack  = makeSolid(colorBlack);

std::list<Effect> allColors = {
    solidRed, solidGreen, solidIndigo,
    solidOrange, solidCyan, solidViolet,
    solidYellow, solidBlue, solidWhite
};

//

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

template <class orig_iter_t>
class cycling_iterator {
    typedef orig_iter_t iterator;
    typedef typename std::iterator_traits<orig_iter_t>::value_type   value_type;
    typedef typename std::iterator_traits<orig_iter_t>::reference    reference;

    orig_iter_t _begin;
    orig_iter_t _end;
    orig_iter_t _cur;

    public:
       cycling_iterator(orig_iter_t begin, orig_iter_t end) : _begin(begin), _end(end), _cur(begin) { }

       cycling_iterator &operator ++() {
           ++_cur;
           if (_cur == _end) _cur = _begin;
           return *this;
       }

       reference operator*() {
           return *_cur;
       }

       reference operator->() {
           return *_cur;
       }
};

typedef cycling_iterator<std::list<Effect>::iterator>   cycling_effect_list_iterator;
cycling_iterator<std::list<Effect>::iterator> makeColorCycler(void) {
    return cycling_iterator<std::list<Effect>::iterator>(allColors.begin(), allColors.end());
}

template <class iter>
class EffectCycle {
    private:
        cycling_iterator<iter>  _cur;

        ard_time_t          _perCycleMS;
        ard_time_t          _lastChange;
    public:
        EffectCycle(
                iter begin, iter end
            ,   double  perCycle    // How long (s) to spend on each effect before cycling to next
        ) : _cur(begin, end), _perCycleMS(perCycle * 1000) {
            _lastChange = millis();
        }

        color_t operator ()(ard_time_t t, int i) {
            while (t > _lastChange + _perCycleMS) {
                _lastChange += _perCycleMS;
                ++_cur;
            }

            return (*_cur)(t, i);
        }
};

template <class EffectIter>
class WindingEffect {
    private:
        const ard_time_t  _windTime;  // How long (ms) does it take to wind from the start of the string to the end?
        const ard_time_t  _waitTime;  // How long (ms) to wait after winding is completed, to start the next
        ard_time_t  _transStart;
        ard_time_t  _timeMark;  // detects same time given successively during iteration, so we avoid recalculating threshold
        int threshold = 0;

        EffectIter  _effectIter;
        Effect one = solidBlack;
        Effect two;
    public:
        WindingEffect(ard_time_t windTime, ard_time_t waitTime, EffectIter effectIter) : 
                _windTime(windTime), _waitTime(waitTime), _transStart(millis()), _effectIter(effectIter) {
            two = *_effectIter;
            ++_effectIter;
        }
        color_t operator()(ard_time_t t, int i) {
            if (t > _transStart + _windTime + _waitTime) {
                // Time to pop the next effect
                _transStart = millis();
                one = two;
                two = *_effectIter;
                ++_effectIter;
                return one(t, i);
            }
            else if (t > _transStart + _windTime) {
                // We're in wait time
                return two(t, i);
            }
            else {
                if (_timeMark != t) {
                    _timeMark = t;
                    // get the fraction of the way into the transition
                    // we are, and set threshold to that
                    double frac = 1. * (t - _transStart) / _windTime;
                    threshold = NUM * frac;
                }
                return  i > threshold? one(t, i) : two(t, i);
            }
        }
};

template <class EffectIter>
class PopInEffect {
    private:
        EffectIter      _iter;
        const unsigned int    _numStrips;
        ard_time_t      _perDotTime;
        ard_time_t      _completeDotsTime;  // How long it takes to get dots for one effect across the whole strip
        ard_time_t      _effectSpacingTime; // How long of a pause between the start of one effect, and the start of another
        ard_time_t      _fullTime;          // How long it takes to phase in all the effects (total time is twice this)
        ard_time_t      _startTime = 0;     // when we started for the current set of effects.

        std::vector<Effect>   _effects;   // A vector of N effects, where N = _numStrips

        void resetEffects() {
            _effects.clear();
            for (unsigned int i=0; i != _numStrips; ++i) {
                _effects.push_back(*_iter);
                ++_iter;
            }
        }

    public:
        PopInEffect(
            EffectIter iter,       // source of effects (usually colors)
            unsigned int numStrips, // how many effects to show at once (with starts spaced out evenly)
            double dotsPerSec       // how quickly to make new dots appear, in dots-per-second
        ) : _iter(iter), _numStrips(numStrips) {
            _perDotTime = 1000 / dotsPerSec;
            _completeDotsTime = (NUM / _numStrips) * _perDotTime;
            _effectSpacingTime = _completeDotsTime / _numStrips;
            _fullTime = (_completeDotsTime + (_effectSpacingTime * (_numStrips-1)));

            _effects.reserve(_numStrips);
            resetEffects();
        }

        color_t operator ()(ard_time_t t, int i) {
            if (t > _startTime + 2 * _fullTime) {
                _startTime = t;
                resetEffects();
                return colorBlack;
            }

            // Count t as ms since _startTime.
            ard_time_t torig = t;
            t -= _startTime;
            // Which effect does this pixel belong to?
            unsigned int effNum = i % _numStrips;
            // XXX: the following calculations could be cached across a
            // strip fill...
            bool fadingIn = t < _fullTime;
            // if we're fading out, count t from start of fade-out
            if (!fadingIn) t -= _fullTime;
            
            bool lit = false;
            ard_time_t effStart = effNum * _effectSpacingTime;
            if (t > effStart + _completeDotsTime) {
                lit = true;
            }
            else if (t > effStart) {
                // How far into the strip?
                double frac = (t - effStart) / _completeDotsTime;
                if (1. * i / NUM >= frac) {
                    lit = true;
                }
            }
            // Reverse the sense of "lit" if we're fading out
            lit ^= !fadingIn;

            if (lit) {
                Effect eff = _effects[effNum];
                return eff(torig, i);
            }
            else {
                return colorBlack;
            }
        }
};

//

Effect rainbot = makeRainbow(3., 2);

std::list<Effect> colors = {solidRed, solidGreen, solidWhite};

// XXX: misnamed now
Effect redgreen = EffectCycle<std::list<Effect>::iterator>(colors.begin(), colors.end(), 0.25);

std::list<Effect> allEffects = {
    PopInEffect< cycling_effect_list_iterator >(makeColorCycler(), 3, 4.5),
    WindingEffect< cycling_effect_list_iterator >(4000, 1000, makeColorCycler()),
    rainbot
};

cycling_iterator<std::list<Effect>::iterator> currentEffect(allEffects.begin(), allEffects.end());
