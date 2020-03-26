#!/usr/bin/env python3

from graphTools import *
import sys


df = lireDataFrame(sys.argv, speedUp=True)

# Sélection des lignes :
# df = df[(-df.threads.isin([8])) & (df.kernel.isin(['mandel']))].reset_index(drop = True)

# Création du graphe :
fig = creerGraphique(df=df,
                     x='threads',
                     y='speedup',
                     col=None,  # 'dim',
                     row='kernel',
                     plottype='lineplot',  # , yscale = 'log'
                     height=8,
                     showTitle=False)

engeristrerGraphique(fig)
