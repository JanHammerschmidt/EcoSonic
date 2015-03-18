__author__ = 'jhammers'

import numpy as np
#import scipy
import matplotlib.pyplot as plt
import itertools
import os
import math, random
from misc import *
from timer import Timer, Profiler, ProfilerExclusive

#import cProfile
#from copy import deepcopy

def show_plot():
    profiler.pause()
    plt.show()
    profiler.resume()

profiler = ProfilerExclusive(True)

def show_scatter(gather_func, eval_func):
    with Profiler.Auto(profiler, 'show_scatter', False):
        x, y, c = [], [], []
        gather_func(x, y, c, eval_func)
        plt.scatter(x, y, color=c)
        show_plot()

def condition2color(cond):
    if cond == "VIS":
        return "blue"
    elif cond == "SLP":
        return "red"
    elif cond == "CNT":
        return "green"
    elif cond == "intro":
        return "yellow"
    else:
        print("WARNING! unknown condition: " + cond)
        return "black"

# def speed_analyze(log):
#     plt.plot(log.pos, log.speed)
#     s = load_json("/Users/jhammers/test.json.zip")
#     s = s["speed"]
#     x = range(len(items))
#     plt.plot([2*x for x in range(0,len(s))], s)
#     plt.plot(s)
#     plt.show()

class EcoSonic_log:
    # QUANT = 0.25

    def __init__(self, file):
        self.file = file

    @property
    def cache_identifier(self): return self.file

    @lazy_property
    def json(self):
        with Profiler.Auto(profiler, 'load_json', False):
            return load_json(self.file)

    # @lazy_cached_property
    # def properties(self):

    @lazy_property
    def items(self): return self.json["items"]

    @lazy_cached_property
    def dt(self): return self.gather("dt")
    @lazy_property
    def speed(self): return self.gather("speed")
    @lazy_property
    def pos(self): return self.gather("position")
    @lazy_property
    def throttle(self): return self.gather("throttle")
    @lazy_cached_property
    def steering(self): return self.gather("steering")
    @lazy_property
    def speed(self): return self.gather("speed")

    @lazy_cached_property
    def condition(self):
        return "intro" if "intro" in self.file else self.json["condition"]

    @lazy_cached_property
    def global_run_counter(self): return self.json["global_run_counter"]
    @lazy_cached_property
    def elapsed_time(self): return self.json["elapsed_time"]
    @lazy_cached_property
    def consumption(self): return self.json["liters_used"]


    @lazy_property
    def t(self): return list(itertools.accumulate(self.dt))

    def gather(self, str):
        with Profiler.Auto(profiler, 'gather', False):
            items = self.items
            return [i[str] for i in self.items]

    def quantize_params(self, x, quant):
        xq_max = math.ceil(x[-1]) + 1 # range of (quantized) x-values
        xq_num = math.ceil(xq_max / quant) # number of elements
        return xq_max, xq_num

    def quantize_x(self, params):
        (xq_max, xq_num) = params
        return np.linspace(0, xq_max, xq_num)

    def quantize(self, data, x, quant, params=None, neutral = 0):
        if params == None:
            params = self.quantize_params(x, quant)
        (xq_max, xq_num) = params
        #x_quant = self.quantize_x(x, params)

        vals, norm = np.zeros(xq_num), np.zeros(xq_num) # values and norm-factor for the corresponding values
        for i,d in enumerate(data):
            t = x[i]
            tqi = t / quant
            tqi0 = math.floor(tqi)
            tqs = (tqi - tqi0)
            vals[tqi0 + 1] += tqs * d
            norm[tqi0 + 1] += tqs
            vals[tqi0] += (1-tqs) * d
            norm[tqi0] += (1-tqs)

        for i in range(len(vals)):
            if norm[i] > 0:
                vals[i] /= norm[i]
            else:
                vals[i] = neutral

        return vals, params

    def scatter_consumption_vs_run(self, x, y, c):
        x.append(self.global_run_counter)
        y.append(self.consumption)
        c.append(condition2color(self.condition))

    def scatter_consumption_vs_lap_time(self, x, y, c):
        x.append(self.elapsed_time)
        y.append(self.consumption)
        c.append(condition2color(self.condition))

    def scatter_steering_integral_vs_run(self, x, y, c):
        x.append(self.global_run_counter)
        y.append(self.steering_integral)
        c.append(condition2color(self.condition))

    def test(self):
        #tq, posq = self.quantize(self.pos, self.t, 0.25)
        tq, posq = self.quantize(self.steering, self.t, 0.25)
        f, (ax1, ax2) = plt.subplots(2)
        print(len(self.t))
        print(len(tq))
        ax1.plot(self.t, self.steering)
        ax2.plot(tq, posq)
        plt.show()

    def show_steering(self):
        plt.plot(self.t, self.steering, color=condition2color(self.condition))
        show_plot()

    @lazy_property
    def steering_integral(self):
        with Profiler.Auto(profiler, 'steering_integral', False):
            st, dt = self.steering, self.dt
            # with Profiler.Auto(profiler, 'convert', False):
            #     dtn, stn = np.array(dt), np.array(st)
            si = 0
            for i,s in enumerate(st):
                si += dt[i] * abs(s)
            #si = sum([i[0]*abs(i[1]) for i in zip(dt,st)])
            #si = sum(np.array(dt) * abs(np.array(st)))
            return si # / self.elapsed_time



