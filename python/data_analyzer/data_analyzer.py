__author__ = 'jhammers'

print('> start')
from timer import Profiler, ProfilerExclusive
profiler = ProfilerExclusive(True)

profiler.start('import', True)
from misc import lazy_cached_property2
# from misc import *
# import numpy as np
# import matplotlib.pyplot as plt
# from idlelib.SearchEngine import get
# import math, random, plots
import plots
# from collections import defaultdict, OrderedDict
# from itertools import permutations, accumulate
# from scipy.stats import ttest_ind
# from plots import show_plot, group_boxplot, boxplot, show_scatter, plot, errorbar

# from EcoSonic_log import EcoSonic_log
# from EcoSonic_VP import EcoSonic_VP
from EcoSonic_All import EcoSonic_All

lazy_cached_property2.profiler = profiler
profiler.switch_to('parsing', False)



# def speed_analyze(log):
#     plt.plot(log.pos, log.speed)
#     s = load_json("/Users/jhammers/test.json.zip")
#     s = s["speed"]
#     x = range(len(items))
#     plt.plot([2*x for x in range(0,len(s))], s)
#     plt.plot(s)
#     plt.show()

# profiler.switch_to('parsing:EcoSonic_log', False)



# profiler.switch_to('parsing:EcoSonic_VP', False)



# profiler.switch_to('parsing:EcoSonic_All', False)




###################################################
#profiler.switch_to('load cache')
#lazy_cached_property.load_cache()
profiler.switch_to('preprocessing')
plots.set_profiler(profiler)
all = EcoSonic_All(jens=False)
all.gather_events()
# with profiler.auto('sanity_check', False):
#     all.sanity_check()
logs = all.all_logs()

profiler.switch_to('main')

### data matrix ###
# all.data_matrix('consumption')

### box plot vs run ###
# all.boxplot_vs_run('consumption', show=True)

### "global" boxplot ###
# all.boxplot_all('consumption')


### gear probability ###
# all.plot_gear_probability()

### eye tracking ###
# print(all.ttest_eye_tracking())

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
# #     log.show_steering()
# for i in [3,4,5,6,7,8]:
#     EcoSonic_log.max_steering_reaction_time = i
#     all.plot_avg_vs_run('avg_steering_reaction_time')

### ttest per run ###
# print(all.ttest_per_run('elapsed_time'))
# print(all.ttest_all('elapsed_time'))
# all.plot_avg_vs_run('elapsed_time')

#TODO: lern-kapazität

### ttest all ###
# for prop in ['eye_tracking_simple_attention_ratio']:
#     print(all.ttest_all(prop))
#     all.plot_avg_vs_run(prop)

### multiple ttests ###
# print(all.ttest_multiple(['num_events', 'num_too_slow', 'num_stop_sign', 'num_traffic_light', 'num_speeding']))
# print(all.ttest_multiple(['eye_tracking_simple_attention_ratio']))
# all.plot_avg_vs_run('eye_tracking_simple_attention_ratio')

props = []
props.extend(['steering_integral', 'avg_steering_reaction_time'])
props.extend(['num_events', 'num_too_slow', 'num_stop_sign', 'num_traffic_light', 'num_speeding'])
props.extend(['elapsed_time', 'consumption', 'eye_tracking_simple_attention_ratio', 'normalized_consumption'])

#all.ttest_multiple_visual(props)

# for prop in props:
#     all.prop_inspection(prop)
all.prop_inspection('elapsed_time')

###################################################

# show_scatter(all.scatter_gather, EcoSonic_log.scatter_steering_integral_vs_run)
#show_scatter(all.scatter_gather, EcoSonic_log.scatter_consumption_vs_lap_time)
# plt.figure(2)

#prop = 'avg_rpm'
# all.data_matrix(prop)
# all.boxplot_all(prop)
# plt.figure(1)
# all.boxplot_vs_run(prop, False)
# plt.figure(2)
# all.plot_avg_vs_run(prop, show = False, global_run_counter=True)
# plt.figure(3)
# all.plot_avg_vs_run(prop)


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