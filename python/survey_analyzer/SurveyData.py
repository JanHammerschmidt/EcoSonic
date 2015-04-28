import numpy as np
from cronbachs_alpha import CronbachAlpha
from itertools import permutations, product
from misc import lazy_property

filename = '/Users/jhammers/Dropbox/Eigene Dateien/phd/Projekte/1_Auto Projekt/Evaluation/EcoSonic-Evaluation.csv'
### where is the data ? ###
condition_rows = {'VIS': 28, 'SLP': 50, 'CNT': 73}
comp_questions_row = 96
comp_questions_offset = {'VIS': 0, 'SLP': 1, 'CNT': 2}
scales = {'Affectiveness': [7,19],
          'Explicitness': ([-1,10],[-2]),
          'Unobtrusiveness': ([-4,-14],[-5]),
          'Persuasiveness': ([3,11,12],[3]),
          'Understandability': [2,17], ## 17: den aktuellen level des verbrauchs kann man bei SLP auch eigentlich gar nicht einschätzen!
          'Akzeptanz / Integration into daily life': ([-6,13,18],[-4]),
          'Hilfreich': ([5,15],[1,6]),
          'Aufmerksamkeit / Überforderung': [9,16],
          'Subjektive Selbst-Einschätzung': [20] }

person_question_row = 10
person_questions = ['Alter',
                    'Nachhaltig handeln',
                    'Musikinstrumente',
                    'Gehörbeeinträchtigung',
                    'Autofahrer',
                    'Fahrverhalten ändern, um Geld zu sparen',
                    'Fahrverhalten ändern, um CO2 einzusparen',
                    'akustischer Typ',
                    'Interesse an System, was hilft, beim Autofahren Energie zu sparen',
                    'Erfahrung mit Klangerzeugung/Audiobearbeitung/programmierung',
                    'Hintergrundgeräusche stören']

class ScaleData(object):
    def __init__(self, items, ques_ids, cond, scale):
        self.items, self.ques_ids, self.cond, self.scale = items, ques_ids, cond, scale
        if len(items) > 1:
            self.cronbach = CronbachAlpha(items)
            self.means = list(zip(ques_ids, np.mean(items,axis=1)))
            self.diffs = []
            for p in [p for p in permutations(range(len(items)),2) if p[1] > p[0]]:
                self.diffs += [list(abs(items[p[0]]-items[p[1]]))]
            self.mean_diff = np.mean(self.diffs)
        else:
            self.cronbach = 1
    @property
    def data(self): return [i for l in self.items   # flatten
                                for i in l if i != -1] # weed out -1
    @property
    def mean(self):
        def avg(i):
            good_items = [i for i in i if i != -1]
            return -1 if len(good_items) == 0 else np.mean(good_items)
        if len([i for j in self.items for i in j if i == -1]) > 0:
            pass
        return self.items[0] if len(self.items) <= 1 \
            else [avg(i) for i in zip(*self.items)] # average of all items that are not -1 (or else: -1)

