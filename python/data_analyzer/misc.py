__author__ = 'jhammers'

import json
import zipfile
import pickle
import os
from collections import defaultdict
from timer import ProfilerDummy


def get_class_value(obj, val_name):
    val = getattr(obj, val_name)
    if callable(val):
        val = val()
    return val
    
# must be hashable
def all_identical(it):
    return len(set(it)) <= 1

# must be a list
def all_identical2(list):
    return list.count(list[0]) == len(list)

def show_dependency_matrix(mat, row_labels, col_labels, show = True, p_mat = None):
    #print(mat)
    im = plt.matshow(mat,cmap=plt.cm.seismic, vmin = -1, vmax = 1)
    nrows, ncols = len(row_labels), len(col_labels)
    plt.yticks(range(nrows), row_labels)
    plt.xticks(range(ncols), col_labels)
    for label in im.axes.xaxis.get_ticklabels():
        label.set_rotation(90)
    x, y = np.meshgrid(np.arange(ncols), np.arange(nrows))
    for i, (x_val, y_val) in enumerate(zip(x.flatten(), y.flatten())):
        row, col = i // ncols, i % ncols
        c = "{:.3f}".format(mat[row,col])
        if p_mat is not None:
            c = c + "\n(p = {:.3f})".format(p_mat[row,col])
        plt.text(x_val, y_val, c, va='center', ha='center')
    if show:
        plt.show()

def load_json(path, is_zip=True):
    if is_zip:
        z = zipfile.ZipFile(path)
        f = z.open(z.infolist()[0])
        return json.loads(f.read().decode())
    else:
        f = open(path)
        return json.loads(f)

class lazy_property(object):
    '''
    meant to be used for lazy evaluation of an object attribute.
    property should represent non-mutable data, as it replaces itself.
    '''

    def __init__(self,fget):
        self.fget = fget
        self.func_name = fget.__name__


    def __get__(self,obj,cls):
        if obj is None:
            return None
        value = self.fget(obj)
        setattr(obj,self.func_name,value)
        return value


class lazy_cached_property(object):
    __slots__ = ['fget', 'func_name']
    cache = {}
    cache_loaded = False
    default_filename = '~/python_lazy_cached_property_cache.bin'
    invalidate = False
    dirty = False

    def __init__(self, fget):
        self.fget = fget
        self.func_name = fget.__name__

    def __get__(self, obj, cls):
        if obj is None:
            return None
        lazy_cached_property.load_cache()
        cache_id = (obj.cache_identifier, self.func_name)
        value = lazy_cached_property.cache.get(cache_id, None)
        if value is None or lazy_cached_property.invalidate:
            value = self.fget(obj)
            lazy_cached_property.cache[cache_id] = value
            lazy_cached_property.dirty = True
        setattr(obj,self.func_name,value)
        return value

    def load_cache(filename = default_filename, force = False):
        if not lazy_cached_property.cache_loaded or force:
            try:
                lazy_cached_property.cache = pickle.load(open(os.path.expanduser(filename), 'rb'))
            except FileNotFoundError:
                pass
            lazy_cached_property.cache_loaded = True

    def save_cache(filename = default_filename, force = False):
        if lazy_cached_property.dirty or force:
            pickle.dump(lazy_cached_property.cache, open(os.path.expanduser(filename), 'wb'))


class lazy_cached_property2(object):

    class single_cache(object):
        __slots__ = ['dirty', 'loaded', 'cache']
        def __init__(self):
            self.dirty = False
            self.loaded = False
            self.cache = {}

        def load(self, func_name, force = False):
            if not self.loaded or force:
                profiler = lazy_cached_property2.profiler
                try:
                    with profiler.auto('load cache: ' + func_name, False):
                        self.cache = pickle.load(lazy_cached_property2.single_cache.get_file(func_name, 'rb'))
                except FileNotFoundError:
                    pass
                self.loaded = True

        def save(self, func_name, force = False):
            if self.dirty or force:
                pickle.dump(self.cache, lazy_cached_property2.single_cache.get_file(func_name, 'wb'))

        def get_file(func_name, modus):
            # os.mkdir # TODO!
            return open(os.path.expanduser(lazy_cached_property2.folder + func_name + ".bin"), modus)

    __slots__ = ['fget', 'func_name']
    cache = defaultdict(lambda: lazy_cached_property2.single_cache())
    folder = '~/python_lazy_cache/'
    invalidate = False
    #dirty = False
    profiler = ProfilerDummy()

    def __init__(self, fget):
        self.fget = fget
        self.func_name = fget.__name__

    def __get__(self, obj, cls):
        with lazy_cached_property2.profiler.auto('get value: ' + self.func_name, False):
            if obj is None:
                return None
            cache = lazy_cached_property2.cache[self.func_name]
            cache.load(self.func_name)
            value = cache.cache.get(obj.cache_identifier, None)
            if value is None or lazy_cached_property2.invalidate:
                value = self.fget(obj)
                cache.cache[obj.cache_identifier] = value
                cache.dirty = True
            setattr(obj,self.func_name,value)
            return value

    # def load_cache(force = False):
    #     if not lazy_cached_property2.cache_loaded or force:
    #         try:
    #             lazy_cached_property2.cache = pickle.load(open(os.path.expanduser(filename), 'rb'))
    #         except FileNotFoundError:
    #             pass
    #         lazy_cached_property.cache_loaded = True

    def save_cache(force = False):
        for id, cache in lazy_cached_property2.cache.items():
            cache.save(id, force)


# code from http://stackoverflow.com/questions/7450957/how-to-implement-rs-p-adjust-in-python
# also see: http://de.wikipedia.org/wiki/Bonferroni-Methode
def correct_pvalues_for_multiple_testing(pvalues, correction_type = "Benjamini-Hochberg"):
    """
    consistent with R - print correct_pvalues_for_multiple_testing([0.0, 0.01, 0.029, 0.03, 0.031, 0.05, 0.069, 0.07, 0.071, 0.09, 0.1])
    """
    if None in pvalues:
        return pvalues
    from numpy import array, empty
    pvalues = array(pvalues)
    n = float(pvalues.shape[0])
    new_pvalues = empty(n)
    if correction_type == "Bonferroni":
        new_pvalues = n * pvalues
    elif correction_type == "Bonferroni-Holm":
        values = [ (pvalue, i) for i, pvalue in enumerate(pvalues) ]
        values.sort()
        for rank, vals in enumerate(values):
            pvalue, i = vals
            new_pvalues[i] = (n-rank) * pvalue
    elif correction_type == "Benjamini-Hochberg":
        values = [ (pvalue, i) for i, pvalue in enumerate(pvalues) ]
        values.sort()
        values.reverse()
        new_values = []
        for i, vals in enumerate(values):
            rank = n - i
            pvalue, index = vals
            new_values.append((n/rank) * pvalue)
        for i in range(0, int(n)-1):
            if new_values[i] < new_values[i+1]:
                new_values[i+1] = new_values[i]
        for i, vals in enumerate(values):
            pvalue, index = vals
            new_pvalues[index] = new_values[i]
    return new_pvalues