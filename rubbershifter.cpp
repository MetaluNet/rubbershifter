/***************************************************************************
 * File: rubbershifter.cpp
 * Auth: Antoine Rousseau
 * Creation date: December 2022
 *
 * Description: an implementation of RubberBand pitch/stretch library (https://breakfastquay.com/rubberband/)
 *
 * Copyright (C) 2022 by Antoine Rousseau [antoine@metalu.net]
 * BSD Simplified License, see the file "LICENSE" in this distribution.
 *
 * Also read RubberBand's license, which is dual-licensed GPL-commercial (https://breakfastquay.com/rubberband/license.html)
 *
 ****************************************************************************/

#include "m_pd.h"
#include "RubberBandStretcher.h"
#include <map>

using namespace RubberBand;
using namespace std;

static t_class *rubbershifter_class;
static map<t_symbol*, map<t_symbol*, RubberBandStretcher::Option>> RBOptions;

typedef struct _rubbershifter
{
    t_object x_obj;
    t_outlet *outlet;
    float pitch;   // 1.0 = no pitch shift
    int bypass;
    int available; // samples available last DSP pass
    map<t_symbol*, t_symbol*> *options;
    RubberBand::RubberBandStretcher *stretcher;
} t_rubbershifter;

static void *rubbershifter_new(t_symbol *s, int argc, t_atom *argv)
{
    t_rubbershifter *x = (t_rubbershifter *)pd_new(rubbershifter_class);
    outlet_new(&x->x_obj, gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
    x->outlet = outlet_new(&x->x_obj, gensym("anything"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);

    x->stretcher = NULL;
    x->options = new map<t_symbol*, t_symbol*>;
    x->pitch = 1.0;
    x->bypass = 0;
    return (x);
}

static void rubbershifter_delete(t_rubbershifter *x)
{
    delete x->stretcher;
    delete x->options;
}

static RubberBandStretcher::Options rubbershifter_processOptions(t_rubbershifter *x)
{
    t_atom at;
    RubberBandStretcher::Options options = (RubberBandStretcher::Option)0;
    for(auto &o : *x->options) {
        options |= RBOptions[o.first][o.second];
    }
    return options;
}

static void rubbershifter_create_stretecher(t_rubbershifter *x)
{
    if(x->stretcher) delete x->stretcher;
    x->stretcher = new RubberBandStretcher(sys_getsr(), 2,
        RubberBandStretcher::OptionProcessRealTime | rubbershifter_processOptions(x));
}

static void rubbershifter_delete_stretecher(t_rubbershifter *x)
{
    if(x->stretcher) {
        delete x->stretcher;
        x->stretcher = NULL;
    }
}

static t_int *rubbershifter_perform(t_int *w)
{
    t_sample *in1 = (t_sample *)(w[1]);
    t_sample *in2 = (t_sample *)(w[2]);
    t_sample *out1 = (t_sample *)(w[3]);
    t_sample *out2 = (t_sample *)(w[4]);
    int n = (int)(w[5]);
    t_rubbershifter *x = (t_rubbershifter *)(w[6]);

    if(!x->bypass) {

        if(x->stretcher == NULL) rubbershifter_create_stretecher(x);
        x->stretcher->setPitchScale(x->pitch);
        int available = x->stretcher->available();
        /* if too many 'available' samples in the stretcher buffer, decrease the 
           timeRatio so the buffer will empty faster than it fills :
        */
        float timeRatio = 1.0 - (max(available - (int)x->stretcher->getStartDelay(), 0) / 20000.0);
        x->stretcher->setTimeRatio(timeRatio);

        const float * inbufs[2];
        inbufs[0] = in1;
        inbufs[1] = in2;
        x->stretcher->process(inbufs, n, false);
        available = x->stretcher->available();

        if(available >  n) {
            float * outbufs[2];
            outbufs[0] = out1;
            outbufs[1] = out2;
            x->stretcher->retrieve(outbufs, n);
            available -= n;
        }
        else {
            //post("z!");
            while  (n--)
            {
                *out1++ = *out2++ = 0.0;
            }
        }
        x->available = available;
    } else {
        t_sample s2;
        while  (n--)
        {
            s2 = *in2++; // read in2 before writing out1, that might overwrite it.
            *out1++ = *in1++;
            *out2++ = s2;
        }
    }
    return (w+7);
}

static void rubbershifter_dsp(t_rubbershifter *x, t_signal **sp)
{
    int n = sp[0]->s_n;
    dsp_add(rubbershifter_perform, 6, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, n, x);
}

static void rubbershifter_bypass(t_rubbershifter *x, t_floatarg f)
{
    x->bypass = f;
}

static void rubbershifter_pitch(t_rubbershifter *x, t_floatarg f)
{
    x->pitch = f;
}

static void rubbershifter_print(t_rubbershifter *x)
{
    logpost(x, PD_NORMAL, "rubbershifter current options:");
    for(auto &o : *x->options) {
        logpost(x, PD_NORMAL, "'%s' : '%s'", o.first->s_name, o.second->s_name);
    }
    if(x->stretcher) logpost(x, PD_NORMAL, "latency : %ld", x->stretcher->getStartDelay());
}

static void rubbershifter_get(t_rubbershifter *x)
{
    t_atom at;
    for(auto &o : *x->options) {
        SETSYMBOL(&at, o.second);
        outlet_anything(x->outlet, o.first, 1, &at);
    }
    if(x->stretcher) {
        SETFLOAT(&at, x->stretcher->getStartDelay());
        outlet_anything(x->outlet, gensym("latency"), 1, &at);
    }
    SETFLOAT(&at, x->available);
    outlet_anything(x->outlet, gensym("available"), 1, &at);
}

static void rubbershifter_option(t_rubbershifter *x, t_symbol *option, int argc, t_atom *argv)
{
    if(argc == 0 || argv->a_type != A_SYMBOL) {
        pd_error(x, "rubbershifter: bad option format");
        return;
    }
    t_symbol *value = atom_getsymbol(argv);
    if(RBOptions[option].count(value)) {
        (*x->options)[option] = value;
        rubbershifter_delete_stretecher(x);
    } else pd_error(x, "rubbershifter: bad value '%s' for option '%s'", value->s_name, option->s_name);
    rubbershifter_processOptions(x);
}

extern "C" {
    void rubbershifter_tilde_setup(void)
    {
        rubbershifter_class = class_new(gensym("rubbershifter~"), (t_newmethod)rubbershifter_new,
            (t_method)rubbershifter_delete, sizeof(t_rubbershifter), 0, A_GIMME, 0);

        class_addmethod(rubbershifter_class, nullfn, gensym("signal"), A_NULL);

        class_addmethod(rubbershifter_class, (t_method)rubbershifter_dsp, gensym("dsp"), A_NULL);
        class_addmethod(rubbershifter_class, (t_method)rubbershifter_bypass, gensym("bypass"), A_FLOAT, A_NULL);
        class_addmethod(rubbershifter_class, (t_method)rubbershifter_pitch, gensym("pitch"), A_FLOAT, A_NULL);
        class_addmethod(rubbershifter_class, (t_method)rubbershifter_print, gensym("print"), A_NULL);
        class_addmethod(rubbershifter_class, (t_method)rubbershifter_get, gensym("get"), A_NULL);

        RBOptions[gensym("engine")][gensym("faster")] =          RubberBandStretcher::OptionEngineFaster;
        RBOptions[gensym("engine")][gensym("finer")] =           RubberBandStretcher::OptionEngineFiner;

        RBOptions[gensym("transients")][gensym("crisp")] =       RubberBandStretcher::OptionTransientsCrisp;
        RBOptions[gensym("transients")][gensym("mixed")] =       RubberBandStretcher::OptionTransientsMixed;
        RBOptions[gensym("transients")][gensym("smooth")] =      RubberBandStretcher::OptionTransientsSmooth;

        RBOptions[gensym("detector")][gensym("compound")] =      RubberBandStretcher::OptionDetectorCompound;
        RBOptions[gensym("detector")][gensym("percussive")] =    RubberBandStretcher::OptionDetectorPercussive;
        RBOptions[gensym("detector")][gensym("soft")] =          RubberBandStretcher::OptionDetectorSoft;

        RBOptions[gensym("phase")][gensym("laminar")] =          RubberBandStretcher::OptionPhaseLaminar;
        RBOptions[gensym("phase")][gensym("independent")] =      RubberBandStretcher::OptionPhaseIndependent;

        RBOptions[gensym("window")][gensym("standard")] =        RubberBandStretcher::OptionWindowStandard;
        RBOptions[gensym("window")][gensym("short")] =           RubberBandStretcher::OptionWindowShort;
        RBOptions[gensym("window")][gensym("long")] =            RubberBandStretcher::OptionWindowLong;

        RBOptions[gensym("smoothing")][gensym("off")] =          RubberBandStretcher::OptionSmoothingOff;
        RBOptions[gensym("smoothing")][gensym("on")] =           RubberBandStretcher::OptionSmoothingOn;

        RBOptions[gensym("formant")][gensym("shifted")] =        RubberBandStretcher::OptionFormantShifted;
        RBOptions[gensym("formant")][gensym("preserved")] =      RubberBandStretcher::OptionFormantPreserved;

        RBOptions[gensym("priority")][gensym("speed")] =         RubberBandStretcher::OptionPitchHighSpeed;
        RBOptions[gensym("priority")][gensym("quality")] =       RubberBandStretcher::OptionPitchHighQuality;
        RBOptions[gensym("priority")][gensym("consistency")] =   RubberBandStretcher::OptionPitchHighConsistency;

        RBOptions[gensym("channel")][gensym("apart")] =          RubberBandStretcher::OptionChannelsApart;
        RBOptions[gensym("channel")][gensym("together")] =       RubberBandStretcher::OptionChannelsTogether;

        for(auto &o : RBOptions) {
            t_symbol *option = o.first;
            //post("rubbershifter_tilde_setup: adding method '%s'", option->s_name);
            class_addmethod(rubbershifter_class, (t_method)rubbershifter_option, option, A_GIMME, A_NULL);
        }

    }
}
