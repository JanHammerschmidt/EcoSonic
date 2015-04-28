from distutils.command.check import check
import numpy as np
from scipy.stats import ttest_ind, mannwhitneyu, spearmanr, wilcoxon, pearsonr, kstest #f_oneway
from matplotlib.mlab import normpdf
from itertools import permutations, product, combinations
import matplotlib.pyplot as plt
from SurveyData import SurveyFile
from misc import show_dependency_matrix, correct_pvalues_for_multiple_testing

default_condition_order = ['VIS', 'SLP', 'CNT']

### utility functions ###

class Result(object):
    def __init__(self, scale, cond1, cond2, p_ttest, p_utest, cronb1, cronb2, t):
        self.scale, self.cond1, self.cond2 = scale, cond1, cond2
        self.p_ttest, self.p_utest, self.cronb1, self.cronb2 = p_ttest, p_utest, cronb1, cronb2
        if t < 0:
            self.cond1, self.cond2 = self.cond2, self.cond1
            self.cronb1, self.cronb2 = self.cronb2, self.cronb1
    def __str__(self):
        return "{0} ({1},{2}): utest: {3}{4:.3f} ttest: {5}{6:.3f} | {7}cronbach ({1}): {8:.3f}, {9}cronbach ({2}): {10:.3f}".format(
            self.scale, self.cond1, self.cond2, '* ' if self.p_utest < 0.05 else '  ', self.p_utest,
                                                '* ' if self.p_ttest < 0.05 else '  ', self.p_ttest,
            '! ' if self.cronb1 < 0.8 else '  ', self.cronb1,
            '! ' if self.cronb2 < 0.8 else '  ', self.cronb2)

### main functions ###

def print_significant_results(survey_file, correct_pvalues=True):
    all_data = survey_file.all_data_full
    for scale, data in all_data.items():
        results = []
        # data, cronbachs = {}, {}
        # for cond, d in data_item.items():
        #     data[cond] = d.data()
        #     cronbachs[cond] = d.cronbach
        perms = combinations(default_condition_order, 2)
        for cond1, cond2 in perms:
            tmp_results = []
            for d1,d2 in product(data[cond1], data[cond2]):
                t,p = ttest_ind(d1.data, d2.data)
                u,p_u = mannwhitneyu(d1.data, d2.data)
                tmp_results += [Result(scale, cond1, cond2, p/2, p_u, d1.cronbach, d2.cronbach, t)]
            # assert len(tmp_results) == 0 or len(tmp_results) == len(list(product(data[cond1], data[cond2])))
            # anova += [f_oneway(list(data.values()))]
            if len(tmp_results) == 0:
                print('WARNING! NO RESULTS FOR:', scale, cond1, cond2)
                continue
            tmp_results.sort(key = lambda x:x.p_utest)
            results += [tmp_results[0]]

        if correct_pvalues:
            p_ttest_corrected, p_utest_corrected = [correct_pvalues_for_multiple_testing(
                                                    [getattr(r,a) for r in results]) for a in ['p_ttest', 'p_utest']]
            for i,r in enumerate(results):
                r.p_ttest = p_ttest_corrected[i]
                r.p_utest = p_utest_corrected[i]
        results.sort(key = lambda x:x.p_utest)
        # results = [x for x in results if x.cond1 == 'VIS' or x.cond2 == 'VIS']
        for r in results:
            print(r)

### output dependency table ###


scales_rows = ['Affectiveness',
              'Explicitness',
              'Unobtrusiveness']
scales_cols = ['Persuasiveness',
               'Understandability',
              'Akzeptanz / Integration into daily life',
              'Hilfreich',
              'Aufmerksamkeit / Überforderung',
              'Subjektive Selbst-Einschätzung']

def check_for_normality(data, visual=False): #returns only p
    mu = np.mean(data)
    sigma = np.std(data)
    D, p = kstest(data, 'norm', args=(mu, sigma))
    if visual:
        print(p)
        plt.hist(data, 7, normed=True)
        x = np.linspace(min(data),max(data),100)
        plt.plot(x, normpdf(x,mu,sigma))
        plt.show()
    return p

