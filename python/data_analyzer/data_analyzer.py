__author__ = 'jhammers'

print('> start')
from timer import Profiler, ProfilerExclusive
profiler = ProfilerExclusive(True)

profiler.start('import', True)
from misc import *
import numpy as np
import matplotlib.pyplot as plt
import math, random
from collections import defaultdict, OrderedDict
from itertools import permutations, accumulate
from scipy.stats import ttest_ind

lazy_cached_property2.profiler = profiler
profiler.switch_to('parsing', False)

def show_plot(show = True):
    if not show:
        return
    profiler.pause()
    plt.show()
    profiler.resume()


def scatter_gather(items, x_val, y_val, c_val='condition_color'):
    x, y, c = [], [], []
    for i in items:
        x.append(get_class_value(i, x_val))
        y.append(get_class_value(i, y_val))
        c.append(get_class_value(i, c_val))
    return x, y, c

def show_scatter(items, x_val, y_val, c_val='condition_color', ylabel = None, xlabel = None, show = None, fig = None):
    with profiler.auto('show_scatter', False):
        x, y, c = scatter_gather(items, x_val, y_val, c_val)
        ylabel = y_val if ylabel is None else ylabel
        xlabel = x_val if xlabel is None else xlabel # TODO!
        show = fig is None if show is None else show
        plot = plt if fig is None else fig
        if ylabel != '':
            if fig is None:
                plot.figure().canvas.set_window_title(ylabel)
                plot.ylabel(ylabel)
                plot.xlabel(xlabel)
            else:
                fig.set_ylabel(ylabel)
                fig.set_xlabel(xlabel)
        plot.scatter(x, y, color=c)
        show_plot(show)
        return x,y

# def show_scatter_old(gather_func, eval_func, show = True):
#     with Profiler.Auto(profiler, 'show_scatter', False):
#         x, y, c = [], [], []
#         gather_func(x, y, c, eval_func)
#         plt.scatter(x, y, color=c)
#         if show:
#             show_plot()

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

profiler.switch_to('parsing:EcoSonic_log', False)

