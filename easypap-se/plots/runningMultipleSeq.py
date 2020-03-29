from graphTools import *
from expTools import *
import os

#Dictionnaire avec les options de compilations d'apres commande
options={}
options["-k "] = ["mandel", "vie", "fourmilière", "sable"]
options["-i "] = [5]
options["-v "] = ["seq"]
options["-s "] = [1024, 2048]
options["-of "] = ["./plots/data/fichier_perf.csv"] #Pour renseigner l'option '-of' il faut donner le chemin depuis le fichier easypap


#Dictionnaire avec les options OMP
ompenv = {}
ompenv["OMP_NUM_THREADS="]=[1]
ompenv["OMP_PLACES="] =["cores","threads"]

nbrun = 2

#Lancement des experiences
execute('./run ', ompenv, options, nbrun, verbose = False)