class EcoSonic_VP:
    def __init__(self, id, base_folder='/Users/jhammers/Dropbox/EcoSonic/', keep_intro = False):
        self.logs = []
        folder = base_folder + "VP" + str(id) + "/"
        for file in os.listdir(folder):
            if file.endswith("json.zip") and (keep_intro or "intro" not in file):
                #print(file)
                self.logs.append(EcoSonic_log(folder + file))

    def load_json(self):
        for log in self.logs:
            log.json

    def scatter_gather(self, x, y, c, func):
        for log in self.logs:
            func(log, x, y, c)

    def show_steering(self):
        quant = 1
        quant_params = []
        for i, log in enumerate(self.logs):
            quant_params.append(log.quantize_params(log.pos, quant))
        quant_params = max(quant_params)

        steering = {'VIS': [], 'SLP': [], 'CNT': []}
        for log in self.logs:
            steering[log.condition].append(log.quantize(log.steering, log.pos, quant, quant_params)[0])

        for cond, sts in steering.items():
            for st in sts[1:]:
                sts[0] += st
            sts[0] /= len(sts)
            print(cond, ':', sum(abs(sts[0]))) # steering_integral

        x_quant = self.logs[0].quantize_x(quant_params)
        f, ax = plt.subplots(3)
        for i,(key,val) in enumerate(steering.items()):
            ax[i].plot(x_quant, val[0], color=condition2color(key))

        plt.show()

class EcoSonic_All:

    def __init__(self, folder = '/Users/jhammers/Dropbox/EcoSonic/', keep_intros = False):
        self.vps = []
        for f in os.listdir(folder):
            if f.startswith('VP'):
                self.vps.append(EcoSonic_VP(int(f[2:]), folder, keep_intros))

    def load_json(self):
        for vp in self.vps:
            vp.load_json()

    def scatter_gather(self, x, y, c, func):
        for i,vp in enumerate(self.vps):
            vp.scatter_gather(x, y, c, func)

    def all_logs(self):
        logs = []
        for vp in self.vps:
            logs.extend(vp.logs)
        return logs

###################################################
profiler.start('load cache')
lazy_cached_property.load_cache()
profiler.switch_to('main')

all = EcoSonic_All()

logs = all.all_logs()
while True:
    log = random.choice(logs)
    print(log.condition, ':', log.steering_integral)
    log.show_steering()

# while True:
#     vp = random.choice(all.vps)
#     vp.show_steering()

#with Timer('calc'):
    #show_scatter(all.scatter_gather, EcoSonic_log.scatter_steering_integral_vs_run)
    # show_scatter(all.scatter_gather, EcoSonic_log.scatter_consumption_vs_lap_time)

# show_scatter(all.scatter_gather, EcoSonic_log.scatter_consumption_vs_run)

#plt.show()
profiler.switch_to('save cache')
lazy_cached_property.save_cache()
profiler.stop()
profiler.output()
exit()

vp = EcoSonic_VP(1003)
log = vp.logs[0]
#log.test()
vp.show_steering()
exit()
# k1 = vp.logs[0].json
# k2 = vp.logs[1].json

log = vp.logs[0]
items = log.items
pos = log.pos
log.test()

#cProfile.run('show_scatter(all.scatter_gather, EcoSonic_log.scatter_steering_integral_vs_run)')

#vp.show_steering()
#vp.show_consumption_vs_time()
#vp.show_consumption()

#log_analyze_all("/Users/jhammers/Dropbox/VP1001/")
#speed_analyze()