class EcoSonic_log:
    # QUANT = 0.25
    max_steering_reaction_time = 5
    default_window_size = (1717,1089)

    def __init__(self, file):
        self.file = file

    @property
    def cache_identifier(self): return self.file

    @lazy_cached_property2
    def json(self):
        with profiler.auto('load_json', False):
            return load_json(self.file)

    # @lazy_cached_property
    # def properties(self):

    ### identifying information of this run
    @lazy_cached_property2
    def condition(self):
        return "intro" if "intro" in self.file else self.json["condition"]
    @lazy_cached_property2
    def global_run_counter(self): return self.json["global_run_counter"]
    @lazy_cached_property2
    def run(self): return self.json["run"]
    @lazy_cached_property2
    def vp_id(self): return self.json["vp_id"]
    @lazy_property
    def condition_color(self): return condition2color(self.condition)
    @lazy_cached_property2
    def window_size(self):
        if 'window_size_width' in self.json:
            return self.json['window_size_width'], self.json['window_size_height']
        else:
            return EcoSonic_log.default_window_size
    @lazy_property
    def valid_eye_tracking_data(self):
        points = self.eye_tracking_point
        ratio = sum(p == (0,0) for p in points) / len(points)
        return ratio < 0.2
    @lazy_property
    def valid_eye_tracking_points(self):
        return [p for p in self.eye_tracking_point if p != (0,0)]

    ### detailed information over the course of the run
    @lazy_property
    def items(self): return self.json["items"]
    @lazy_cached_property2
    def dt(self): return self.gather("dt")
    @lazy_cached_property2
    def speed(self): return self.gather("speed")
    @lazy_cached_property2
    def pos(self): return self.gather("position")
    @lazy_cached_property2
    def throttle(self): return self.gather("throttle")
    @lazy_cached_property2
    def gear(self): return self.gather("gear")
    @lazy_cached_property2
    def steering(self): return self.gather("steering")
    @lazy_cached_property2
    def user_steering(self): return self.gather("user_steering")
    @lazy_cached_property2
    def scripted_steering(self): return self.gather("scripted_steering")
    @lazy_cached_property2
    def eye_tracking_point(self): return list(zip(self.gather("eye_tracker_point_x"),
                                                  self.gather("eye_tracker_point_y")))
    @lazy_property
    def t(self): return list(accumulate(self.dt))

    ### events for this run ###
    @lazy_cached_property2
    def events(self): return self.json["events"]
    @lazy_property
    def num_events(self): return len(self.events)
    def gather_events(self):
        types = [('TooSlow', 'too_slow'), ('Speeding', 'speeding'), ('StopSign', 'stop_sign'), ('TrafficLight', 'traffic_light')]
        for t in types:
            setattr(self, 'num_' + t[1], len([e for e in self.events if e['type'] == t[0]]))

    ### qualitative information of this run (single-item!) ###
    @lazy_cached_property2
    def elapsed_time(self): return self.json["elapsed_time"]
    @lazy_cached_property2
    def consumption(self): return self.json["liters_used"]
    @lazy_property
    def eye_tracking_simple_attention_ratio(self):
        if not self.valid_eye_tracking_data:
            return None
        points = self.valid_eye_tracking_points
        a = sum(p[1] < 544 for p in points)
        return a / (len(points)) # - a

    @lazy_property
    def normalized_consumption(self):
        v = np.array([self.elapsed_time, self.consumption])
        if not (hasattr(EcoSonic_log, 'consumption_normalize_origin') and hasattr(EcoSonic_log, 'consumption_normalize_vector')):
            EcoSonic_log.all_logs.calc_consumption_normalize_parameters()
        return np.dot(v - EcoSonic_log.consumption_normalize_origin, EcoSonic_log.consumption_normalize_vector)

    @property
    def avg_steering_reaction_time(self):
        return np.mean(self.steering_reaction_times)

    @lazy_property
    def steering_integral(self):
        with profiler.auto('steering_integral', False):
            st, dt = self.steering, self.dt
            # with Profiler.Auto(profiler, 'convert', False):
            #     dtn, stn = np.array(dt), np.array(st)
            si = 0
            for i,s in enumerate(st):
                si += dt[i] * abs(s)
            #si = sum([i[0]*abs(i[1]) for i in zip(dt,st)])
            #si = sum(np.array(dt) * abs(np.array(st)))
            return si # / self.elapsed_time

    ### technical routines ###
    def sanity_check(self):
        len_t = len(self.t)
        assert self.window_size == EcoSonic_log.default_window_size
        for data in [self.dt, self.steering, self.scripted_steering]:
            assert len(data) == len_t

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
        f, ax = plt.subplots(3)
        ax[0].plot(self.t, self.steering)
        ax[1].plot(self.t, self.user_steering)
        ax[2].plot(self.t, self.scripted_steering)
        action = self.steering_action
        x,y,xr,yr = [], [], [], []
        response = self.steering_response
        print(self.steering_reaction_times)
        for i,a in enumerate(action):
            x.append([self.t[j] for j in a])
            y.append([0.03 if i == 1 else -0.03] * len(a))
            xr.append([self.t[j] for j in response[i]])
            yr.append([0 if i == 1 else 0] * len(a))

        for i in [1,2]:
            for j in range(2):
                ax[i].scatter(x[j], y[j])
                ax[i].scatter(xr[j], yr[j])
            plt.xlim([0,140])
        show_plot()

    @lazy_property
    def steering_action(self):
        assert len(self.scripted_steering) == len(self.steering)
        assert len(self.t) == len(self.steering)

        def find_action(test_func):
            action, i = [], 0
            while i < len(self.t):
                if test_func(self.scripted_steering[i], self.steering[i]):
                    action.append(i)
                    t = self.t[i]
                    while i < len(self.t) and self.t[i] - t < 5: #skip 5 seconds
                        i += 1
                else:
                    i += 1
            return action
        left_action = find_action(lambda scripted, steering: scripted < 0 and steering < -0.3)
        right_action = find_action(lambda scripted, steering: scripted > 0 and steering > 0.3)
        return left_action, right_action

    @property
    def steering_response(self):
        left, right = self.steering_action
        def response(action, func):
            ret = [0] * len(action)
            for idx,a in enumerate(action):
                i, t = a, self.t[a]
                while i < len(self.t) and self.t[i] - t < EcoSonic_log.max_steering_reaction_time: # maximum of 5 seconds reaction time!
                    if func(self.user_steering[i]):
                        break
                    i += 1
                assert i < len(self.t)
                ret[idx] = i
            return ret

        left_response = response(left, lambda x: x > 0.0005)
        right_response = response(right, lambda x: x < -0.0005)
        return left_response, right_response, left, right

    @property
    def steering_reaction_times(self):
        left_response, right_response, left, right = self.steering_response
        r = [i for i in zip(left, left_response)] + [i for i in zip(right, right_response)]
        r.sort()
        return [self.t[response] - self.t[action] for (action, response) in r]


    def gather(self, str):
        with profiler.auto('gather', False):
            return [i[str] for i in self.items]

    def quantize_params(self, x, quant):
        xq_max = math.ceil(x[-1]) + 1 # range of (quantized) x-values
        xq_num = math.ceil(xq_max / quant) # number of elements
        return xq_max, xq_num

    def quantize_x(self, params):
        (xq_max, xq_num) = params
        return np.linspace(0, xq_max, xq_num)

    def quantize(self, data, x, quant, params=None, neutral = 0):
        with profiler.auto('quantize', False):
            if params == None:
                params = self.quantize_params(x, quant)
            (xq_max, xq_num) = params
            #x_quant = self.quantize_x(x, params)

            vals, norm = np.zeros(xq_num), np.zeros(xq_num) # values and norm-factor for the corresponding values
            with Profiler.auto('quantize:main_loop', False):
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

