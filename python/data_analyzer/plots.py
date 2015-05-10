__author__ = 'jhammers'

import matplotlib.pyplot as plt
import numpy as np
from timer import ProfilerDummy
from data_analyzer.misc import get_class_value

boxplot_bootstrap=5000
boxplot_whiskers = [5,95]
boxplot_showfliers = False
boxplot_notch = True
boxplot_show_means = True

###########################
profiler = ProfilerDummy()

def set_profiler(p):
    global profiler
    profiler = p

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

def _get_params(show, fig):
    show = fig is None if show is None else show
    plot = plt if fig is None else fig
    return show, plot

def _set_labels(ylabel, xlabel, fig, plot):
    if ylabel != '':
        if fig is None:
            plot.figure().canvas.set_window_title(ylabel)
            plot.ylabel(ylabel)
            plot.xlabel(xlabel)
        else:
            fig.set_ylabel(ylabel)
            fig.set_xlabel(xlabel)


def _set_box_color(bp, color):
    plt.setp(bp['boxes'], color=color)
    plt.setp(bp['whiskers'], color=color)
    plt.setp(bp['caps'], color=color)
    plt.setp(bp['medians'], color=color)
    plt.setp(bp['means'], color=color)

def show_scatter(items, x_val, y_val, c_val='condition_color', ylabel = None, xlabel = None, show = None, fig = None):
    x, y, c = scatter_gather(items, x_val, y_val, c_val)
    ylabel = y_val if ylabel is None else ylabel
    xlabel = x_val if xlabel is None else xlabel # TODO!
    show, plot = _get_params(show, fig)
    _set_labels(ylabel, xlabel, fig, plot)
    plot.scatter(x, y, color=c)
    show_plot(show)
    return x,y


# def std_error(vals):
#     return scipy.

def errorbar(xval, yvals, colors, ylabel='', xlabel='', show=None, fig=None):
    show, plot = _get_params(show, fig)
    _set_labels(ylabel, xlabel, fig, plot)
    pl = plot.errorbar(xval, [np.mean(i) for i in yvals], yerr=np.std(yvals,axis=1)/np.sqrt([len(i) for i in yvals]), c=colors)
    pl[-1][0].set_linestyle('--') #epl[-1][0] is the LineCollection objects of the errorbar lines

def plot(xval, yval, colors, ylabel='', xlabel='', show=None, fig=None):
    show, plot = _get_params(show, fig)
    _set_labels(ylabel, xlabel, fig, plot)
    plot.plot(xval, yval, c=colors)

def boxplot(data, labels, ylabel = '', xlabel = '', show = None, fig = None, ylim=None):
    show, plot = _get_params(show, fig)
    _set_labels(ylabel, xlabel, fig, plot)
    plot.boxplot(data, labels=labels, showmeans=boxplot_show_means, whis=boxplot_whiskers, notch=boxplot_notch, bootstrap=boxplot_bootstrap, showfliers=boxplot_showfliers)
    if ylim is not None:
        plot.ylim(ylim) if fig is None else fig.set_ylim(ylim)
    show_plot(show)

# data: vector (for each condition) of vector (for each x-pos) of data items
def group_boxplot(data, colors, conditions, labels = None, ylabel = '', xlabel = '', show = None, fig = None, box_width = 0.5, intra_dist = 0.7, inter_dist = 1.0):

    cc = len(conditions)
    assert len(data) == len(colors) == cc
    show, plot = _get_params(show, fig)
    _set_labels(ylabel, xlabel, fig, plot)

    group_distance = (cc-1) * intra_dist + inter_dist
    for i,d in enumerate(data):
        positions = np.arange(len(d))*group_distance + i*intra_dist
        ploti = plot.boxplot(d, positions=positions, widths=box_width, showmeans=True, whis=boxplot_whiskers, notch=True, bootstrap=boxplot_bootstrap, showfliers=boxplot_showfliers)
        _set_box_color(ploti, colors[i])
        plot.plot(positions, [np.mean(i) for i in d], c=colors[i], label=conditions[i])
    # for i,c in enumerate(colors):
    #     plt.plot([], c=c, label=conditions[i])
    plot.legend()

    if labels is None:
        labels = np.arange(max([len(d) for d in data])) + 1
    all_data = [i for d in data for p in d for i in p]
    dmin, dmax = min(all_data), max(all_data)
    margin = (dmax-dmin) * 0.05
    xticks = np.arange(len(labels)) * group_distance + 0.5 * (cc-1) * intra_dist
    if fig is None:
        xlim, ylim = plot.xlim, plot.ylim
        plot.xticks(xticks, labels)
    else:
        xlim, ylim = plot.set_xlim, plot.set_ylim
        plot.set_xticks(xticks)
        plot.set_xticklabels(labels)
    xlim(-intra_dist, len(labels) * group_distance) # passt so ungefähr ..
    #ylim(dmin-margin, dmax+margin)
    #plt.tight_layout()
    show_plot(show)