def dependency_mat(survey_file, corr_function = spearmanr, specific=None, show=True, full_data=False):
    all_data = survey_file.all_data_full if full_data else survey_file.all_data_only_if_okay
    all_f_data = {}
    for scale in scales_rows+scales_cols:
        if not scale in all_f_data:
            all_f_data[scale] = survey_file.get_combined_scale_data(all_data[scale], specific)

    mat = np.zeros((len(scales_rows),len(scales_cols)))
    corr,cache = [],{}
    for i, scale in enumerate(scales_rows): #rows
        f_data = all_f_data[scale]
        for j,scale2 in enumerate(scales_cols): #columns
            if scale == scale2:
                mat[i,j] = 1
                continue
            try:
                mat[i,j] = cache[(scale,scale2)]
            except KeyError:
                f_data2 = all_f_data[scale2]
                corr_cur = []
                for d1,d2 in product(f_data, f_data2):
                    dd1,dd2 = zip(*(i for i in zip(d1[0],d2[0]) if not -1 in i)) # weed out NONEs (-1)
                    r,p = corr_function(dd1,dd2)
                    p_ks1, p_ks2 = check_for_normality(dd1), check_for_normality(dd2)
                    corr_cur += [(r,p,d1,d2,scale,scale2, p_ks1, p_ks2, dd1, dd2)]
                max_corr = max(corr_cur, key=lambda c:abs(c[0]))
                # check_for_normality(max_corr[-2],True)
                # check_for_normality(max_corr[-1], True)
                mat[i,j] = cache[(scale,scale2)] = cache[(scale2,scale)] = max_corr[0]
                #assert mat[i,j] == min(corr_cur, key=lambda c:c[1])[0]
                corr += [max_corr]
                # if (scale2,scale) in cache:
                #     assert mat[i,j] == cache[(scale2,scale)]

    corr.sort(key=lambda x:x[1])
    for c in corr:
        print(c[4],c[5],c[0],c[1])

    show_dependency_matrix(mat, scales_rows, scales_cols, False)
    plt.gcf().canvas.set_window_title('All conditions' if specific is None else specific)
    if show:
        plt.show()

def dependency_mat_all(survey_file, corr_function=spearmanr):
    dependency_mat(survey_file, corr_function, None, False)
    for cond in default_condition_order:
        dependency_mat(survey_file, corr_function, cond, False)
    plt.show()

### possible corr_functions ###
# spearmanr => braucht keine normalverteilte Daten! p-value nur zuverlässig bei N>500
# pearsonr  => braucht streng genommen normalverteilte daten, p-value nur zuverlässig bei N>500
# lambda x,y:np.corrcoef(x,y)[1,0],0 => anscheinend das gleiche wie pearsonr


### correlation analysis between "personal factors" and the different scales ###

def dependency_mat_person(survey_file, corr_function=spearmanr, specific=None, show=True):
    all_data = survey_file.all_data_full
    scales = scales_rows + scales_cols
    all_f_data = {}
    for scale in scales:
        if not scale in all_f_data:
            scale_data = survey_file.get_combined_scale_data(all_data[scale], specific)
            if specific is None: # calc average of the results of the condition
                scale_data = [(np.mean(np.array_split(data, 3),axis=0),comb) for data,comb in scale_data]
            all_f_data[scale] = scale_data

    person_questions = survey_file.person_questions
    person_data = [survey_file.get_person_data(i) for i in range(len(person_questions))]

    mat = np.zeros((len(scales),len(person_questions)))
    corr,cache = [],{}
    for i, scale in enumerate(scales): #rows
        f_data = all_f_data[scale]
        for j,p_data in enumerate(person_data): #columns
            person_question = person_questions[j]
            try:
                mat[i,j] = cache[(scale,person_question)]
            except KeyError:
                corr_cur = []
                for d1 in f_data:
                    d2 = p_data
                    d1,d2 = zip(*(i for i in zip(d1[0],d2) if not -1 in i)) # weed out NONEs (-1)
                    r,p = corr_function(d1,d2)
                    corr_cur += [(r,p,d1,d2,scale,person_question)]
                max_corr = max(corr_cur, key=lambda c:abs(c[0]))
                mat[i,j] = cache[(scale,person_question)] = cache[(person_question,scale)] = max_corr[0]
                #assert mat[i,j] == min(corr_cur, key=lambda c:c[1])[0]
                corr += [max_corr]
                # if (scale2,scale) in cache:
                #     assert mat[i,j] == cache[(scale2,scale)]
    show_dependency_matrix(mat, scales, person_questions, False)
    plt.gcf().canvas.set_window_title('All conditions' if specific is None else specific)
    if show:
        plt.show()


### main ###

survey_file = SurveyFile()

# output significant results ##
# print('\n# significant results')
print_significant_results(survey_file)

### dependency mat for design dimensions and design goals ###

# dependency_mat_all(survey_file)
# dependency_mat(survey_file, specific=None, full_data=False)
#
### dependency mat for person data ###
# dependency_mat_person(survey_file)



# http://en.wikipedia.org/wiki/Correlation_and_dependence
# http://docs.scipy.org/doc/numpy/reference/generated/numpy.corrcoef.html