profiler.switch_to('parsing:EcoSonic_VP', False)

class EcoSonic_VP:
    def __init__(self, vp_id, base_folder='/Users/jhammers/Dropbox/EcoSonic/', keep_intro = False):
        self.logs = []
        folder = base_folder + "VP" + str(vp_id) + "/"
        for file in os.listdir(folder):
            if file.endswith("json.zip") and (keep_intro or "intro" not in file):
                #print(file)
                self.logs.append(EcoSonic_log(folder + file))
        if False: # !!
            self.logs = [log for log in self.logs if log.run <= 4]
            self.logs.sort(key=lambda x:x.global_run_counter)
            for i, log in enumerate(self.logs):
                log.global_run_counter = i+1
            #print([l.global_run_counter for l in self.logs])
        self.vp_id, self.folder = vp_id, folder

    def load_json(self):
        for log in self.logs:
            log.json

    def sanity_check(self):
        for log in self.logs:
            assert int(log.vp_id) == self.vp_id or self.vp_id == 6001
            assert EcoSonic_VP.condition_order_from_vp_id(self.vp_id) == self.condition_order()
            log.sanity_check()

    def show_scatter_vs_run_counter(self, y_val, **kwargs):
        show_scatter(self.logs, 'global_run_counter', y_val, 'condition_color', **kwargs)

    def scatter_gather(self, x, y, c, func):
        for log in self.logs:
            func(log, x, y, c)

    def condition_order_from_vp_id(vp_id):
        return {1: ['VIS', 'SLP', 'CNT'],
                2: ['SLP', 'VIS', 'CNT'],
                3: ['CNT', 'SLP', 'VIS'],
                4: ['VIS', 'CNT', 'SLP'],
                5: ['SLP', 'CNT', 'VIS'],
                6: ['CNT', 'VIS', 'SLP']}[vp_id % 1000]

    def condition_order(self):
        conditions = [log.condition for log in sorted(self.logs, key = lambda x: x.global_run_counter)]
        return list(OrderedDict.fromkeys(conditions))
        s = set(t)

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

profiler.switch_to('parsing:EcoSonic_All', False)

