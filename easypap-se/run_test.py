#!/usr/bin/env python3

"""
Little script by chocorion.
"""

import subprocess, sys, os
from subprocess import PIPE

NUMBER_TRY = 2
TIMEOUT_SECONDS = 390

VARIANT = [
    ['seq', [], [1]],
    ['tileddb', ['-g', '8'], [i for i in range(2, 49, 2)]],
    ['tiledsharedy', ['-g', '8'], [i for i in range(2, 49, 2)]],
    ['omp', ['-g', '8'], [i for i in range(2, 49, 2)]],
    ['omp', ['-g', '16'], [i for i in range(2, 49, 2)]],
    ['omp', ['-g', '32'], [i for i in range(2, 49, 2)]],
    ['omp', ['-g', '64'], [i for i in range(2, 49, 2)]],
    ['omp', ['-g', '128'], [i for i in range(2, 49, 2)]],
    ['omp', ['-g', '256'], [i for i in range(2, 49, 2)]],
    
    #['omp_small_lines', ['-g', '8'], [i for i in range(2, 49)]],
    #['omp_damier', ['-g', '8'], [i for i in range(2, 49)]],
    # ['omp_picnic', ['-g', '8'],  [i for i in range(2, 49, 2)]],
    # ['omp_picnic', ['-g', '12'], [i for i in range(2, 49, 2)]],
    # ['omp_picnic', ['-g', '16'], [i for i in range(2, 49, 2)]],
    # ['omp_picnic', ['-g', '24'], [i for i in range(2, 49, 2)]],
    # ['omp_picnic', ['-g', '32'], [i for i in range(2, 49, 2)]],
    # ['omp_picnic', ['-g', '64'], [i for i in range(2, 49, 2)]],
    # ['omp_picnic', ['-g', '128'], [i for i in range(2, 49, 2)]],
    # ['omp_picnic', ['-g', '256'], [i for i in range(2, 49, 2)]],
    # ['omp_picnic', ['-g', '512'], [i for i in range(2, 49, 2)]],
]

# Boolean for dump diff
ARG = [
    ['4partout', True, ['480']],
    #['alea', True, ['3840']]#960, '1920', '3840']]
]


def clean_png():
    subprocess.run(
        ['rm *.png'],
        shell=True,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL
    )

if __name__ == "__main__":
    clean_png()

    for arg in ARG:
        for size in arg[2]:
            ref_time = -1
            for variant in VARIANT:
                for num_threads in variant[2]:
                    cmd = ['./run', '-v', variant[0], '-a', arg[0], '-s', size, '-k', 'sable', '-n'] + [i for i in variant[1]]

                    if arg[1]:
                        cmd += ['-du']
                    
                    time_sum = 0.

                    correct = True
                    for i in range(NUMBER_TRY):
                        p = subprocess.Popen(cmd, stdout=PIPE, stderr=PIPE, env=dict(os.environ, **{"OMP_PLACES":"cores", "OMP_SCHEDULE":"dynamic", "OMP_NUM_THREADS":str(num_threads)}))  

                        try :
                            outs, errs = p.communicate(timeout=TIMEOUT_SECONDS)
                            time = float(errs.decode('utf-8').split('\n')[2])
                        except subprocess.TimeoutExpired:
                            p.kill()
                            correct=False
                            print("\033[31mTIMEOUT for cmd -> {} --> {} threads\033[0m".format(' '.join(cmd), num_threads))

                        except:
                            print("\033[31mError for cmd -> {} --> {} threads\033[0m".format(' '.join(cmd), num_threads))
                            correct = False
                            break
                        time_sum += time
                    
                    moyenne = time_sum/NUMBER_TRY
                    if correct:
                        CORRECT_OUTPUT = True
                        if ref_time == -1:
                            ref_time = moyenne
                        else:
                            # On n'est plus à la version seq
                            if arg[1]:
                                p = subprocess.run(
                                    ['diff dump-sable-{}-dim-{}-iter-*.png'.format('seq', size) + ' dump-sable-{}-dim-{}-iter-*.png'.format(variant[0], size)],
                                    shell=True,
                                    stdout=PIPE,
                                    stderr=PIPE
                                )

                                if len(p.stdout.decode('utf-8')) != 0:
                                    CORRECT_OUTPUT = False

                                p = subprocess.run(
                                    ['rm dump-sable-{}-dim-{}-iter-*.png'.format(variant[0], size)],
                                    shell=True,
                                    stdout=subprocess.DEVNULL,
                                    stderr=subprocess.DEVNULL
                                )

                        print("{:2d} threads -> {:65s} --> {:8s} Speedup : ".format(num_threads, ' '.join(cmd), "{:.2f}".format(moyenne)), end="")
                        speedup = ref_time/moyenne

                        if speedup >= 1.:
                            print("\t\033[92m{:.2f}\033[0m".format(speedup), end="")
                        else:
                            print("\t\033[91m{:.2f}\033[0m".format(speedup), end="")

                        if arg[1]:
                            if CORRECT_OUTPUT:
                                print("\t\033[92mcorrect   output\033[0m")
                            else:
                                print("\t\033[91mincorrect output\033[0m")
                        else:
                            print("\t\033[34mno output verification\033[0m")

            print('')
            clean_png()

    
