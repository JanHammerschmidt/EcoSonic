__author__ = 'jhammers'

import json
import zipfile
import pickle
import os

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

    def __init__(self, fget):
        if not lazy_cached_property.cache_loaded:
            lazy_cached_property.load_cache()
        self.fget = fget
        self.func_name = fget.__name__

    def __get__(self, obj, cls):
        if obj is None:
            return None
        cache_id = (obj.cache_identifier, self.func_name)
        value = lazy_cached_property.cache.get(cache_id, None)
        if value is None or lazy_cached_property.invalidate:
            value = self.fget(obj)
            lazy_cached_property.cache[cache_id] = value
        setattr(obj,self.func_name,value)
        return value

    def load_cache(filename = default_filename):
        try:
            lazy_cached_property.cache = pickle.load(open(os.path.expanduser(filename), 'rb'))
        except FileNotFoundError:
            pass
        lazy_cached_property.cache_loaded = True

    def save_cache(filename = default_filename):
        pickle.dump(lazy_cached_property.cache, open(os.path.expanduser(filename), 'wb'))