class EcoSonic_All:

    def __init__(self, folder = '/Users/jhammers/Dropbox/EcoSonic/', keep_intros = False, jens = None):
        self.vps, self.jens = [], jens
        for f in os.listdir(folder):
            if len(f) == 6 and f.startswith('VP'):
                self.vps.append(EcoSonic_VP(int(f[2:]), folder, keep_intros))
        assert not hasattr(EcoSonic_log, 'all_logs')
        EcoSonic_log.all_logs = self

    def sanity_check(self):
        for vp in self.vps:
            vp.sanity_check()

    # def load_json(self):
    #     for vp in self.vps:
    #         vp.load_json()

    def show_scatter_all_vs_run(self, y_val):
        show_scatter(self.all_logs(), 'global_run_counter', y_val, 'condition_color')

    def scatter_gather(self, x, y, c, func):
        for i,vp in enumerate(self.vps):
            vp.scatter_gather(x, y, c, func)

    def all_logs(self, no_filter_5 = False):
        logs = [log for vp in self.vps
                    for log in vp.logs
                    if no_filter_5 or log.run < 5]
        return logs if self.jens is None \
            else logs[:-12] if self.jens else logs[12:]

    def gather_events(self):
        for log in self.all_logs():
            log.gather_events()

    def data_vs_run(self, prop):
        data = defaultdict(lambda: [[] for _ in range(4)])
        for log in self.all_logs():
            if log.run < 5:
                val = get_class_value(log, prop)
                if val is not None:
                    data[log.condition][log.run-1].append(val)
        return data

    def plot_avg_vs_run(self, prop, show=True):
        data = self.data_vs_run(prop)
        for key,val in data.items():
            plt.plot(range(4), [np.mean(i) for i in val], c=condition2color(key))
        show_plot(show)

    def plot_avg_vs_vp_id(self, prop, show=True):
        data = {}
        for vp in self.vps:
            data[vp.vp_id] = np.mean([get_class_value(log, prop) for log in vp.logs])
        plt.plot(range(len(data)), [d[1] for d in sorted(data.items())])
        show_plot(show)

    def calc_consumption_normalize_parameters(self):
        # x,y = show_scatter(logs, 'elapsed_time', 'consumption', show=False)
        # xx = np.linspace(min(x), max(x), 100)
        # for i in range(5):
        #     plt.plot(xx, np.poly1d(np.polyfit(x,y,i))(xx))
        #get normal vector of linear regression
        x, y, _ = scatter_gather(logs, 'elapsed_time', 'consumption')
        v = np.polyfit(x, y, 1)
        EcoSonic_log.consumption_normalize_origin = np.array([0, v[1]])
        v = [1,v[0]]
        v = [-v[1], v[0]] # orthogonalize
        v /= np.linalg.norm(v) # normalize
        EcoSonic_log.consumption_normalize_vector = np.array(v)
        # show_scatter(logs, 'elapsed_time', 'normalized_consumption', show=False)
        # plt.plot(xx, np.poly1d([0])(xx))
        # show_plot()

    def ttest_per_run(self, prop, runs = [0,3]):
        data = self.data_vs_run(prop)
        # cond, perm = list(data.keys()), []
        # for i,x in enumerate(cond):
        #     for j in range(i+1, len(cond)):
        #         perm.append((cond[i], cond[j]))
        perms = permutations(data.keys(), 2)
        values = []
        for cond1,cond2 in perms:
            for i in runs:
                t,p = ttest_ind(data[cond1][i], data[cond2][i])
                if t > 0: # only 'smaller than'
                    continue
                values.append( ((i,(cond1,cond2)), (t, p/2)) ) # one-sided test!
        values.sort(key = lambda x:x[1][1])
        return values

    def ttest_all(self, prop):
        data = self.data_vs_run(prop)
        for key,d in data.items():
                                                              #flatten data (use all runs)
            data[key] = [i for sublist in d for i in sublist] #alternatively: itertools.chain.from_iterable(d)
        perm = permutations(data.keys(), 2)
        values = []
        for p in perm:
            ttest = ttest_ind(data[p[0]], data[p[1]])
            if ttest[0] > 0: # only 'smaller than'
                continue
            ttest = (ttest[0], ttest[1] / 2) # one-sided test!
            values.append( (p,ttest) )
        values.sort(key = lambda x:x[1][1])
        return values

    def ttest_multiple(self, props, max_items = 5):
        values = []
        for p in props:
            values.extend([(p,v) for v in self.ttest_per_run(p)][:max_items])
            values.extend([(p,v) for v in self.ttest_all(p)][:max_items])
        values.sort(key = lambda x:x[1][1][1])
        return values

    def ttest_eye_tracking(self):
        data, values = {}, []
        for log in self.all_logs():
            data.setdefault(log.condition, []).extend([1 if p[1] < 544 else 0 for p in log.valid_eye_tracking_points])
        perms = permutations(data.keys(), 2)
        for cond1,cond2 in perms:
            t,p = ttest_ind(data[cond1], data[cond2])
            if t < 0:
                values.append(((cond1,cond2), p/2))
        values.sort(key=lambda x:x[1])
        return values

    def plot_gear_probability(self):
        data = defaultdict(lambda: [[] for _ in range(5)])
        for log in self.all_logs():
            d = data[log.condition]
            for i in range(len(log.t)):
                d[log.gear[i]].append(log.speed[i])
        f, ax = plt.subplots(5) # one plot for each gear
        for cond, d in data.items(): # each condition
            for gear,o in enumerate(d): # each gear
                hist,bins = np.histogram(o, 30)
                x = [(bins[i]+bins[i+1])/2 for i in range(len(bins)-1)]
                ax[gear].plot(x,hist,c=condition2color(cond))
        show_plot()

