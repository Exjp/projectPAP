#!/usr/bin/env python3

"""
Little script by chocorion.
"""

import subprocess
from subprocess import PIPE

NUMBER_TRY = 1
TIMEOUT_SECONDS = 30

VARIANT = [
    ['seq', []],
    ['omp', []],
    ['tiled', []],
]

ARG = [
    '4partout',
    'alea'
]

SIZE = [
    '480',
    '960',
    '1920',
    '3840'
]

if __name__ == "__main__":
    for arg in ARG:
        for size in SIZE:
            ref_time = -1
            for variant in VARIANT:
                cmd = ['./run', '-v', variant[0], '-a', arg, '-s', size, '-k', 'sable', '-n'] + [i for i in variant[1]]
                time_sum = 0.

                correct = True
                for i in range(NUMBER_TRY):
                    p = subprocess.Popen(cmd, stdout=PIPE, stderr=PIPE)

                    try :
                        outs, errs = p.communicate(timeout=TIMEOUT_SECONDS)
                        time = float(errs.decode('utf-8').split('\n')[2])
                    except subprocess.TimeoutExpired:
                        p.kill()
                        correct=False
                        print("\033[31mTIMEOUT for cmd -> {} \033[0m".format(' '.join(cmd)))

                    except:
                        print("\033[31mError for cmd -> {} \033[0m".format(' '.join(cmd)))
                        correct = False
                        break
                    time_sum += time
                
                moyenne = time_sum/NUMBER_TRY
                if correct:
                    if ref_time == -1:
                        ref_time = moyenne
                    print("{:55s} \t->  {:10s} Speedup : ".format(' '.join(cmd), "{:.2f}".format(moyenne)), end="")
                    speedup = ref_time/moyenne

                    if speedup >= 1.:
                        print("\033[92m{:.2f}\033[0m".format(speedup))
                    else:
                        print("\033[91m{:.2f}\033[0m".format(speedup))

            print('')