class SurveyFile(object):

    def __init__(self, default_min_cronbach = 0.7):
        ### get data (int and string) ###
        self.data_int = np.genfromtxt(filename, dtype='int', delimiter=';')
        self.data_str, data_bytes = [], np.genfromtxt(filename, dtype='bytes', delimiter=';')
        for b in data_bytes:
            self.data_str.append([s.decode(errors='ignore') for s in b])

        ### fill up questions that don't have comparative questions
        for q,s in scales.items():
            if type(s) is not tuple:
                scales[q] = (s,[])

        self.default_min_cronbach = default_min_cronbach


    def sanity_check(self):
        # condition rows are correct #
        for cond, row in condition_rows.items():
            assert self.data_str[row][1].startswith(cond)

        # comparative questions row is correct
        assert self.data_str[comp_questions_row][1].startswith('Abschl')

    # returns data columns only
    def data_items(self, row, invert = False):
        data = self.data_int[row][2:-3]
        return 8-data if invert else data

    def find_question(self, ques_id, row, num_rows, invert):
        i = [i for i,k in enumerate(self.data_str[row:row+num_rows]) if k[0] == ques_id] # find question
        assert len(i) == 1
        return self.data_items(i[0] + row, invert)

    # gets all data for a set of questions
    def get_data(self, cond, questions, comp_questions, scale):
        ret, ques_ids = [], []
        row = condition_rows[cond]
        for q in questions:
            ques_id = cond[0] + str(abs(q))
            if ques_id == 'V18':
                continue
            ques_ids += [ques_id]
            ret += [self.find_question(ques_id, row, 21, invert=q<0)]
        for q in comp_questions:
            ques_id = 'E' + str(3*(abs(q)-1) + 1 + comp_questions_offset[cond])
            ques_ids += [ques_id]
            ret += [self.find_question(ques_id, comp_questions_row, 29, invert=q<0)]
        return ScaleData(ret, ques_ids, cond, scale)

    def get_person_data(self, question): # Alter, Allgemeine Fragen. question = idx of person_questions
        ques_id = 'P1' if question == 0 else 'A' + str(question)
        return self.find_question(ques_id, person_question_row, 17, False)

    def get_scale_data(self):
        all_data = {} # all data, ordered by 1) scale and 2) condition
        for scale, (questions,comp_questions) in scales.items():
            data = {}
            for cond in condition_rows: # gather data for each condition
                data[cond] = self.get_data(cond, questions, comp_questions, scale)
            all_data[scale] = data
        return all_data

    def get_scale_data_cronbach_augmented(self, only_if_okay, min_cronbach=None):
        all_data = self.get_scale_data()
        return self.augment_cronbach(all_data, only_if_okay, min_cronbach if min_cronbach is not None else self.default_min_cronbach)

    @lazy_property
    def all_data_full(self):
        return self.get_scale_data_cronbach_augmented(only_if_okay=False)

    @lazy_property
    def all_data_only_if_okay(self):
        return self.get_scale_data_cronbach_augmented(only_if_okay=True)

    ## check for cronbach ##
    def check_cronbach(self, all_data):
        all_scales = []
        for scale, data in all_data.items():
            for cond, d in data.items():
                if len(d.items) > 1: # and d.cronbach < 0.8:
                    all_scales += [d]
        all_scales.sort(key= lambda x:x.cronbach)
        for d in all_scales:
            print(d.cronbach, d.mean_diff, d.means, d.diffs)

        # show total diffs for each VP ##
        all_diffs = np.zeros(len(all_scales[0].items[0]))
        for d in all_scales:
            for diffs in d.diffs:
                all_diffs += diffs
        print(all_diffs)

    ## messing with cronbach ## (wenns nicht klappt mit dem zusammenfassen, subset nehmen (bis zu einzelnen items ..)
    def augment_cronbach(self, all_data, only_if_not_okay, min_cronbach = 0.7):
        for scale, data in all_data.items():
            # print(scale, any(i.cronbach < min_cronbach for i in data.values()))
            for cond, d in data.items():
                new_data = [] # add more permutations which are "cronbach-correct"
                for maxlen in range(len(d.items)-1,0,-1):
                    for p in [p for p in permutations(range(len(d.items)), maxlen) if len(p) <= 1 or p[1] > p[0]]:
                        new_set = [d.items[p] for p in p]
                        new_d = ScaleData(new_set, [d.ques_ids[p] for p in p], cond, scale)
                        if new_d.cronbach >= min_cronbach:
                            new_data.append(new_d)
                if d.cronbach >= min_cronbach:
                    if only_if_not_okay:
                        new_data = [d]
                    else:
                        new_data += [d]
                data[cond] = new_data
        return all_data

    def get_combined_scale_data(self, data, specific=None, ordering=['VIS','SLP','CNT']):
        """
        returns a number of possible data for one scale, each ordered by condition (vis,slp,cnt)
        the number of possibilities is determined by the number of scale-representations
        for each condition (all combinations that are "cronbach-approved"..)
        """
        if specific is None:
            combs = list(product(data['VIS'], data['SLP'], data['CNT'])) #all combinations of conditions
        else:
            combs = list(product(data[specific]))
        print(data['VIS'][0].scale, len(combs))
        for i,comb in enumerate(combs): #in each comb is a data-series: vis,slp,cnt..
            combs[i] = ([v for scale in comb for v in scale.mean],comb) # sequentially combine the means for each condition-combination
        return combs

    @property
    def person_questions(self):
        return person_questions