###################################################
#profiler.switch_to('load cache')
#lazy_cached_property.load_cache()
profiler.switch_to('preprocessing')
all = EcoSonic_All(jens=True)
all.gather_events()
with profiler.auto('sanity_check', False):
    all.sanity_check()
logs = all.all_logs()

profiler.switch_to('main')

### gear probability ###
# all.plot_gear_probability()

### eye tracking ###
#print(all.ttest_eye_tracking())

### basic tests ###
# all.plot_avg_vs_run('eye_tracking_simple_attention_ratio', filter=lambda log: log.valid_eye_tracking_data)
# all.plot_avg_vs_run('consumption', filter=lambda log: log.valid_eye_tracking_data)

### compare vp1001 with vp6001 ###
# f, ax = plt.subplots(2)
# for i, vp in enumerate([all.vps[0], all.vps[-1]]):
#     vp.show_scatter_vs_run_counter('normalized_consumption', fig = ax[i])
#     vp.show_scatter_vs_run_counter('consumption', fig = ax[i])
# show_plot()

###  normalized consumption ###
# all.calc_consumption_normalize_parameters()
# all.plot_avg_vs_run('consumption')
# plt.figure(2)
# all.plot_avg_vs_run('normalized_consumption')

### all scripted_steering are (apprx.) the same ###
# # logs = logs[:20]
# qp = max([log.quantize_params(log.pos, 1) for log in logs]) # quantize params (max) for all logs
# st = [log.quantize(log.scripted_steering, log.pos, 1, qp)[0] for log in logs] # scripted steering quantized
# sa = sum(st) / len(st) # averaged
# print("MSE:", sum([sum((s - sa)**2) for s in st]) / sum([len(s) for s in st])) # mean square error => small! => OK!

### invalidate cache ###
# lazy_cached_property.invalidate = True
# for log in logs:
#     log.gear
    # log.valid_eye_tracking_data

### steering ###
# while True:
#     log = random.choice(logs)
#     #print(log.condition, ':', log.steering_integral)
#     #print(log.scripted_steering)
#     log.show_steering()
# for i in [3,4,5,6,7,8]:
#     EcoSonic_log.max_steering_reaction_time = i
#     all.plot_avg_vs_run('avg_steering_reaction_time')

### ttest per run ###
# print(all.ttest_per_run('consumption'))
# all.plot_avg_vs_run('consumption')

### ttest all ###
# for prop in ['eye_tracking_simple_attention_ratio']:
#     print(all.ttest_all(prop))
#     all.plot_avg_vs_run(prop)

### multiple ttests ###
# print(all.ttest_multiple(['num_events', 'num_too_slow', 'num_stop_sign', 'num_traffic_light']))
# print(all.ttest_multiple(['eye_tracking_simple_attention_ratio']))
# all.plot_avg_vs_run('eye_tracking_simple_attention_ratio')

###################################################

# show_scatter(all.scatter_gather, EcoSonic_log.scatter_steering_integral_vs_run)
#show_scatter(all.scatter_gather, EcoSonic_log.scatter_consumption_vs_lap_time)

#show_scatter_old(all.scatter_gather, EcoSonic_log.scatter_consumption_vs_run, False)
# plt.figure(2)
# all.show_scatter_all_vs_run('consumption')


###################################################
profiler.switch_to('save cache')
#lazy_cached_property.save_cache()
lazy_cached_property2.save_cache()
profiler.stop()
profiler.output()
exit()



###################################################
###################################################
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