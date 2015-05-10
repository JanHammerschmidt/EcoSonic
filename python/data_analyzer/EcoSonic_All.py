import os
import numpy as np, matplotlib.pyplot as plt
from scipy.stats import ttest_ind
from collections import defaultdict
from itertools import permutations
from prettytable import PrettyTable
from misc import get_class_value
from EcoSonic_log import EcoSonic_log
from EcoSonic_VP import EcoSonic_VP
from plots import boxplot, group_boxplot, errorbar, show_plot

default_condition_order = ['VIS', 'CNT', 'SLP']

def condition2color(cond):
    if cond == "VIS":
        return '#2C7BB6' #"blue"
    elif cond == "SLP":
        return '#D7191C' # "red"
    elif cond == "CNT":
        return '#008837' # "green"
    elif cond == "intro":
        return "yellow"
    else:
        print("WARNING! unknown condition: " + cond)
        return "black"

class Result(object):
    def __init__(self, prop, cond1, cond2, p_ttest, run = -1):
        self.prop, self.cond1, self.cond2, self.p_ttest, self.run = prop, cond1, cond2, p_ttest, run

    def __str__(self):
        return "{}{} ({},{}): ttest: {}{:.3f}".format(
            self.prop, '['+str(self.run)+']' if self.run >= 0 else '[A]',
            self.cond1, self.cond2, '* ' if self.p_ttest < 0.05 else '  ', self.p_ttest)

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

    def all_logs(self, filter_5 = True):
        logs = [log for vp in self.vps
                    for log in vp.logs
                    if log.run < 5 or not filter_5]
        return logs if self.jens is None \
            else logs[:-12] if self.jens else logs[12:]

    def gather_events(self):
        for log in self.all_logs():
            log.gather_events()

    def data_vs_run(self, prop, global_run_counter = False):
        max_items = 12 if global_run_counter else 4
        data = defaultdict(lambda: [[] for _ in range(max_items)])
        for log in self.all_logs():
            val = get_class_value(log, prop)
            if val is not None:
                idx = log.global_run_counter if global_run_counter else log.run
                assert idx <= max_items
                data[log.condition][idx-1].append(val)
        return data, max_items

    def plot_avg_vs_run(self, prop, global_run_counter = False, show=None, fig=None):
        data, max_items = self.data_vs_run(prop, global_run_counter=global_run_counter)
        for key,values in data.items():
            errorbar(np.arange(max_items)+1, values, ylabel=prop,
                 xlabel='global run counter' if global_run_counter else 'run',
                 colors=condition2color(key), show=show, fig=fig)
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
        data,_ = self.data_vs_run(prop)
        # cond, perm = list(data.keys()), []
        # for i,x in enumerate(cond):
        #     for j in range(i+1, len(cond)):
        #         perm.append((cond[i], cond[j]))
        perms = permutations(data.keys(), 2)
        values = []
        for cond1,cond2 in perms:
            for run in runs:
                t,p = ttest_ind(data[cond1][run], data[cond2][run])
                if t > 0: # only 'smaller than'
                    continue
                values.append(Result(prop, cond1, cond2, p/2, run)) # one-sided test!
        values.sort(key = lambda x:x.p_ttest)
        return values

    def ttest_all(self, prop):
        data,_ = self.data_vs_run(prop)
        for key,d in data.items():
                                                              #flatten data (use all runs)
            data[key] = [i for sublist in d for i in sublist] #alternatively: itertools.chain.from_iterable(d)
        perm = permutations(data.keys(), 2)
        values = []
        for cond1,cond2 in perm:
            t,p = ttest_ind(data[cond1], data[cond2])
            if t > 0: # only 'smaller than'
                continue
            values.append(Result(prop, cond1, cond2, p))
        values.sort(key = lambda x:x.p_ttest)
        return values

    def ttest_multiple(self, props, max_items = 5):
        values = []
        for p in props:
            values.extend(self.ttest_per_run(p)[:max_items])
            values.extend(self.ttest_all(p)[:max_items])
        values.sort(key = lambda x:x.p_ttest)
        return values

    def ttest_multiple_visual(self, props, max_items = 5):
        for p in props:
            values = []
            values.extend([(p,v) for v in self.ttest_per_run(p)][:max_items])
            values.extend([(p,v) for v in self.ttest_all(p)][:max_items])
            values.sort(key = lambda x:x[1][1][1])
            print(values)
            #if values[0][1][1][1] <= 0.1:
            #    self.plot_avg_vs_run(p)
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

    def boxplot_all(self, prop, show=None, fig=None):
        values, data = {}, []
        for log in self.all_logs(filter_5 = True):
            val = get_class_value(log, prop)
            if val is not None:
                values.setdefault(log.condition, []).append(val)
        conditions = EcoSonic_VP.condition_order_from_vp_id(1)
        data = [values[cond] for cond in conditions]
        boxplot(data, conditions, prop, 'condition', show, fig)
        # plt.boxplot(data, labels = conditions, showmeans = True, meanline = False)
        # show_plot(show)

    def boxplot_vs_run(self, prop, show=None, fig=None):
        data, _ = self.data_vs_run(prop)
        conditions = EcoSonic_VP.condition_order_from_vp_id(1)
        data = [data[cond] for cond in conditions]
        group_boxplot(data, [condition2color(c) for c in conditions], conditions, ylabel=prop, xlabel = 'run', show=show, fig=fig)

    def data_matrix(self, prop):
        conditions = default_condition_order[:]
        values = []
        for log in self.all_logs():
            val = get_class_value(log, prop)
            if val is not None:
                values.append((log.condition, log.condition_rank, val))

        data = np.ndarray((4,4))
        for i,cond in enumerate(conditions):
            for j in range(3):
                data[i,j] = np.mean([val for condition, rank, val in values if condition == cond and rank == j+1])
            data[i,3] = np.mean([val for condition, _, val in values if condition == cond])
        for j in range(3):
            data[3,j] = np.mean([val for _, rank, val in values if rank == j+1])
        data[3,3] = np.mean([val for _,_,val in values])

        t = PrettyTable(['', '1', '2', '3', 'M'])
        conditions += ['M']
        for i, row in enumerate(data):
            t.add_row([conditions[i]]+list(row))
        t.float_format = ".3"
        print(t)

    def prop_inspection(self, prop):
        self.data_matrix(prop)
        for v in self.ttest_multiple([prop], 2):
            print(v)
        # plt.figure().canvas.set_window_title(prop)
        f, ax = plt.subplots(2,2)
        ax = ax.flatten()
        self.boxplot_all(prop, fig=ax[0])
        self.boxplot_vs_run(prop, fig=ax[1])
        self.plot_avg_vs_run(prop, fig=ax[2], global_run_counter=True)
        self.plot_avg_vs_run(prop, fig=ax[3])
        show_plot()