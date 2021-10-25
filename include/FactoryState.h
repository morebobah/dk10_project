
#include "DK10.h"
#include "MixerState.h"
#include "PressState.h"

class FactoryState{
    public:
        DK10State dk10();
        MixerState mixerState();
        PressState pressState();
};

