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