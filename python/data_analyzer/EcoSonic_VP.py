import os
from EcoSonic_log import EcoSonic_log

class EcoSonic_VP:
    def __init__(self, vp_id, base_folder='/Users/jhammers/Dropbox/EcoSonic/', keep_intro = False):
        self.logs = []
        folder = base_folder + "VP" + str(vp_id) + "/"
        for file in os.listdir(folder):
            if file.endswith("json.zip") and (keep_intro or "intro" not in file):
                #print(file)
                self.logs.append(EcoSonic_log(folder + file))
        if True:
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