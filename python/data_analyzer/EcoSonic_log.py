from misc import lazy_property, lazy_cached_property2

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
    @lazy_property
    def condition_rank(self): return (self.global_run_counter-1) / 4 + 1 # 1-3
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
    def acceleration(self): return self.gather("acceleration")
    @lazy_cached_property2
    def rpm(self): return self.gather("rpm")
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
    def steering_integral(self): return self.integral('steering')

    @lazy_property
    def acceleration_integral(self): return self.integral('acceleration')

    @lazy_property
    def avg_rpm(self): return np.mean(self.rpm)

    ### technical routines ###
    def sanity_check(self):
        len_t = len(self.t)
        assert self.window_size == EcoSonic_log.default_window_size
        for data in [self.dt, self.steering, self.scripted_steering]:
            assert len(data) == len_t

    def integral(self, prop, time_normalized = False):
        data, dt = get_class_value(self, prop), self.dt
        #si = sum([i[0]*abs(i[1]) for i in zip(dt,st)])
        #si = sum(np.array(dt) * abs(np.array(st)))
        acc = 0
        for i,d in enumerate(data):
            acc += dt[i] * abs(d)
        return acc / self.elapsed_time if time_normalized else acc

    # def test(self):
    #     #tq, posq = self.quantize(self.pos, self.t, 0.25)
    #     tq, posq = self.quantize(self.steering, self.t, 0.25)
    #     f, (ax1, ax2) = plt.subplots(2)
    #     print(len(self.t))
    #     print(len(tq))
    #     ax1.plot(self.t, self.steering)
    #     ax2.plot(tq, posq)
    #     plt.show()

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