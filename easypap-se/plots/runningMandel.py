from graphTools import *
from expTools import *
import os

#Dictionnaire avec les options de compilations d'apres commande
options={}
options["-k "] = ["mandel"]
options["-i "] = [5]
options["-v "] = ["omp"]
options["-s "] = [1024, 2048]
options["-of "] = ["./plots/data/fichier_perf.csv"] #Pour renseigner l'option '-of' il faut donner le chemin depuis le fichier easypap


#Dictionnaire avec les options OMP
ompenv = {}
ompenv["OMP_NUM_THREADS="]=[1] + list(range(4,9,4))
ompenv["OMP_PLACES="] =["cores","threads"]

nbrun = 2
#Lancement des experiences
execute('./run ', ompenv, options, nbrun, verbose = False, easyPath="..")

#Lancement de la version seq avec le nombre de thread impose a 1
options["-v "] = ["seq"]
ompenv["OMP_NUM_THREADS="]=[1]
execute('./run', ompenv, options, nbrun, verbose = False, easyPath="..")
