import time

class Timer(object):
    def __init__(self, name = 'Timer', verbose=True):
        self.verbose = verbose if name == 'Timer' else True
        self.name = name

    def __enter__(self):
        self.start = time.perf_counter()
        return self

    def __exit__(self, *args):
        self._elapsed = time.perf_counter() - self.start
        if self.verbose:
            self.print()

    def print(self):
        print('%s: %f seconds' % (self.name, self.elapsed))

    @property
    def elapsed(self):
        return self._elapsed

class ProfilerItem(object):
    __slots__ = ['time', 'count', 'started', 'start_time', 'replaced']

    def __init__(self):
        self.time, self.count = 0, 0
        self.started = False
        self.replaced = None

    def start(self):
        assert not self.started
        self.count += 1
        self.started = True
        self.start_time = time.perf_counter()

    def stop(self):
        assert self.started
        t = time.perf_counter() - self.start_time
        self.time += t
        self.started = False
        return t

    def set_replaced(self, item):
        assert item != self
        self.replaced = item


class Profiler(object):
    class Auto(object):
        __slots__ = ['profiler', 'name', 'verbose']
        def __init__(self, profiler, name, verbose = None):
            self.profiler, self.name, self.verbose = profiler, name, verbose
        def __enter__(self):
            self.profiler.start(self.name, self.verbose)
        def __exit__(self, *args):
            self.profiler.stop(self.name)

    def __init__(self):
        self.items = {}

    def start(self, name):
        self.items.setdefault(name, ProfilerItem()).start()

    def stop(self, name):
        self.items[name].stop()

    def stop_all(self):
        for i in self.items.values():
            if i.started:
                i.stop()

    def output(self):
        for name, item in sorted(self.items.items(), key=lambda i:i[1].time, reverse=True):
            print('%s: %f' % (name, item.time))

class ProfilerExclusive(object):
    # class AutoStop(object):
    #     __slots__ = ['profiler', 'name', 'started']
    #     def __init__(self, profiler, name):
    #         self.profiler, self.name = profiler, name
    #     def __enter__(self):
    #         self.profiler.start(self.name)
    #     def __exit__(self):
    #         self.profiler.stop(self.name)

    def __init__(self, verbose = False):
        self.verbose = verbose
        self.items = {}
        self.started = None

    def section(self, name): print(">", name)
    def _verbose(self, verbose):
        return verbose if verbose != None else self.verbose

    def start(self, name, verbose = None):
        if self._verbose(verbose):
            self.section(name)
        if self.started != None:
            self.started.stop()
        item = self.items.setdefault(name, ProfilerItem())
        item.set_replaced(self.started)
        self.started = item
        item.start()

    def _stop_item(self):
        assert self.started is not None
        self.started.stop()

    def stop(self, _ = None):
        self._stop_item()
        self.started = self.started.replaced
        if self.started is not None:
            self.started.start() # TODO: no count += 1 here!

    def switch_to(self, name, verbose = None):
        if self._verbose(verbose):
            self.section(name)
        self._stop_item()
        item = self.items.setdefault(name, ProfilerItem())
        item.set_replaced(self.started.replaced)
        self.started = item
        item.start()

    def pause(self):
        assert self.started is not None
        self.started.stop()

    def resume(self):
        assert self.started is not None
        self.started.start()

    def stop_all(self):
        if self.started != None:
            self.started.stop()
            started = None

    def output(self):
        t_all = sum(map(lambda i: i.time, self.items.values()))
        print("took %f seconds" % t_all)
        for name, item in sorted(self.items.items(), key=lambda i:i[1].time, reverse=True):
            print('[%i%%] %s: %f (%i)' % (item.time / t_all * 100, name, item.time, item.count))
